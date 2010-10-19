// Copyright 2008 Google Inc. All Rights Reserved.
// Author: pzembrod@google.com (Philip Zembrod)

// FileVideoEngine shall be used to spool received video into files and to
// transmit prerecorded video from files. It is intended for use with
// call.exe clients and derived bots for 1:1 and n:n TalkVideo testing.
// Having a client without actual connection to media hardware allows to
// run several of them on one machine, together withone actual browser
// client, for single-handed manual testing and single-browser automatic
// testing.
// Currently FileVideoEngine records into file names like "video_1.rtp" or
// "somebasename_video_5.rtp". The base name can be set in the engine.
// The auxiliary rtcp streams are recorded into files with the extension
// ".rtcp".

// FileVideoEngine is designed for the MediaEngine mechanism
// in Talk Video clients, to replace LmiVideoEngine
// in a constructor call like
//   return new CompositeMediaEngine<GipsVoiceEngine, LmiVideoEngine>;
// Eventually together with a FileAudioEngine, a file based media engine
// shall be constructed with
//   return new CompositeMediaEngine<FileAudioEngine, FileVideoEngine>;
// While FileAudioEngine is not available, FileVideoEngine can be combined
// with GipsVoiceEngine:
//   return new CompositeMediaEngine<GipsVoiceEngine, FileVideoEngine>;
// One of these calls is performed by the static method
//   MediaEngine* MediaEngine::Create()
// depending on compiler flags or through dependency injection of a
// factory.

#ifndef TALK_SESSION_PHONE_FILEVIDEOENGINE_H__
#define TALK_SESSION_PHONE_FILEVIDEOENGINE_H__

#include <string>
#include <vector>

// basictypes.h is included for DISALLOW_EVIL_CONSTRUCTORS
#include "talk/base/basictypes.h"
// codec.h is included because std::vector<VideoCodec> must know
// VideoCodec's size
#include "talk/session/phone/codec.h"
// filevideomediachannel.h is included because in mediaengine.cc
// when compiling CompositeMediaEngine the compiler must know that
// FileVideoMediaChannel* FileVideoEngine::CreateChannel(...)
// returns a class derived from VideoMediaChannel.
#include "talk/session/phone/filevideomediachannel.h"
// Using CaptureResult enumeration from mediaengine.h.
#include "talk/session/phone/mediaengine.h"

// Transmitting video is harder and will be implemented when
// receiving and recording works.
// TODO(pzembrod): remove all ifdef REPLAY_IMPLEMENTED once
// transmitting is implemented and tested.
#undef REPLAY_IMPLEMENTED

namespace cricket {

struct Device;
class VideoRenderer;
class VoiceMediaChannel;

// See comment at top of file
class FileVideoEngine {
 public:
  FileVideoEngine();
  virtual ~FileVideoEngine() {}
  // Init() is called by virtual bool CompositeMediaEngine::Init().
  bool Init();

  // Accessors for file_base_name_.
  // The getter is virtual to allow mocking it out.
  void set_file_base_name(const std::string& file_base_name);
  virtual const std::string& file_base_name() const;

  // This creates the actual worker, a FileVideoMediaChannel instance.
  FileVideoMediaChannel* CreateChannel(VoiceMediaChannel* voice_media_channel);

  // All the following methods are called by corresponding
  // virtual methods of CompositeMediaEngine and don't do
  // anything yet.
  void GetCaptureDeviceNames(std::vector<std::string>* names);
  bool SetCaptureDevice(const Device* device);
  bool SetCaptureFormat(int width, int height, int framerate);
  void Terminate() {}
  int GetCapabilities() { return 0; }
  bool SetOptions(int opts) { return true; }
  bool SetLocalRenderer(VideoRenderer* renderer) { return true; }
  CaptureResult SetCapture(bool capture) { return CR_SUCCESS;  }
  const std::vector<VideoCodec>& codecs();
  bool FindCodec(const VideoCodec& codec);
  bool SetDefaultEncoderConfig(const VideoEncoderConfig& config);
  void SetLogging(int severity, const char* filter) {}
  sigslot::signal1<bool> SignalCaptureResult;

 private:
  // The engine just supports one capture device; actual name's not
  // important, behind it are recorded files, of course.
  static const char* const kFileCaptureDeviceName;

  // The codec in which the recording currently available for replay
  // is encoded.
  VideoCodec replay_codec_;

  // List of supported codecs. Published by codec(). Used by FindCodec.
  // Since we could record any encoding we might like some generic
  // flexibility here. Might be overkill, though.
  std::vector<VideoCodec> codecs_;

  // Recording file base name for FileVideoMediaChannel.
  std::string file_base_name_;
  DISALLOW_EVIL_CONSTRUCTORS(FileVideoEngine);
};

}  // namespace cricket

#endif  // TALK_SESSION_PHONE_FILEVIDEOENGINE_H__
