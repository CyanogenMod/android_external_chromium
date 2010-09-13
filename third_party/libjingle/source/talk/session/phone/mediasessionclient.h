/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_SESSION_PHONE_MEDIASESSIONCLIENT_H_
#define TALK_SESSION_PHONE_MEDIASESSIONCLIENT_H_

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "talk/session/phone/call.h"
#include "talk/session/phone/channelmanager.h"
#include "talk/session/phone/cryptoparams.h"
#include "talk/base/sigslot.h"
#include "talk/base/sigslotrepeater.h"
#include "talk/base/messagequeue.h"
#include "talk/base/thread.h"
#include "talk/p2p/base/sessionmanager.h"
#include "talk/p2p/base/session.h"
#include "talk/p2p/base/sessionclient.h"
#include "talk/p2p/base/sessiondescription.h"

namespace cricket {

class Call;
class MediaSessionDescription;

class MediaSessionClient: public SessionClient, public sigslot::has_slots<> {
 public:
  MediaSessionClient(const buzz::Jid& jid, SessionManager *manager);
  // Alternative constructor, allowing injection of media_engine
  // and device_manager.
  MediaSessionClient(const buzz::Jid& jid, SessionManager *manager,
      MediaEngine* media_engine, DeviceManager* device_manager);
  ~MediaSessionClient();

  const buzz::Jid &jid() const { return jid_; }
  SessionManager* session_manager() const { return session_manager_; }
  ChannelManager* channel_manager() const { return channel_manager_; }

  int GetCapabilities() { return channel_manager_->GetCapabilities(); }

  Call *CreateCall(bool video = false, bool mux = false);
  void DestroyCall(Call *call);

  Call *GetFocus();
  void SetFocus(Call *call);

  void JoinCalls(Call *call_to_join, Call *call);

  bool GetAudioInputDevices(std::vector<std::string>* names) {
    return channel_manager_->GetAudioInputDevices(names);
  }
  bool GetAudioOutputDevices(std::vector<std::string>* names) {
    return channel_manager_->GetAudioOutputDevices(names);
  }
  bool GetVideoCaptureDevices(std::vector<std::string>* names) {
    return channel_manager_->GetVideoCaptureDevices(names);
  }

  bool SetAudioOptions(const std::string& in_name, const std::string& out_name,
                       int opts) {
    return channel_manager_->SetAudioOptions(in_name, out_name, opts);
  }
  bool SetOutputVolume(int level) {
    return channel_manager_->SetOutputVolume(level);
  }
  bool SetVideoOptions(const std::string& cam_device) {
    return channel_manager_->SetVideoOptions(cam_device);
  }

  sigslot::signal2<Call *, Call *> SignalFocus;
  sigslot::signal1<Call *> SignalCallCreate;
  sigslot::signal1<Call *> SignalCallDestroy;
  sigslot::repeater0<> SignalDevicesChange;

  MediaSessionDescription* CreateOfferSessionDescription(bool video = false);
  MediaSessionDescription* CreateAcceptSessionDescription(
      const SessionDescription* offer);

 private:
  void Construct();
  void OnSessionCreate(Session *session, bool received_initiate);
  void OnSessionState(BaseSession *session, BaseSession::State state);
  void OnSessionDestroy(Session *session);
  virtual const FormatDescription* ParseFormat(const buzz::XmlElement* element);
  virtual buzz::XmlElement* WriteFormat(const FormatDescription* format);
  Session *CreateSession(Call *call);
  static bool ParseAudioCodec(const buzz::XmlElement* element, Codec* out);
  static bool ParseVideoCodec(const buzz::XmlElement* element, VideoCodec* out);



  buzz::Jid jid_;
  SessionManager* session_manager_;
  Call *focus_call_;
  ChannelManager *channel_manager_;
  std::map<uint32, Call *> calls_;
  std::map<SessionID, Call *> session_map_;

  friend class Call;
};

// Parameters for a voice and/or video session.
class MediaSessionDescription : public SessionDescription {
 public:
  // Base class with common options.
  struct Content {
    Content() : ssrc_(0), ssrc_set_(false), rtcp_mux_(false),
                rtp_headers_disabled_(false) {}

    uint32 ssrc() const { return ssrc_; }
    bool ssrc_set() const { return ssrc_set_; }
    void set_ssrc(uint32 ssrc) {
      ssrc_ = ssrc;
      ssrc_set_ = true;
    }

    bool rtcp_mux() const { return rtcp_mux_; }
    void set_rtcp_mux(bool mux) { rtcp_mux_ = mux; }

    bool rtp_headers_disabled() const {
      return rtp_headers_disabled_;
    }
    void set_rtp_headers_disabled(bool disable) {
      rtp_headers_disabled_ = disable;
    }

    const std::vector<CryptoParams>& cryptos() const { return cryptos_; }
    void AddCrypto(const CryptoParams& params) {
      cryptos_.push_back(params);
    }

    template <class T> struct PreferenceSort {
      bool operator()(T a, T b) { return a.preference > b.preference; }
    };

    uint32 ssrc_;
    bool ssrc_set_;
    bool rtcp_mux_;
    bool rtp_headers_disabled_;
    std::vector<CryptoParams> cryptos_;
  };
  // Voice-specific options.
  struct VoiceContent : public Content {
    const std::vector<Codec>& codecs() const { return codecs_; }
    void AddCodec(const Codec& codec) {
      codecs_.push_back(codec);
    }
    void SortCodecs() {
      std::sort(codecs_.begin(), codecs_.end(), PreferenceSort<Codec>());
    }
    std::vector<Codec> codecs_;
  };
  // Video-specific options.
  struct VideoContent : public Content {
    const std::vector<VideoCodec>& codecs() const { return codecs_; }
    void AddCodec(const VideoCodec& codec) {
      codecs_.push_back(codec);
    }
    void SortCodecs() {
      std::sort(codecs_.begin(), codecs_.end(), PreferenceSort<VideoCodec>());
    }
    std::vector<VideoCodec> codecs_;
  };

  const VoiceContent& voice() const { return voice_; }
  VoiceContent& voice() { return voice_; }
  const VideoContent& video() const { return video_; }
  VideoContent& video() { return video_; }

  void Sort() {
    voice_.SortCodecs();
    video_.SortCodecs();
  }

  const std::string &lang() const { return lang_; }
  void set_lang(const std::string &lang) { lang_ = lang; }

 private:
  VoiceContent voice_;
  VideoContent video_;
  std::string lang_;
};

}  // namespace cricket

#endif  // TALK_SESSION_PHONE_MEDIASESSIONCLIENT_H_
