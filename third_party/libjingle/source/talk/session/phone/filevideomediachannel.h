// Copyright 2008 Google Inc. All Rights Reserved.
// Author: pzembrod@google.com (Philip Zembrod)

// FileVideoMediaChannel is the worker channel for FileVideoEngine,
// designed for the MediaEngine mechanism in Talk Video clients
// (see comments at top of file filevideoengine.h).
// FileVideoMediaChannel takes care of the actual transmitting
// and receiving of video streams over rtp and the reading
// from and recording to files. FileVideoMediaChannel is created
// by FileVideoEngine::CreateChannel().
// rtp (real time protocol) is a udp protocol used (here) to transfer
// audio and video streams.
// rtcp (real time control protocol), also udp-based, is an associated
// control protocol.

#ifndef TALK_SESSION_PHONE_FILEVIDEOMEDIACHANNEL_H_
#define TALK_SESSION_PHONE_FILEVIDEOMEDIACHANNEL_H_

#include <string>
#include <vector>

#include "talk/base/criticalsection.h"
#include "talk/base/stream.h"  // for StreamResult
#include "talk/session/phone/mediachannel.h"

using talk_base::CritScope;
using talk_base::FileStream;

namespace cricket {

class FileVideoEngine;

// See comment at top of file
class FileVideoMediaChannel : public VideoMediaChannel {
 public:
  // The creating engine is used to provide the recording file base name.
  explicit FileVideoMediaChannel(FileVideoEngine* engine);
  virtual ~FileVideoMediaChannel();
  // FileVideoMediaChannel can run without Init(), but then it doesn't record.
  // Calling Init() again restarts recording to engine's current file name.
  // If the file name has changed since the last Init() call, the old files
  // are closed and new files (one for rtp, one for rtcp) opened.
  virtual void Init();

  virtual bool SetOptions(int options) {
    return true;
  }
  virtual bool SetRecvCodecs(const std::vector<VideoCodec>& codecs) {
    return true;
  }

  // This is called after initiating or accepting a call
  // with the possible codecs of a session. The channel has to
  // choose one it can handle and inform the session of the choice.
  virtual bool SetSendCodecs(const std::vector<VideoCodec>& codecs);

  // Called on receiving a rtp packet. packet_data points to a buffer
  // containing the packet's bytes, packet_length is the buffer's length.
  virtual void OnPacketReceived(const void* packet_data, int packet_length);
  // Called on receiving a rtcp packet. packet_data points to a buffer
  // containing the packet's bytes, packet_length is the buffer's length.
  virtual void OnRtcpReceived(const void* packet_data, int packet_length);

  // Remaining pure virtual methods of MediaChannel which are
  // not overridden in VideoMediaChannel are overridden with
  // empty implementations here to be able to run the class.
  // Most or all of them will doubtless be filled out with
  // useful code later.
  virtual bool SetRenderer(uint32 ssrc, VideoRenderer* r) { return true; }
  virtual bool SetRender(bool render) { return true; }
  virtual bool AddScreencast(uint32 ssrc, talk_base::WindowId id) {
    return true;
  }
  virtual bool RemoveScreencast(uint32 ssrc) { return true; }
  virtual bool SetSend(bool send) { return true; }
  virtual void SetSendSsrc(uint32 id) {}
  virtual bool SetMaxSendBandwidth(int max_bandwidth) { return false; }
  virtual bool AddStream(uint32 ssrc, uint32 voice_ssrc) { return false; }
  virtual bool RemoveStream(uint32 ssrc) { return false; }
  virtual bool Mute(bool muted) { return false; }
  virtual bool GetStats(VideoMediaInfo* info) { return false; }

  // Builds the file name for rtp recording.
  // public for testing.
  std::string rtp_filename() const;
  // Builds the file name for rtcp recording.
  // public for testing.
  std::string rtcp_filename() const;

 private:
  // Common inner function for rtp_filename() and rtcp_filename(). Does
  // all the work except for the extensions ".rtp"/".rtcp".
  std::string file_name() const;

  // Creates and opens rtp_writer_ for writing (mode "w") into file
  // rtp_filename().
  void OpenRtpFile();
  // Creates and opens rtcp_writer_ for writing (mode "w") into file
  // rtcp_filename().
  void OpenRtcpFile();

  // Closes and deletes rtp_writer_.
  void CloseRtpFile();
  // Closes and deletes rtcp_writer_.
  void CloseRtcpFile();

  // Helper function for OpenRtpFile() and OpenRtcpFile().
  // Creates, opens and returns FileStream object.
  // Params as in FileStream::Open(). Returns NULL on failure.
  FileStream* OpenFileStream(const std::string& file_name, const char* mode);

  // Helper function for OnPacketReceived() and OnRtcpReceived().
  // Writes rtp or rtcp packet in packet_data/packet_length to writer.
  // FileStream *writer my be NULL, then nothing is written.
  // protocol_name should be "rtp" or "rtcp" and is used in
  // logging messages only.
  void WritePacket(FileStream* writer, const std::string& protocol_name,
                   const void* packet_data, int packet_length);

  // Helper method for WritePacket(). It's a wrapper around
  // FileStream::Write() with error logging and returns the
  // return value of FileStream::Write(), SR_SUCCESS on success.
  // *what_is_written is used for the error message and should be
  // something like "rtp packet size" or "rtp packet data".
  // writer must not be NULL.
  talk_base::StreamResult WriteAndCheck(FileStream* writer,
      const std::string& what_is_written, const void* data, int length);

  // Engine creating FileVideoMediaChannel instance, used to
  // provide file_base_name() for recording.
  FileVideoEngine* engine_;
  // FileStream for recording rtp packages
  talk_base::scoped_ptr<FileStream> rtp_writer_;
  // FileStream for recording rtcp packages
  talk_base::scoped_ptr<FileStream> rtcp_writer_;

  // ID  to create unique filenames for several instances
  // which might exist in parallel.
  int id_;
  // Static counter to create these IDs.
  static int id_generator_;
  // Mutex for id_generator_.
  static talk_base::CriticalSection cs_id_generator_;
  DISALLOW_EVIL_CONSTRUCTORS(FileVideoMediaChannel);
};

}  // namespace cricket

#endif  // TALK_SESSION_PHONE_FILEVIDEOMEDIACHANNEL_H_
