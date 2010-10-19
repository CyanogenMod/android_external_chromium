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
#include "talk/p2p/base/sessiondescription.h"  // Needed to delete contents.

namespace cricket {

struct ParseError;
struct WriteError;
class Candidate;
class ContentParser;
class TransportParser;

typedef std::vector<buzz::XmlElement*> XmlElements;
typedef std::vector<Candidate> Candidates;
typedef std::map<std::string, ContentParser*> ContentParserMap;
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

struct SessionInitiate {
  // Object will have ownership of contents.
  SessionInitiate() : owns_contents(true)  {}

  // Caller retains ownership of contents.
  SessionInitiate(const std::string& transport_name,
                  const std::vector<ContentInfo>& contents) :
      transport_name(transport_name),
      contents(contents), owns_contents(false) {}

  ~SessionInitiate() {
    if (owns_contents) {
      for (std::vector<ContentInfo>::iterator content = contents.begin();
           content != contents.end(); content++) {
        delete content->description;
      }
    }
  }

  // Caller takes ownership of contents.
  std::vector<ContentInfo> AdoptContents() {
    std::vector<ContentInfo> out;
    contents.swap(out);
    return out;
  }

  std::string transport_name;  // xmlns of <transport>
  // TODO(pthatcher): Jingle spec allows candidates to be in the
  // initiate.  We should support receiving them.
  std::vector<ContentInfo> contents;
  bool owns_contents;
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
bool ParseFirstContentType(const buzz::XmlElement* action_elem,
                           std::string* content_type,
                           ParseError* error);
void WriteSessionMessage(const SessionMessage& msg,
                         const XmlElements& action_elems,
                         buzz::XmlElement* stanza);
bool ParseSessionInitiate(const buzz::XmlElement* action_elem,
                          const ContentParserMap& content_parsers,
                          SessionInitiate* init, ParseError* error);
bool WriteSessionInitiate(const SessionInitiate& init,
                          const ContentParserMap& content_parsers,
                          SignalingProtocol protocol,
                          XmlElements* elems,
                          WriteError* error);
bool ParseSessionAccept(const buzz::XmlElement* action_elem,
                        const ContentParserMap& content_parsers,
                        SessionAccept* accept, ParseError* error);
bool WriteSessionAccept(const SessionAccept& accept,
                        const ContentParserMap& content_parsers,
                        XmlElements* elems,
                        WriteError* error);
bool ParseSessionTerminate(const buzz::XmlElement* action_elem,
                           SessionTerminate* term, ParseError* error);
bool WriteSessionTerminate(const SessionAccept& term,
                           XmlElements* elems,
                           WriteError* error);
bool ParseTransportInfo(const buzz::XmlElement* action_elem,
                        const TransportParserMap& trans_parsers,
                        TransportInfo* info, ParseError* error);
bool WriteTransportInfo(const TransportInfo& info,
                        const TransportParserMap& trans_parsers,
                        SignalingProtocol protocol,
                        XmlElements* elems,
                        WriteError* error);
}  // namespace cricket

#endif  // TALK_P2P_BASE_SESSIONMESSAGES_H_
