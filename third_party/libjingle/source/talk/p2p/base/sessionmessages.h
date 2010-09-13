/*
 * libjingle
 * Copyright 2010, Google Inc.
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

#ifndef TALK_P2P_BASE_SESSIONMESSAGES_H_
#define TALK_P2P_BASE_SESSIONMESSAGES_H_

#include <string>
#include <vector>
#include <map>

#include "talk/xmllite/xmlelement.h"
#include "talk/p2p/base/constants.h"
// Needed to delete SessionInitiate.format.
#include "talk/p2p/base/sessiondescription.h"

namespace cricket {

struct ParseError;
class Candidate;
class FormatParser;
class TransportParser;

// see comment in constants.h about FormatDescription and
// SessionDescription being the same.  SessionDescription is the old
// word.  FormatDescription is the new word.
typedef SessionDescription FormatDescription;
typedef std::vector<buzz::XmlElement*> XmlElements;
typedef std::vector<Candidate> Candidates;
typedef std::map<std::string, FormatParser*> FormatParserMap;
typedef std::map<std::string, TransportParser*> TransportParserMap;

enum ActionType {
  ACTION_UNKNOWN,

  ACTION_SESSION_INITIATE,
  ACTION_SESSION_INFO,
  ACTION_SESSION_ACCEPT,
  ACTION_SESSION_REJECT,
  ACTION_SESSION_TERMINATE,

  ACTION_TRANSPORT_INFO,
  ACTION_TRANSPORT_ACCEPT,
};

// Abstraction of a <jingle> element within an <iq> stanza, per XMPP
// standard XEP-166.  Can be serialized into multiple protocols,
// including the standard (Jingle) and the draft standard (Gingle).
// In general, used to communicate actions related to a p2p session,
// such accept, initiate, terminate, etc.

struct SessionMessage {
  SessionMessage() : action_elem(NULL), stanza(NULL) {}

  SessionMessage(SignalingProtocol protocol, ActionType type,
                 const std::string& sid, const std::string& initiator) :
      protocol(protocol), type(type), sid(sid), initiator(initiator),
      action_elem(NULL), stanza(NULL) {}

  std::string id;
  std::string from;
  std::string to;
  SignalingProtocol protocol;
  ActionType type;
  std::string sid;  // session id
  std::string initiator;

  // Used for further parsing when necessary.
  // Represents <session> or <jingle>.
  const buzz::XmlElement* action_elem;
  // Mostly used for debugging.
  const buzz::XmlElement* stanza;
};

// These are different "action"s found in <jingle> or <session> The
// Jingle specs allows actions to have multiple "contents", where each
// content is a pair of format+transport.  our Sessions can only
// handle one such content.  It will be a long time before we can
// support multiple contents in the session.  So, our actions only
// supports one.

// TODO(pthatcher): switch to jingle-style contents (Session has
// multiple Contents, which each are a pairs of (format, transport)

struct SessionInitiate {
  SessionInitiate() : format(NULL), owns_format(false)  {}

  SessionInitiate(const std::string& transport_name,
                  const std::string& format_name,
                  const FormatDescription* format) :
      transport_name(transport_name),
      format_name(format_name), format(format), owns_format(false) {}

  ~SessionInitiate() {
    if (owns_format) {
      delete format;
    }
  }

  // Object takes ownership of format.
  void SetFormat(const std::string& format_name,
                 const FormatDescription* format) {
    this->format_name = format_name;
    this->format = format;
    this->owns_format = true;
  }

  // Caller takes ownership of format.
  const FormatDescription* AdoptFormat() {
    const FormatDescription* out = format;
    format = NULL;
    owns_format = false;
    return out;
  }

  std::string transport_name;  // xmlns of <transport>
  // TODO(pthatcher): Jingle spec allows candidates to be in the
  // initiate.  We should support receiving them.
  std::string format_name;  // xmlns of <description>
  const FormatDescription* format;
  bool owns_format;
};

typedef SessionInitiate SessionAccept;

struct SessionTerminate {
  std::string reason;
  std::string debug_reason;
};

struct TransportInfo {
  TransportInfo() {}

  TransportInfo(const std::string& transport_name,
                const Candidates& candidates) :
      transport_name(transport_name), candidates(candidates) {}

  std::string transport_name;  // xmlns of <transport>
  Candidates candidates;
};

struct SessionReject {
};

bool IsSessionMessage(const buzz::XmlElement* stanza);
bool ParseSessionMessage(const buzz::XmlElement* stanza,
                         SessionMessage* msg,
                         ParseError* error);
bool ParseFormatName(const buzz::XmlElement* action_elem,
                     std::string* format_name,
                     ParseError* error);
void WriteSessionMessage(const SessionMessage& msg,
                         const XmlElements& action_elems,
                         buzz::XmlElement* stanza);
bool ParseSessionInitiate(const buzz::XmlElement* action_elem,
                          const FormatParserMap& format_parsers,
                          SessionInitiate* init, ParseError* error);
void WriteSessionInitiate(const SessionInitiate& init,
                          const FormatParserMap& format_parsers,
                          SignalingProtocol protocol,
                          XmlElements* elems);
bool ParseSessionAccept(const buzz::XmlElement* action_elem,
                        const FormatParserMap& format_parsers,
                        SessionAccept* accept, ParseError* error);
void WriteSessionAccept(const SessionAccept& accept,
                        const FormatParserMap& format_parsers,
                        XmlElements* elems);
bool ParseSessionTerminate(const buzz::XmlElement* action_elem,
                           SessionTerminate* term, ParseError* error);
void WriteSessionTerminate(const SessionAccept& term,
                           XmlElements* elems);
bool ParseTransportInfo(const buzz::XmlElement* action_elem,
                        const TransportParserMap& trans_parsers,
                        TransportInfo* info, ParseError* error);
void WriteTransportInfo(const TransportInfo& info,
                        const TransportParserMap& trans_parsers,
                        SignalingProtocol protocol,
                        XmlElements* elems);
}  // namespace cricket

#endif  // TALK_P2P_BASE_SESSIONMESSAGES_H_
