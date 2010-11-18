/*
 * Copyright 2010, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ANDROID_URL_REQUEST_CONTEXT_GETTER_H
#define ANDROID_URL_REQUEST_CONTEXT_GETTER_H

#include "base/message_loop_proxy.h"
#include "base/thread.h"
#include "common/net/url_request_context_getter.h"
#include "net/url_request/url_request_context.h"

class AndroidURLRequestContextGetter : public URLRequestContextGetter {
public:
  AndroidURLRequestContextGetter()
    : context_getter_function_(0), io_thread_(0) { };

  virtual ~AndroidURLRequestContextGetter() { }

  // URLRequestContextGetter implementation
  virtual URLRequestContext* GetURLRequestContext();
  virtual scoped_refptr<base::MessageLoopProxy> GetIOMessageLoopProxy() const;

  static AndroidURLRequestContextGetter* Get();

  typedef URLRequestContext* (URLRequestContextGetterFunction)();
  void SetURLRequestContextGetterFunction(
      URLRequestContextGetterFunction* function);
  void SetIOThread(base::Thread* io_thread) { io_thread_ = io_thread; }

private:
  static scoped_refptr<AndroidURLRequestContextGetter> instance_;
  URLRequestContextGetterFunction* context_getter_function_;
  mutable base::Thread* io_thread_;
};

#endif // ANDROID_URL_REQUEST_CONTEXT_GETTER_H
