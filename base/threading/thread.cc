// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/thread.h"

#include "base/lazy_instance.h"
#include "base/third_party/dynamic_annotations/dynamic_annotations.h"
#include "base/threading/thread_local.h"
#include "base/synchronization/waitable_event.h"

namespace base {

namespace {

// We use this thread-local variable to record whether or not a thread exited
// because its Stop method was called.  This allows us to catch cases where
// MessageLoop::Quit() is called directly, which is unexpected when using a
// Thread to setup and run a MessageLoop.
base::LazyInstance<base::ThreadLocalBoolean> lazy_tls_bool(
    base::LINKER_INITIALIZED);

}  // namespace

// This task is used to trigger the message loop to exit.
class ThreadQuitTask : public Task {
 public:
  virtual void Run() {
    MessageLoop::current()->Quit();
    Thread::SetThreadWasQuitProperly(true);
  }
};

// Used to pass data to ThreadMain.  This structure is allocated on the stack
// from within StartWithOptions.
struct Thread::StartupData {
  // We get away with a const reference here because of how we are allocated.
  const Thread::Options& options;

  // Used to synchronize thread startup.
  WaitableEvent event;

  explicit StartupData(const Options& opt)
      : options(opt),
        event(false, false) {}
};

Thread::Thread(const char* name)
    : started_(false),
      stopping_(false),
      startup_data_(NULL),
      thread_(0),
      message_loop_(NULL),
      thread_id_(kInvalidThreadId),
      name_(name) {
#if defined(ANDROID)
  // For debugging. See http://b/5244039
  is_starting_ = false;
#endif
}

Thread::~Thread() {
  Stop();
}

bool Thread::Start() {
#if defined(ANDROID)
  // For debugging. See http://b/5244039
  LOG(INFO) << "Start(), this=" << this << ", name_=" << name_;
#endif
  return StartWithOptions(Options());
}

bool Thread::StartWithOptions(const Options& options) {
  DCHECK(!message_loop_);

#if defined(ANDROID)
  // For debugging. See http://b/5244039
  LOG(INFO) << "StartWithOptions(), this=" << this << ", name_=" << name_;
  is_starting_lock_.Acquire();
  CHECK(!is_starting_);
  is_starting_ = true;
  is_starting_lock_.Release();
#endif

  SetThreadWasQuitProperly(false);

  StartupData startup_data(options);
#if defined(ANDROID)
  // For debugging. See http://b/5244039
  LOG(INFO) << "StartWithOptions() created startup_data, this=" << this
      << ", name_=" << name_;
#endif
  startup_data_ = &startup_data;

  if (!PlatformThread::Create(options.stack_size, this, &thread_)) {
    DLOG(ERROR) << "failed to create thread";
    startup_data_ = NULL;
    return false;
  }

  // Wait for the thread to start and initialize message_loop_
#if defined(ANDROID)
  // For debugging. See http://b/5244039
  LOG(INFO) << "StartWithOptions() waiting for thread to start, this=" << this
      << ", name_=" << name_;
#endif
  startup_data.event.Wait();

  // set it to NULL so we don't keep a pointer to some object on the stack.
#if defined(ANDROID)
  // For debugging. See http://b/5244039
  LOG(INFO) << "StartWithOptions() clearing startup_data_, this=" << this
      << ", name_=" << name_;
  startup_data_ = reinterpret_cast<StartupData*>(0xbbadbeef);
#else
  startup_data_ = NULL;
#endif
  started_ = true;

#if defined(ANDROID)
  // For debugging. See http://b/5244039
  is_starting_lock_.Acquire();
  is_starting_ = false;
  is_starting_lock_.Release();
#endif

  DCHECK(message_loop_);
  return true;
}

void Thread::Stop() {
#if defined(ANDROID)
  // For debugging. See http://b/5244039
  LOG(INFO) << "Stop(), this=" << this << ", name_=" << name_;
#endif
  if (!thread_was_started())
    return;

  StopSoon();

  // Wait for the thread to exit.
  //
  // TODO(darin): Unfortunately, we need to keep message_loop_ around until
  // the thread exits.  Some consumers are abusing the API.  Make them stop.
  //
  PlatformThread::Join(thread_);

  // The thread should NULL message_loop_ on exit.
  DCHECK(!message_loop_);

  // The thread no longer needs to be joined.
  started_ = false;

  stopping_ = false;
#if defined(ANDROID)
  // For debugging. See http://b/5244039
  LOG(INFO) << "Stop() done, this=" << this << ", name_=" << name_;
#endif
}

void Thread::StopSoon() {
  // We should only be called on the same thread that started us.

  // Reading thread_id_ without a lock can lead to a benign data race
  // with ThreadMain, so we annotate it to stay silent under ThreadSanitizer.
  DCHECK_NE(ANNOTATE_UNPROTECTED_READ(thread_id_), PlatformThread::CurrentId());

  if (stopping_ || !message_loop_)
    return;

  stopping_ = true;
  message_loop_->PostTask(FROM_HERE, new ThreadQuitTask());
}

void Thread::Run(MessageLoop* message_loop) {
  message_loop->Run();
}

void Thread::SetThreadWasQuitProperly(bool flag) {
  lazy_tls_bool.Pointer()->Set(flag);
}

bool Thread::GetThreadWasQuitProperly() {
  bool quit_properly = true;
#ifndef NDEBUG
  quit_properly = lazy_tls_bool.Pointer()->Get();
#endif
  return quit_properly;
}

void Thread::ThreadMain() {
  {
#if defined(ANDROID)
    // For debugging. See http://b/5244039
    LOG(INFO) << "ThreadMain() starting, this=" << this
        << ", name_=" << name_;
#endif
    // The message loop for this thread.
    MessageLoop message_loop(startup_data_->options.message_loop_type);

    // Complete the initialization of our Thread object.
    thread_id_ = PlatformThread::CurrentId();
    PlatformThread::SetName(name_.c_str());
    ANNOTATE_THREAD_NAME(name_.c_str());  // Tell the name to race detector.
    message_loop.set_thread_name(name_);
    message_loop_ = &message_loop;
    message_loop_proxy_ = MessageLoopProxy::CreateForCurrentThread();

    // Let the thread do extra initialization.
    // Let's do this before signaling we are started.
    Init();

#if defined(ANDROID)
    // For debugging. See http://b/5244039
    LOG(INFO) << "ThreadMain() signalling, this=" << this
        << ", name_=" << name_;
#endif
    startup_data_->event.Signal();
    // startup_data_ can't be touched anymore since the starting thread is now
    // unlocked.

    Run(message_loop_);

    // Let the thread do extra cleanup.
    CleanUp();

    // Assert that MessageLoop::Quit was called by ThreadQuitTask.
    DCHECK(GetThreadWasQuitProperly());

    // We can't receive messages anymore.
    message_loop_ = NULL;
    message_loop_proxy_ = NULL;
  }
  thread_id_ = kInvalidThreadId;
}

}  // namespace base
