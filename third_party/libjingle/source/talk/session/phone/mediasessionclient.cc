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

MediaSessionDescription* MediaSessionClient::CreateOfferSessionDescription(
    bool video) {
  MediaSessionDescription* session_desc = new MediaSessionDescription();


  // add audio codecs
  std::vector<Codec> codecs;
  channel_manager_->GetSupportedCodecs(&codecs);
  for (std::vector<Codec>::const_iterator i = codecs.begin();
       i != codecs.end(); ++i)
    session_desc->voice().AddCodec(*i);

  // add video codecs, if this is a video call
  if (video) {
    std::vector<VideoCodec> video_codecs;
    channel_manager_->GetSupportedVideoCodecs(&video_codecs);
    for (std::vector<VideoCodec>::const_iterator i = video_codecs.begin();
         i != video_codecs.end(); i++)
      session_desc->video().AddCodec(*i);
  }

  session_desc->Sort();
  return session_desc;
}

MediaSessionDescription* MediaSessionClient::CreateAcceptSessionDescription(
  const SessionDescription* offer) {
  const MediaSessionDescription* offer_desc =
      static_cast<const MediaSessionDescription*>(offer);
  MediaSessionDescription* accept_desc = new MediaSessionDescription();

  // add audio codecs
  std::vector<Codec> codecs;
  channel_manager_->GetSupportedCodecs(&codecs);
  for (unsigned int i = 0; i < offer_desc->voice().codecs().size(); ++i) {
    if (channel_manager_->FindCodec(offer_desc->voice().codecs()[i]))
      accept_desc->voice().AddCodec(offer_desc->voice().codecs()[i]);
  }

  // add video codecs, if the incoming session description has them
  if (!offer_desc->video().codecs().empty()) {
    std::vector<VideoCodec> video_codecs;
    channel_manager_->GetSupportedVideoCodecs(&video_codecs);
    for (unsigned int i = 0; i < offer_desc->video().codecs().size(); ++i) {
      if (channel_manager_->FindVideoCodec(offer_desc->video().codecs()[i]))
        accept_desc->video().AddCodec(offer_desc->video().codecs()[i]);
    }
  }

  accept_desc->Sort();
  return accept_desc;
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

    Call *call = CreateCall(session->session_type() == NS_GINGLE_VIDEO);
    session_map_[session->id()] = call;
    call->AddSession(session);
  }
}

void MediaSessionClient::OnSessionState(BaseSession *session,
                                        BaseSession::State state) {
  if (state == Session::STATE_RECEIVEDINITIATE) {
    // If our accept would have no codecs, then we must reject this call.
    MediaSessionDescription* accept_desc =
        CreateAcceptSessionDescription(session->remote_description());
    if (accept_desc->voice().codecs().size() == 0) {
      // TODO(?): include an error description with the rejection.
      session->Reject();
    }
    delete accept_desc;
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
  assert(it != session_map_.end());
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
                                         Codec* out) {
  int id = GetXmlAttr(element, QN_ID, -1);
  if (id < 0)
    return false;

  std::string name = GetXmlAttr(element, QN_NAME, buzz::STR_EMPTY);
  int clockrate = GetXmlAttr(element, QN_CLOCKRATE, 0);
  int bitrate = GetXmlAttr(element, QN_BITRATE, 0);
  int channels = GetXmlAttr(element, QN_CHANNELS, 1);
  *out = Codec(id, name, clockrate, bitrate, channels, 0);
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

const FormatDescription* MediaSessionClient::ParseFormat(
    const buzz::XmlElement* element) {
  MediaSessionDescription* media = new MediaSessionDescription();
  // Includes payloads of unknown type (xmlns).  We need to know they
  // are there so that we don't auto-add old codecs unless there are
  // really no payload types, even of unknown type.
  bool has_payload_types = false;
  for (const buzz::XmlElement* payload_type = element->FirstElement();
       payload_type != NULL;
       payload_type = payload_type->NextElement()) {
    has_payload_types = true;
    const std::string& name = payload_type->Name().LocalPart();
    if (name == QN_GINGLE_AUDIO_PAYLOADTYPE.LocalPart() ||
        name == QN_GINGLE_VIDEO_PAYLOADTYPE.LocalPart()) {
      if (payload_type->Name() == QN_GINGLE_AUDIO_PAYLOADTYPE) {
        Codec acodec;
        if (ParseAudioCodec(payload_type, &acodec)) {
          media->voice().AddCodec(acodec);
        }
      } else if (payload_type->Name() == QN_GINGLE_VIDEO_PAYLOADTYPE) {
        VideoCodec vcodec;
        if (ParseVideoCodec(payload_type, &vcodec)) {
          media->video().AddCodec(vcodec);
        }
      }
    }
  }

  if (!has_payload_types) {
    // For backward compatibility, we can assume the other client is
    // an old version of Talk if it has no audio payload types at all.
    media->voice().AddCodec(Codec(103, "ISAC", 16000, -1, 1, 1));
    media->voice().AddCodec(Codec(0, "PCMU", 8000, 64000, 1, 0));
  }

  // get ssrcs, if present
  const buzz::XmlElement* src_id;
  src_id = element->FirstNamed(QN_GINGLE_AUDIO_SRCID);
  if (src_id) {
    media->voice().set_ssrc(strtoul(src_id->BodyText().c_str(),
        NULL, 10));
  }
  src_id = element->FirstNamed(QN_GINGLE_VIDEO_SRCID);
  if (src_id) {
    media->video().set_ssrc(strtoul(src_id->BodyText().c_str(),
        NULL, 10));
  }

  return media;
}

buzz::XmlElement* WriteAudioCodec(const Codec& codec) {
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

buzz::XmlElement* MediaSessionClient::WriteFormat(
    const FormatDescription* untyped_format) {
  const MediaSessionDescription* media =
      static_cast<const MediaSessionDescription*>(untyped_format);

  bool has_video_codecs = !media->video().codecs().empty();
  buzz::XmlElement* format_elem = new buzz::XmlElement(
      has_video_codecs ? QN_GINGLE_VIDEO_FORMAT
                       : QN_GINGLE_AUDIO_FORMAT, true);

  for (size_t i = 0; i < media->voice().codecs().size(); ++i) {
    format_elem->AddElement(WriteAudioCodec(media->voice().codecs()[i]));
  }
  for (size_t i = 0; i < media->video().codecs().size(); ++i) {
    format_elem->AddElement(WriteVideoCodec(media->video().codecs()[i]));
  }

  // Add ssrcs, if set.
  if (media->voice().ssrc_set()) {
    buzz::XmlElement* src_id =
        new buzz::XmlElement(QN_GINGLE_AUDIO_SRCID, true);
    if (media->voice().ssrc()) {
      SetXmlBody(src_id, media->voice().ssrc());
    }
    format_elem->AddElement(src_id);
  }
  if (has_video_codecs && media->video().ssrc_set()) {
    buzz::XmlElement* src_id =
        new buzz::XmlElement(QN_GINGLE_VIDEO_SRCID, true);
    if (media->video().ssrc()) {
      SetXmlBody(src_id, media->video().ssrc());
    }
    format_elem->AddElement(src_id);
  }


  return format_elem;
}

}  // namespace cricket
