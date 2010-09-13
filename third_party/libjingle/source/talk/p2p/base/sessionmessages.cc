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

#include "talk/base/logging.h"
#include "talk/p2p/base/sessionmessages.h"
#include "talk/xmpp/constants.h"
#include "talk/p2p/base/constants.h"
#include "talk/p2p/base/p2ptransport.h"
#include "talk/p2p/base/parsing.h"
#include "talk/p2p/base/sessionclient.h"
#include "talk/p2p/base/transport.h"

namespace cricket {

ActionType ToActionType(const std::string& type) {
  if (type == GINGLE_ACTION_INITIATE)
    return ACTION_SESSION_INITIATE;
  if (type == GINGLE_ACTION_INFO)
    return ACTION_SESSION_INFO;
  if (type == GINGLE_ACTION_ACCEPT)
    return ACTION_SESSION_ACCEPT;
  if (type == GINGLE_ACTION_REJECT)
    return ACTION_SESSION_REJECT;
  if (type == GINGLE_ACTION_TERMINATE)
    return ACTION_SESSION_TERMINATE;
  if (type == GINGLE_ACTION_CANDIDATES)
    return ACTION_TRANSPORT_INFO;
  if (type == JINGLE_ACTION_TRANSPORT_INFO)
    return ACTION_TRANSPORT_INFO;

  return ACTION_UNKNOWN;
}

std::string ToString(ActionType type, SignalingProtocol protocol) {
  switch (type) {
    case ACTION_SESSION_INITIATE:
      return GINGLE_ACTION_INITIATE;
    case ACTION_SESSION_INFO:
      return GINGLE_ACTION_INFO;
    case ACTION_SESSION_ACCEPT:
      return GINGLE_ACTION_ACCEPT;
    case ACTION_SESSION_REJECT:
      return GINGLE_ACTION_REJECT;
    case ACTION_SESSION_TERMINATE:
      return GINGLE_ACTION_TERMINATE;
    case ACTION_TRANSPORT_INFO:
      if (protocol == PROTOCOL_GINGLE2)
        return JINGLE_ACTION_TRANSPORT_INFO;
      else
        return GINGLE_ACTION_CANDIDATES;
    default:
      return "";
  }
}

bool IsSessionMessage(const buzz::XmlElement* stanza) {
  if (stanza->Name() != buzz::QN_IQ ||
      stanza->Attr(buzz::QN_TYPE) != buzz::STR_SET)
    return false;

  const buzz::XmlElement* action = stanza->FirstNamed(QN_GINGLE_SESSION);
  if (action == NULL)
    return false;

  return (action->HasAttr(buzz::QN_TYPE) &&
          action->HasAttr(buzz::QN_ID)   &&
          action->HasAttr(QN_INITIATOR));
}

bool ParseSessionMessage(const buzz::XmlElement* stanza,
                         SessionMessage* msg,
                         ParseError* error) {
  const buzz::XmlElement* action;
  if (!RequireXmlChild(stanza, QN_GINGLE_SESSION.LocalPart(), &action, error))
    return false;

  std::string type_string;
  if (!RequireXmlAttr(action, buzz::QN_TYPE, &type_string, error))
    return false;

  msg->id = stanza->Attr(buzz::QN_ID);
  msg->from = stanza->Attr(buzz::QN_FROM);
  msg->to = stanza->Attr(buzz::QN_TO);
  msg->stanza = stanza;

  msg->sid = action->Attr(buzz::QN_ID);
  msg->initiator = action->Attr(QN_INITIATOR);
  msg->type = ToActionType(type_string);
  if (msg->type == ACTION_UNKNOWN)
    return BadParse("unknown action: " + type_string, error);
  msg->action_elem = action;

  if (type_string == GINGLE_ACTION_CANDIDATES ||
      (msg->type == ACTION_SESSION_INITIATE &&
       action->FirstNamed(QN_GINGLE_P2P_TRANSPORT) == NULL)) {
    msg->protocol = PROTOCOL_GINGLE;
  } else {
    msg->protocol = PROTOCOL_GINGLE2;
  }

  return true;
}

void WriteSessionMessage(const SessionMessage& msg,
                         const XmlElements& action_elems,
                         buzz::XmlElement* stanza) {
  stanza->SetAttr(buzz::QN_TO, msg.to);
  stanza->SetAttr(buzz::QN_TYPE, buzz::STR_SET);

  buzz::XmlElement* action = new buzz::XmlElement(QN_GINGLE_SESSION, true);
  action->AddAttr(buzz::QN_TYPE,
                  ToString(msg.type, msg.protocol));
  action->AddAttr(buzz::QN_ID, msg.sid);
  action->AddAttr(QN_INITIATOR, msg.initiator);

  AddXmlChildren(action, action_elems);

  stanza->AddElement(action);
}


TransportParser* GetTransportParser(const TransportParserMap& trans_parsers,
                                    const std::string& name) {
  TransportParserMap::const_iterator map = trans_parsers.find(name);
  if (map == trans_parsers.end()) {
    return NULL;
  } else {
    return map->second;
  }
}

bool ParseCandidates(const buzz::XmlElement* candidates_elem,
                     const TransportParserMap& trans_parsers,
                     const std::string& trans_name,
                     Candidates* candidates,
                     ParseError* error) {
  TransportParser* trans_parser = GetTransportParser(trans_parsers, trans_name);
  if (trans_parser == NULL)
    return BadParse("unknown transport type: " + trans_name, error);

  return trans_parser->ParseCandidates(candidates_elem, candidates, error);
}

// Pass in NULL candidates if you don't want them parsed.
bool ParseGingleTransport(const buzz::XmlElement* action_elem,
                          const TransportParserMap& trans_parsers,
                          std::string* name,
                          Candidates* candidates,
                          ParseError* error) {
  const buzz::XmlElement* candidates_elem;
  const buzz::XmlElement* trans_elem = GetXmlChild(action_elem, LN_TRANSPORT);
  if (trans_elem == NULL) {  // PROTCOL_GINGLE
    *name = NS_GINGLE_P2P;
    candidates_elem = action_elem;
  } else {  // PROTOCOL_GINGLE2
    *name = trans_elem->Name().Namespace();
    candidates_elem = trans_elem;
  }

  if (candidates != NULL) {
    return ParseCandidates(candidates_elem, trans_parsers, *name,
                           candidates, error);
  }
  return true;
}

bool ParseGingleTransportName(const buzz::XmlElement* action_elem,
                              std::string* name,
                              ParseError* error) {
  return ParseGingleTransport(action_elem, TransportParserMap(),
                              name, NULL, error);
}

buzz::XmlElement* NewTransportElement(const std::string& name) {
  return new buzz::XmlElement(buzz::QName(true, name, LN_TRANSPORT), true);
}

void WriteGingleTransport(const std::string& trans_name,
                          const Candidates& candidates,
                          const TransportParserMap& trans_parsers,
                          SignalingProtocol protocol,
                          XmlElements* elems) {
  TransportParser* trans_parser = GetTransportParser(trans_parsers, trans_name);
  if (trans_parser == NULL)
    // TODO(pthatcher): should we handle errors from writes?
    // return BadParse("unknown transport type: " + trans_name, error);
    return;

  if (protocol == PROTOCOL_GINGLE2) {
    buzz::XmlElement* trans_elem = NewTransportElement(trans_name);
    XmlElements cand_elems;
    trans_parser->WriteCandidates(candidates, protocol, &cand_elems);
    AddXmlChildren(trans_elem, cand_elems);
    elems->push_back(trans_elem);
  } else {
    trans_parser->WriteCandidates(candidates, protocol, elems);
  }
}

void WriteGingleTransportWithoutCandidates(const std::string& trans_name,
                                           SignalingProtocol protocol,
                                           XmlElements* elems) {
  if (protocol == PROTOCOL_GINGLE2) {
    elems->push_back(NewTransportElement(trans_name));
  }
}

FormatParser* GetFormatParser(const FormatParserMap& format_parsers,
                              const std::string& name) {
  FormatParserMap::const_iterator map = format_parsers.find(name);
  if (map == format_parsers.end()) {
    return NULL;
  } else {
    return map->second;
  }
}

// Pass in a NULL format to disable format parsing (so you only get the name).
bool ParseFormat(const buzz::XmlElement* format_elem,
                 const FormatParserMap& format_parsers,
                 std::string* format_name,
                 const FormatDescription** format,
                 ParseError* error) {
  *format_name = format_elem->Name().Namespace();
  if (format != NULL) {
    FormatParser* format_parser = GetFormatParser(format_parsers, *format_name);
    if (format_parser == NULL)
      return BadParse("unknown application format: " + *format_name, error);
    *format = format_parser->ParseFormat(format_elem);
  }
  return true;
}

bool ParseGingleFormat(const buzz::XmlElement* action_elem,
                       const FormatParserMap& format_parsers,
                       std::string* format_name,
                       const FormatDescription** format,
                       ParseError* error) {
  const buzz::XmlElement* format_elem;
  if (!RequireXmlChild(action_elem, LN_DESCRIPTION, &format_elem, error))
    return false;

  return ParseFormat(format_elem, format_parsers,
                     format_name, format, error);
}

void WriteFormat(const std::string& format_name,
                 const FormatDescription* format,
                 const FormatParserMap& format_parsers,
                 XmlElements* elems) {
  FormatParser* format_parser =
      GetFormatParser(format_parsers, format_name);
  if (format_parser == NULL)
    // TODO(pthatcher): should we handle errors from writes?
    // return error->Set("unknown application format: " + init.format_name);
    return;

  elems->push_back(format_parser->WriteFormat(format));
}

bool ParseFormatName(const buzz::XmlElement* action_elem,
                     std::string* format_name,
                     ParseError* error) {
  return ParseGingleFormat(action_elem, FormatParserMap(),
                           format_name, NULL, error);
}

bool ParseSessionInitiate(const buzz::XmlElement* action_elem,
                          const FormatParserMap& format_parsers,
                          SessionInitiate* init, ParseError* error) {
  std::string format_name;
  const FormatDescription* format;
  if (!ParseGingleFormat(action_elem, format_parsers,
                         &format_name, &format, error))
    return false;
  init->SetFormat(format_name, format);

  if (!ParseGingleTransportName(action_elem, &(init->transport_name), error))
    return false;

  return true;
}

void WriteSessionInitiate(const SessionInitiate& init,
                          const FormatParserMap& format_parsers,
                          SignalingProtocol protocol,
                          XmlElements* elems) {
  WriteFormat(init.format_name, init.format, format_parsers, elems);
  // We don't have any candidates yet, so only send the transport
  // name.  Send candidates asynchronously later with transport-info
  // or candidates messages.
  WriteGingleTransportWithoutCandidates(init.transport_name, protocol, elems);
}

bool ParseSessionAccept(const buzz::XmlElement* action_elem,
                        const FormatParserMap& format_parsers,
                        SessionAccept* accept, ParseError* error) {
  std::string format_name;
  const FormatDescription* format;
  if (!ParseGingleFormat(action_elem, format_parsers,
                         &format_name, &format, error))
    return false;
  accept->SetFormat(format_name, format);

  return true;
}

void WriteSessionAccept(const SessionAccept& accept,
                        const FormatParserMap& format_parsers,
                        XmlElements* elems) {
  WriteFormat(accept.format_name, accept.format, format_parsers,
              elems);
}

bool ParseSessionTerminate(const buzz::XmlElement* action_elem,
                           SessionTerminate* term, ParseError* error) {
  const buzz::XmlElement* reason_elem = action_elem->FirstElement();
  if (reason_elem != NULL) {
    term->reason = reason_elem->Name().LocalPart();
    const buzz::XmlElement *debug_elem = reason_elem->FirstElement();
    if (debug_elem != NULL) {
      term->debug_reason = debug_elem->Name().LocalPart();
    }
  }
  return true;
}

void WriteSessionTerminate(const SessionTerminate& term,
                           XmlElements* elems) {
  elems->push_back(new buzz::XmlElement(
      buzz::QName(true, NS_EMPTY, term.reason)));
}

bool ParseTransportInfo(const buzz::XmlElement* action_elem,
                        const TransportParserMap& trans_parsers,
                        TransportInfo* info, ParseError* error) {
  return ParseGingleTransport(action_elem, trans_parsers,
                              &(info->transport_name), &(info->candidates),
                              error);
}

void WriteTransportInfo(const TransportInfo& info,
                        const TransportParserMap& trans_parsers,
                        SignalingProtocol protocol,
                        XmlElements* elems) {
  WriteGingleTransport(info.transport_name, info.candidates, trans_parsers,
                       protocol, elems);
}


}  // namespace cricket
