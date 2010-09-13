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

#include "talk/p2p/base/constants.h"

namespace cricket {


const std::string NS_EMPTY("");
const std::string NS_JINGLE("urn:xmpp:jingle:1");
const std::string NS_GINGLE("http://www.google.com/session");

// actions (aka <session> or <jingle>)
const buzz::QName QN_INITIATOR(true, NS_EMPTY, "initiator");
const buzz::QName QN_ACTION(true, NS_EMPTY, "action");

const buzz::QName QN_JINGLE_JINGLE(true, NS_JINGLE, "jingle");
const buzz::QName QN_JINGLE_CONTENT(true, NS_JINGLE, "content");
const std::string JINGLE_ACTION_SESSION_INITIATE("session-initiate");
const std::string JINGLE_ACTION_SESSION_INFO("session-info");
const std::string JINGLE_ACTION_SESSION_ACCEPT("session-accept");
const std::string JINGLE_ACTION_SESSION_TERMINATE("session-terminate");
const std::string JINGLE_ACTION_TRANSPORT_INFO("transport-info");

const buzz::QName QN_GINGLE_SESSION(true, NS_GINGLE, "session");
const std::string GINGLE_ACTION_INITIATE("initiate");
const std::string GINGLE_ACTION_INFO("info");
const std::string GINGLE_ACTION_ACCEPT("accept");
const std::string GINGLE_ACTION_REJECT("reject");
const std::string GINGLE_ACTION_TERMINATE("terminate");
const std::string GINGLE_ACTION_CANDIDATES("candidates");

// SessionApps (aka Gingle <session><description>
//              or Jingle <content><description>)
const std::string LN_DESCRIPTION("description");
const std::string LN_PAYLOADTYPE("payload-type");
const buzz::QName QN_ID(true, NS_EMPTY, "id");
const buzz::QName QN_NAME(true, NS_EMPTY, "name");
const buzz::QName QN_CLOCKRATE(true, NS_EMPTY, "clockrate");
const buzz::QName QN_BITRATE(true, NS_EMPTY, "bitrate");
const buzz::QName QN_CHANNELS(true, NS_EMPTY, "channels");
const buzz::QName QN_WIDTH(true, NS_EMPTY, "width");
const buzz::QName QN_HEIGHT(true, NS_EMPTY, "height");
const buzz::QName QN_FRAMERATE(true, NS_EMPTY, "framerate");
const buzz::QName QN_PARAMETER(true, NS_EMPTY, "parameter");
const std::string LN_NAME("name");
const std::string LN_VALUE("value");
const buzz::QName QN_PAYLOADTYPE_PARAMETER_NAME(true, NS_EMPTY, LN_NAME);
const buzz::QName QN_PAYLOADTYPE_PARAMETER_VALUE(true, NS_EMPTY, LN_VALUE);
const std::string PAYLOADTYPE_PARAMETER_BITRATE("bitrate");
const std::string PAYLOADTYPE_PARAMETER_HEIGHT("height");
const std::string PAYLOADTYPE_PARAMETER_WIDTH("width");
const std::string PAYLOADTYPE_PARAMETER_FRAMERATE("framerate");


const std::string NS_JINGLE_AUDIO("urn:xmpp:jingle:apps:rtp:audio");
const buzz::QName QN_JINGLE_AUDIO_FORMAT(
    true, NS_JINGLE_AUDIO, LN_DESCRIPTION);
const buzz::QName QN_JINGLE_AUDIO_PAYLOADTYPE(
    true, NS_JINGLE_AUDIO, LN_PAYLOADTYPE);
const std::string NS_JINGLE_VIDEO("urn:xmpp:jingle:apps:rtp:video");
const buzz::QName QN_JINGLE_VIDEO_FORMAT(
    true, NS_JINGLE_VIDEO, LN_DESCRIPTION);
const buzz::QName QN_JINGLE_VIDEO_PAYLOADTYPE(
    true, NS_JINGLE_VIDEO, LN_PAYLOADTYPE);

const std::string NS_GINGLE_AUDIO("http://www.google.com/session/phone");
const buzz::QName QN_GINGLE_AUDIO_FORMAT(
    true, NS_GINGLE_AUDIO, LN_DESCRIPTION);
const buzz::QName QN_GINGLE_AUDIO_PAYLOADTYPE(
    true, NS_GINGLE_AUDIO, LN_PAYLOADTYPE);
const buzz::QName QN_GINGLE_AUDIO_SRCID(true, NS_GINGLE_AUDIO, "src-id");
const std::string NS_GINGLE_VIDEO("http://www.google.com/session/video");
const buzz::QName QN_GINGLE_VIDEO_FORMAT(
    true, NS_GINGLE_VIDEO, LN_DESCRIPTION);
const buzz::QName QN_GINGLE_VIDEO_PAYLOADTYPE(
    true, NS_GINGLE_VIDEO, LN_PAYLOADTYPE);
const buzz::QName QN_GINGLE_VIDEO_SRCID(true, NS_GINGLE_VIDEO, "src-id");
const buzz::QName QN_GINGLE_VIDEO_BANDWIDTH(true, NS_GINGLE_VIDEO, "bandwidth");

// transports and candidates
const std::string NS_JINGLE_P2P("urn:xmpp:jingle:transports:ice-udp:1");
const std::string LN_TRANSPORT("transport");
const std::string LN_CANDIDATE("candidate");
const buzz::QName QN_JINGLE_P2P_TRANSPORT(true, NS_JINGLE_P2P, LN_TRANSPORT);
const buzz::QName QN_JINGLE_P2P_CANDIDATE(true, NS_JINGLE_P2P, LN_CANDIDATE);
const buzz::QName QN_UFRAG(true, cricket::NS_EMPTY, "ufrag");
const buzz::QName QN_PWD(true, cricket::NS_EMPTY, "pwd");
const buzz::QName QN_COMPONENT(true, cricket::NS_EMPTY, "component");
const buzz::QName QN_IP(true, cricket::NS_EMPTY, "ip");
const buzz::QName QN_PORT(true, cricket::NS_EMPTY, "port");
const buzz::QName QN_NETWORK(true, cricket::NS_EMPTY, "network");
const buzz::QName QN_GENERATION(true, cricket::NS_EMPTY, "generation");
const buzz::QName QN_PRIORITY(true, cricket::NS_EMPTY, "priority");
const buzz::QName QN_PROTOCOL(true, cricket::NS_EMPTY, "protocol");
const std::string JINGLE_CANDIDATE_TYPE_PEER_STUN("prflx");
const std::string JINGLE_CANDIDATE_TYPE_SERVER_STUN("srflx");
const std::string JINGLE_CANDIDATE_NAME_RTP("1");
const std::string JINGLE_CANDIDATE_NAME_RTCP("2");

const std::string NS_GINGLE_P2P("http://www.google.com/transport/p2p");
const buzz::QName QN_GINGLE_P2P_TRANSPORT(true, NS_GINGLE_P2P, LN_TRANSPORT);
const buzz::QName QN_GINGLE2_P2P_CANDIDATE(true, NS_GINGLE_P2P, LN_CANDIDATE);
const buzz::QName QN_GINGLE_P2P_CANDIDATE(true, NS_GINGLE, LN_CANDIDATE);
const buzz::QName QN_GINGLE_P2P_UNKNOWN_CHANNEL_NAME(true,
                             NS_GINGLE_P2P, "unknown-channel-name");
const buzz::QName QN_GINGLE_CANDIDATE(true, cricket::NS_GINGLE, "candidate");
const buzz::QName QN_ADDRESS(true, cricket::NS_EMPTY, "address");
const buzz::QName QN_USERNAME(true, cricket::NS_EMPTY, "username");
const buzz::QName QN_PASSWORD(true, cricket::NS_EMPTY, "password");
const buzz::QName QN_PREFERENCE(true, cricket::NS_EMPTY, "preference");
const std::string GINGLE_CANDIDATE_TYPE_STUN("stun");
const std::string GINGLE_CANDIDATE_NAME_RTP("rtp");
const std::string GINGLE_CANDIDATE_NAME_RTCP("rtcp");
const std::string GINGLE_CANDIDATE_NAME_VIDEO_RTP("video_rtp");
const std::string GINGLE_CANDIDATE_NAME_VIDEO_RTCP("video_rtcp");

const std::string NS_GINGLE_RAW("http://www.google.com/transport/raw-udp");
const buzz::QName QN_GINGLE_RAW_TRANSPORT(true, NS_GINGLE_RAW, "transport");
const buzz::QName QN_GINGLE_RAW_CHANNEL(true, NS_GINGLE_RAW, "channel");


// old stuff
#ifdef FEATURE_ENABLE_VOICEMAIL
const std::string NS_VOICEMAIL("http://www.google.com/session/voicemail");
const buzz::QName QN_VOICEMAIL_REGARDING(true, NS_VOICEMAIL, "regarding");
#endif

/* Disabling redirect until we can implement it the "Jingle way".
const std::string GINGLE_ACTION_REDIRECT("redirect");
const buzz::QName QN_REDIRECT_TARGET(true, NS_GINGLE_SESSION, "target");
const buzz::QName QN_REDIRECT_COOKIE(true, NS_GINGLE_SESSION, "cookie");
const buzz::QName QN_REDIRECT_REGARDING(true, NS_GINGLE_SESSION, "regarding");
*/

}  // namespace cricket
