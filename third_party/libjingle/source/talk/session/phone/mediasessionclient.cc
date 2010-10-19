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

#include "talk/session/phone/mediasessionclient.h"

#include "talk/base/logging.h"
#include "talk/base/stringutils.h"
#include "talk/p2p/base/constants.h"
#include "talk/p2p/base/parsing.h"
#include "talk/xmpp/constants.h"
#include "talk/xmllite/qname.h"

using namespace talk_base;

namespace cricket {

MediaSessionClient::MediaSessionClient(
    const buzz::Jid& jid, SessionManager *manager)
    : jid_(jid), session_manager_(manager), focus_call_(NULL),
      channel_manager_(new ChannelManager(session_manager_->worker_thread())) {
  Construct();
}

MediaSessionClient::MediaSessionClient(
    const buzz::Jid& jid, SessionManager *manager,
    MediaEngine* media_engine, DeviceManager* device_manager)
    : jid_(jid), session_manager_(manager), focus_call_(NULL),
      channel_manager_(new ChannelManager(
          media_engine, device_manager, session_manager_->worker_thread())) {
  Construct();
}


void MediaSessionClient::Construct() {
  // Register ourselves as the handler of phone and video sessions.
  session_manager_->AddClient(NS_GINGLE_AUDIO, this);
  session_manager_->AddClient(NS_GINGLE_VIDEO, this);
  // Forward device notifications.
  SignalDevicesChange.repeat(channel_manager_->SignalDevicesChange);
  // Bring up the channel manager.
  // In previous versions of ChannelManager, this was done automatically
  // in the constructor.
  channel_manager_->Init();
}

MediaSessionClient::~MediaSessionClient() {
  // Destroy all calls
  std::map<uint32, Call *>::iterator it;
  while (calls_.begin() != calls_.end()) {
    std::map<uint32, Call *>::iterator it = calls_.begin();
    DestroyCall((*it).second);
  }

  // Delete channel manager. This will wait for the channels to exit
  delete channel_manager_;

  // Remove ourselves from the client map.
  session_manager_->RemoveClient(NS_GINGLE_VIDEO);
  session_manager_->RemoveClient(NS_GINGLE_AUDIO);
}

SessionDescription* MediaSessionClient::CreateOffer(bool video, bool set_ssrc) {
  SessionDescription* offer = new SessionDescription();
  AudioContentDescription* audio = new AudioContentDescription();


  AudioCodecs audio_codecs;
  channel_manager_->GetSupportedAudioCodecs(&audio_codecs);
  for (AudioCodecs::const_iterator codec = audio_codecs.begin();
       codec != audio_codecs.end(); codec++) {
    audio->AddCodec(*codec);
  }
  if (set_ssrc) {
    audio->set_ssrc(0);
  }
  audio->SortCodecs();
  offer->AddContent(CN_AUDIO, NS_GINGLE_AUDIO, audio);

  // add video codecs, if this is a video call
  if (video) {
    VideoContentDescription* video = new VideoContentDescription();
    VideoCodecs video_codecs;
    channel_manager_->GetSupportedVideoCodecs(&video_codecs);
    for (VideoCodecs::const_iterator codec = video_codecs.begin();
         codec != video_codecs.end(); codec++) {
      video->AddCodec(*codec);
    }
    if (set_ssrc) {
      video->set_ssrc(0);
    }
    video->SortCodecs();
    offer->AddContent(CN_VIDEO, NS_GINGLE_VIDEO, video);
  }

  return offer;
}

const ContentInfo* GetFirstMediaContent(const SessionDescription* sdesc,
                                        const std::string& content_type) {
  if (sdesc == NULL)
    return NULL;

  return sdesc->FirstContentByType(content_type);
}

const ContentInfo* GetFirstAudioContent(const SessionDescription* sdesc) {
  return GetFirstMediaContent(sdesc, NS_GINGLE_AUDIO);
}

const ContentInfo* GetFirstVideoContent(const SessionDescription* sdesc) {
  return GetFirstMediaContent(sdesc, NS_GINGLE_VIDEO);
}

SessionDescription* MediaSessionClient::CreateAnswer(
    const SessionDescription* offer) {
  SessionDescription* accept = new SessionDescription();

  const ContentInfo* audio_content = GetFirstAudioContent(offer);
  if (audio_content) {
    const AudioContentDescription* audio_offer =
        static_cast<const AudioContentDescription*>(audio_content->description);
    AudioContentDescription* audio_accept = new AudioContentDescription();
    for (AudioCodecs::const_iterator codec = audio_offer->codecs().begin();
         codec != audio_offer->codecs().end(); codec++) {
      if (channel_manager_->FindAudioCodec(*codec)) {
        audio_accept->AddCodec(*codec);
      }
    }
    audio_accept->SortCodecs();
    accept->AddContent(audio_content->name, audio_content->type, audio_accept);
  }

  const ContentInfo* video_content = GetFirstVideoContent(offer);
  if (video_content) {
    const VideoContentDescription* video_offer =
        static_cast<const VideoContentDescription*>(video_content->description);
    VideoContentDescription* video_accept = new VideoContentDescription();
    for (VideoCodecs::const_iterator codec = video_offer->codecs().begin();
         codec != video_offer->codecs().end(); codec++) {
      if (channel_manager_->FindVideoCodec(*codec)) {
        video_accept->AddCodec(*codec);
      }
    }
    video_accept->SortCodecs();
    accept->AddContent(video_content->name, video_content->type, video_accept);
  }

  return accept;
}

Call *MediaSessionClient::CreateCall(bool video, bool mux) {
  Call *call = new Call(this, video, mux);
  calls_[call->id()] = call;
  SignalCallCreate(call);
  return call;
}

void MediaSessionClient::OnSessionCreate(Session *session,
                                         bool received_initiate) {
  if (received_initiate) {
    session->SignalState.connect(this, &MediaSessionClient::OnSessionState);

    Call *call = CreateCall(session->content_type() == NS_GINGLE_VIDEO);
    session_map_[session->id()] = call;
    call->AddSession(session);
  }
}

void MediaSessionClient::OnSessionState(BaseSession *session,
                                        BaseSession::State state) {
  if (state == Session::STATE_RECEIVEDINITIATE) {
    // If our accept would have no codecs, then we must reject this call.
    SessionDescription* accept = CreateAnswer(session->remote_description());
    const ContentInfo* audio_content = GetFirstAudioContent(accept);
    const AudioContentDescription* audio_accept = (!audio_content) ? NULL :
        static_cast<const AudioContentDescription*>(audio_content->description);

    if (!audio_accept || audio_accept->codecs().size() == 0) {
      // TODO(?): include an error description with the rejection.
      session->Reject();
    }
    delete accept;
  }
}

void MediaSessionClient::DestroyCall(Call *call) {
  // Change focus away, signal destruction

  if (call == focus_call_)
    SetFocus(NULL);
  SignalCallDestroy(call);

  // Remove it from calls_ map and delete

  std::map<uint32, Call *>::iterator it = calls_.find(call->id());
  if (it != calls_.end())
    calls_.erase(it);

  delete call;
}

void MediaSessionClient::OnSessionDestroy(Session *session) {
  // Find the call this session is in, remove it

  std::map<SessionID, Call *>::iterator it = session_map_.find(session->id());
  ASSERT(it != session_map_.end());
  if (it != session_map_.end()) {
    Call *call = (*it).second;
    session_map_.erase(it);
    call->RemoveSession(session);
  }
}

Call *MediaSessionClient::GetFocus() {
  return focus_call_;
}

void MediaSessionClient::SetFocus(Call *call) {
  Call *old_focus_call = focus_call_;
  if (focus_call_ != call) {
    if (focus_call_ != NULL)
      focus_call_->EnableChannels(false);
    focus_call_ = call;
    if (focus_call_ != NULL)
      focus_call_->EnableChannels(true);
    SignalFocus(focus_call_, old_focus_call);
  }
}

void MediaSessionClient::JoinCalls(Call *call_to_join, Call *call) {
  // Move all sessions from call to call_to_join, delete call.
  // If call_to_join has focus, added sessions should have enabled channels.

  if (focus_call_ == call)
    SetFocus(NULL);
  call_to_join->Join(call, focus_call_ == call_to_join);
  DestroyCall(call);
}

Session *MediaSessionClient::CreateSession(Call *call) {
  const std::string& type = call->video() ? NS_GINGLE_VIDEO : NS_GINGLE_AUDIO;
  Session *session = session_manager_->CreateSession(jid().Str(), type);
  session_map_[session->id()] = call;
  return session;
}

bool MediaSessionClient::ParseAudioCodec(const buzz::XmlElement* element,
                                         AudioCodec* out) {
  int id = GetXmlAttr(element, QN_ID, -1);
  if (id < 0)
    return false;

  std::string name = GetXmlAttr(element, QN_NAME, buzz::STR_EMPTY);
  int clockrate = GetXmlAttr(element, QN_CLOCKRATE, 0);
  int bitrate = GetXmlAttr(element, QN_BITRATE, 0);
  int channels = GetXmlAttr(element, QN_CHANNELS, 1);
  *out = AudioCodec(id, name, clockrate, bitrate, channels, 0);
  return true;
}

bool MediaSessionClient::ParseVideoCodec(const buzz::XmlElement* element,
                                         VideoCodec* out) {
  int id = GetXmlAttr(element, QN_ID, -1);
  if (id < 0)
    return false;

  std::string name = GetXmlAttr(element, QN_NAME, buzz::STR_EMPTY);
  int width = GetXmlAttr(element, QN_WIDTH, 0);
  int height = GetXmlAttr(element, QN_HEIGHT, 0);
  int framerate = GetXmlAttr(element, QN_FRAMERATE, 0);
  *out = VideoCodec(id, name, width, height, framerate, 0);
  return true;
}

bool MediaSessionClient::ParseContent(const buzz::XmlElement* elem,
                                      const ContentDescription** content,
                                      ParseError* error) {
  const std::string& content_type = elem->Name().Namespace();
  if (NS_GINGLE_AUDIO == content_type) {
    AudioContentDescription* audio = new AudioContentDescription();

    if (elem->FirstElement()) {
      for (const buzz::XmlElement* codec_elem =
               elem->FirstNamed(QN_GINGLE_AUDIO_PAYLOADTYPE);
           codec_elem != NULL;
           codec_elem = codec_elem->NextNamed(QN_GINGLE_AUDIO_PAYLOADTYPE)) {
        AudioCodec audio_codec;
        if (ParseAudioCodec(codec_elem, &audio_codec)) {
          audio->AddCodec(audio_codec);
        }
      }
    } else {
      // For backward compatibility, we can assume the other client is
      // an old version of Talk if it has no audio payload types at all.
      audio->AddCodec(AudioCodec(103, "ISAC", 16000, -1, 1, 1));
      audio->AddCodec(AudioCodec(0, "PCMU", 8000, 64000, 1, 0));
    }

    const buzz::XmlElement* src_id = elem->FirstNamed(QN_GINGLE_AUDIO_SRCID);
    if (src_id) {
      audio->set_ssrc(strtoul(src_id->BodyText().c_str(), NULL, 10));
    }

    *content = audio;
  } else if (NS_GINGLE_VIDEO == content_type) {
    VideoContentDescription* video = new VideoContentDescription();

    for (const buzz::XmlElement* codec_elem =
             elem->FirstNamed(QN_GINGLE_VIDEO_PAYLOADTYPE);
         codec_elem != NULL;
         codec_elem = codec_elem->NextNamed(QN_GINGLE_VIDEO_PAYLOADTYPE)) {
      VideoCodec video_codec;
      if (ParseVideoCodec(codec_elem, &video_codec)) {
        video->AddCodec(video_codec);
      }
    }

    const buzz::XmlElement* src_id = elem->FirstNamed(QN_GINGLE_VIDEO_SRCID);
    if (src_id) {
      video->set_ssrc(strtoul(src_id->BodyText().c_str(), NULL, 10));
    }

    *content = video;
  } else {
    return BadParse("unknown media content type: " + content_type, error);
  }

  return true;
}

buzz::XmlElement* WriteAudioCodec(const AudioCodec& codec) {
  buzz::XmlElement* payload_type =
      new buzz::XmlElement(QN_GINGLE_AUDIO_PAYLOADTYPE, true);
  AddXmlAttr(payload_type, QN_ID, codec.id);
  payload_type->AddAttr(QN_NAME, codec.name);
  if (codec.clockrate > 0)
    AddXmlAttr(payload_type, QN_CLOCKRATE, codec.clockrate);
  if (codec.bitrate > 0)
    AddXmlAttr(payload_type, QN_BITRATE, codec.bitrate);
  if (codec.channels > 1)
    AddXmlAttr(payload_type, QN_CHANNELS, codec.channels);
  return payload_type;
}

buzz::XmlElement* WriteVideoCodec(const VideoCodec& codec) {
  buzz::XmlElement* payload_type =
      new buzz::XmlElement(QN_GINGLE_VIDEO_PAYLOADTYPE, true);
  AddXmlAttr(payload_type, QN_ID, codec.id);
  payload_type->AddAttr(QN_NAME, codec.name);
  AddXmlAttr(payload_type, QN_WIDTH, codec.width);
  AddXmlAttr(payload_type, QN_HEIGHT, codec.height);
  AddXmlAttr(payload_type, QN_FRAMERATE, codec.framerate);
  return payload_type;
}

bool MediaSessionClient::WriteContent(const ContentDescription* untyped_content,
                                      buzz::XmlElement** elem,
                                      WriteError* error) {
  const MediaContentDescription* media =
      static_cast<const MediaContentDescription*>(untyped_content);

  buzz::XmlElement* content_elem;
  if (media->type() == MEDIA_TYPE_AUDIO) {
    const AudioContentDescription* audio =
        static_cast<const AudioContentDescription*>(untyped_content);
    content_elem = new buzz::XmlElement(QN_GINGLE_AUDIO_CONTENT, true);

    for (AudioCodecs::const_iterator codec = audio->codecs().begin();
         codec != audio->codecs().end(); codec++) {
      content_elem->AddElement(WriteAudioCodec(*codec));
    }
    if (audio->ssrc_set()) {
      buzz::XmlElement* src_id =
          new buzz::XmlElement(QN_GINGLE_AUDIO_SRCID, true);
      if (audio->ssrc()) {
        SetXmlBody(src_id, audio->ssrc());
      }
      content_elem->AddElement(src_id);
    }


    *elem = content_elem;
  } else if (media->type() == MEDIA_TYPE_VIDEO) {
    const VideoContentDescription* video =
        static_cast<const VideoContentDescription*>(untyped_content);
    content_elem = new buzz::XmlElement(QN_GINGLE_VIDEO_CONTENT, true);

    for (VideoCodecs::const_iterator codec = video->codecs().begin();
         codec != video->codecs().end(); codec++) {
      content_elem->AddElement(WriteVideoCodec(*codec));
    }
    if (video->ssrc_set()) {
      buzz::XmlElement* src_id =
          new buzz::XmlElement(QN_GINGLE_VIDEO_SRCID, true);
      if (video->ssrc()) {
        SetXmlBody(src_id, video->ssrc());
      }
      content_elem->AddElement(src_id);
    }

    *elem = content_elem;
  }
  return true;
}

}  // namespace cricket
