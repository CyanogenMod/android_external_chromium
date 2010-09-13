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

#ifndef TALK_P2P_BASE_CONSTANTS_H_
#define TALK_P2P_BASE_CONSTANTS_H_

#include <string>
#include "talk/xmllite/qname.h"

// This file contains constants related to signaling that are used in various
// classes in this directory.

namespace cricket {

// There are 3 different types of Jingle messages or protocols: Jingle
// (the spec in XEP-166, etc), Gingle (the legacy protocol) and hybrid
// (both at the same time).  Gingle2 is a temporary protocol that we
// are only keeping around right now during this refactoring phase.
// Once we finish refactoring and start implementing Jingle, we will
// remove Gingle2.

// NS_ == namespace
// QN_ == buzz::QName (namespace + name)
// LN_ == "local name" == QName::LocalPart()
//   these are useful when you need to find a tag
//   that has different namespaces (like <description> or <transport>)

extern const std::string NS_EMPTY;
extern const std::string NS_JINGLE;
extern const std::string NS_GINGLE;

// TODO(pthatcher): remove GINGLE2 when we
// move to purely Jingle and Gingle protocols.
enum SignalingProtocol {
  PROTOCOL_JINGLE,
  PROTOCOL_GINGLE,
  PROTOCOL_GINGLE2,
  PROTOCOL_HYBRID,
};

// actions (aka Gingle <session> or Jingle <jingle>)
extern const buzz::QName QN_INITIATOR;
extern const buzz::QName QN_ACTION;

extern const buzz::QName QN_JINGLE_JINGLE;
extern const buzz::QName QN_JINGLE_CONTENT;
extern const std::string JINGLE_ACTION_SESSION_INITIATE;
extern const std::string JINGLE_ACTION_SESSION_INFO;
extern const std::string JINGLE_ACTION_SESSION_ACCEPT;
extern const std::string JINGLE_ACTION_SESSION_TERMINATE;
extern const std::string JINGLE_ACTION_TRANSPORT_INFO;

extern const buzz::QName QN_GINGLE_SESSION;
extern const std::string GINGLE_ACTION_INITIATE;
extern const std::string GINGLE_ACTION_INFO;
extern const std::string GINGLE_ACTION_ACCEPT;
extern const std::string GINGLE_ACTION_REJECT;
extern const std::string GINGLE_ACTION_TERMINATE;
extern const std::string GINGLE_ACTION_CANDIDATES;


// SessionFormats (aka Gingle <session><description>
//                  or Jingle <content><description>)
// For now, FormatDescription == SessionDescription
// Long term, everything will be FormatDescription
extern const std::string LN_DESCRIPTION;
extern const std::string LN_PAYLOADTYPE;
extern const buzz::QName QN_ID;
extern const buzz::QName QN_NAME;
extern const buzz::QName QN_CLOCKRATE;
extern const buzz::QName QN_BITRATE;
extern const buzz::QName QN_CHANNELS;
extern const buzz::QName QN_WIDTH;
extern const buzz::QName QN_HEIGHT;
extern const buzz::QName QN_FRAMERATE;
extern const buzz::QName QN_PARAMETER;
extern const std::string LN_NAME;
extern const std::string LN_VALUE;
extern const buzz::QName QN_PAYLOADTYPE_PARAMETER_NAME;
extern const buzz::QName QN_PAYLOADTYPE_PARAMETER_VALUE;
extern const std::string PAYLOADTYPE_PARAMETER_BITRATE;
extern const std::string PAYLOADTYPE_PARAMETER_HEIGHT;
extern const std::string PAYLOADTYPE_PARAMETER_WIDTH;
extern const std::string PAYLOADTYPE_PARAMETER_FRAMERATE;

extern const std::string NS_JINGLE_AUDIO;
extern const buzz::QName QN_JINGLE_AUDIO_FORMAT;
extern const buzz::QName QN_JINGLE_AUDIO_PAYLOADTYPE;
extern const std::string NS_JINGLE_VIDEO;
extern const buzz::QName QN_JINGLE_VIDEO_FORMAT;
extern const buzz::QName QN_JINGLE_VIDEO_PAYLOADTYPE;

extern const std::string NS_GINGLE_AUDIO;
extern const buzz::QName QN_GINGLE_AUDIO_FORMAT;
extern const buzz::QName QN_GINGLE_AUDIO_PAYLOADTYPE;
extern const buzz::QName QN_GINGLE_AUDIO_SRCID;
extern const std::string NS_GINGLE_VIDEO;
extern const buzz::QName QN_GINGLE_VIDEO_FORMAT;
extern const buzz::QName QN_GINGLE_VIDEO_PAYLOADTYPE;
extern const buzz::QName QN_GINGLE_VIDEO_SRCID;
extern const buzz::QName QN_GINGLE_VIDEO_BANDWIDTH;

// transports and candidates
extern const std::string NS_JINGLE_P2P;
extern const std::string LN_TRANSPORT;
extern const std::string LN_CANDIDATE;
extern const buzz::QName QN_JINGLE_P2P_TRANSPORT;
extern const buzz::QName QN_JINGLE_P2P_CANDIDATE;
extern const buzz::QName QN_UFRAG;
extern const buzz::QName QN_COMPONENT;
extern const buzz::QName QN_PWD;
extern const buzz::QName QN_IP;
extern const buzz::QName QN_PORT;
extern const buzz::QName QN_NETWORK;
extern const buzz::QName QN_GENERATION;
extern const buzz::QName QN_PRIORITY;
extern const buzz::QName QN_PROTOCOL;
extern const std::string JINGLE_CANDIDATE_TYPE_PEER_STUN;
extern const std::string JINGLE_CANDIDATE_TYPE_SERVER_STUN;
extern const std::string JINGLE_CANDIDATE_NAME_RTP;
extern const std::string JINGLE_CANDIDATE_NAME_RTCP;

extern const std::string NS_GINGLE_P2P;
extern const buzz::QName QN_GINGLE_P2P_TRANSPORT;
extern const buzz::QName QN_GINGLE2_P2P_CANDIDATE;
extern const buzz::QName QN_GINGLE_P2P_CANDIDATE;
extern const buzz::QName QN_GINGLE_P2P_UNKNOWN_CHANNEL_NAME;
extern const buzz::QName QN_GINGLE_CANDIDATE;
extern const buzz::QName QN_ADDRESS;
extern const buzz::QName QN_USERNAME;
extern const buzz::QName QN_PASSWORD;
extern const buzz::QName QN_PREFERENCE;
extern const std::string GINGLE_CANDIDATE_TYPE_STUN;
extern const std::string GINGLE_CANDIDATE_NAME_RTP;
extern const std::string GINGLE_CANDIDATE_NAME_RTCP;
extern const std::string GINGLE_CANDIDATE_NAME_VIDEO_RTP;
extern const std::string GINGLE_CANDIDATE_NAME_VIDEO_RTCP;

extern const std::string NS_GINGLE_RAW;
extern const buzz::QName QN_GINGLE_RAW_TRANSPORT;
extern const buzz::QName QN_GINGLE_RAW_CHANNEL;
extern const buzz::QName QN_GINGLE_RAW_BEHIND_SYM_NAT;
extern const buzz::QName QN_GINGLE_RAW_CAN_RECEIVE_FROM_SYM_NAT;

// old stuff
#ifdef FEATURE_ENABLE_VOICEMAIL
extern const std::string NS_VOICEMAIL;
extern const buzz::QName QN_VOICEMAIL_REGARDING;
#endif

}  // namespace cricket

#endif  // TALK_P2P_BASE_CONSTANTS_H_
