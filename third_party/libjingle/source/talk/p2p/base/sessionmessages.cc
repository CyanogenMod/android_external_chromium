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

#include "talk/p2p/base/sessionmessages.h"

#include "talk/base/logging.h"
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
  if (type == JINGLE_ACTION_TRANSPORT_ACCEPT)
    return ACTION_TRANSPORT_ACCEPT;

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

bool WriteGingleTransport(const std::string& trans_name,
                          const Candidates& candidates,
                          const TransportParserMap& trans_parsers,
                          SignalingProtocol protocol,
                          XmlElements* elems,
                          WriteError* error) {
  TransportParser* trans_parser = GetTransportParser(trans_parsers, trans_name);
  if (trans_parser == NULL)
    return BadWrite("unknown transport type: " + trans_name, error);

  if (protocol == PROTOCOL_GINGLE2) {
    buzz::XmlElement* trans_elem = NewTransportElement(trans_name);
    XmlElements cand_elems;
    if (!trans_parser->WriteCandidates(candidates, protocol,
                                       &cand_elems, error))
      return false;
    AddXmlChildren(trans_elem, cand_elems);
    elems->push_back(trans_elem);
  } else {
    if (!trans_parser->WriteCandidates(candidates, protocol, elems, error))
      return false;
  }
  return true;
}

bool WriteGingleTransportWithoutCandidates(const std::string& trans_name,
                                           SignalingProtocol protocol,
                                           XmlElements* elems,
                                           WriteError* error) {
  if (protocol == PROTOCOL_GINGLE2) {
    elems->push_back(NewTransportElement(trans_name));
  }
  return true;
}

ContentParser* FindContentParser(const ContentParserMap& content_parsers,
                                 const std::string& type) {
  ContentParserMap::const_iterator map = content_parsers.find(type);
  if (map == content_parsers.end()) {
    return NULL;
  } else {
    return map->second;
  }
}

// Like FindContentParser, but does trickery for NS_GINGLE_AUDIO and
// NS_GINGLE_VIDEO.  Because clients still assume only one content
// type, but we split up the types, we need to be able to let either
// content type work as the other.
ContentParser* GetContentParser(const ContentParserMap& content_parsers,
                                const std::string& type) {
  ContentParser* parser = FindContentParser(content_parsers, type);
  if (parser == NULL) {
    if (type == NS_GINGLE_AUDIO) {
      return FindContentParser(content_parsers, NS_GINGLE_VIDEO);
    } else if (type == NS_GINGLE_VIDEO) {
      return FindContentParser(content_parsers, NS_GINGLE_AUDIO);
    }
  }

  return parser;
}

bool ParseContent(const std::string& name,
                  const std::string& type,
                  const buzz::XmlElement* elem,
                  const ContentParserMap& parsers,
                  std::vector<ContentInfo>* contents,
                  ParseError* error) {
  ContentParser* parser = GetContentParser(parsers, type);
  if (parser == NULL)
    return BadParse("unknown application content: " + type, error);

  const ContentDescription* desc;
  if (!parser->ParseContent(elem, &desc, error))
    return false;

  contents->push_back(ContentInfo(name, type, desc));
  return true;
}

bool ParseGingleContentType(const buzz::XmlElement* action_elem,
                            std::string* content_type,
                            const buzz::XmlElement** content_elem,
                            ParseError* error) {
  if (!RequireXmlChild(action_elem, LN_DESCRIPTION, content_elem, error))
    return false;

  *content_type = (*content_elem)->Name().Namespace();
  return true;
}

bool ParseGingleContents(const buzz::XmlElement* action_elem,
                         const ContentParserMap& content_parsers,
                         std::vector<ContentInfo>* contents,
                         ParseError* error) {
  std::string content_type;
  const buzz::XmlElement* content_elem;
  if (!ParseGingleContentType(action_elem, &content_type, &content_elem, error))
    return false;

  if (content_type == NS_GINGLE_VIDEO) {
    // A parser parsing audio or video content should look at the
    // namespace and only parse the codecs relevant to that namespace.
    // We use this to control which codecs get parsed: first video,
    // then audio.
    if (!ParseContent(CN_VIDEO, NS_GINGLE_VIDEO,
                      content_elem, content_parsers,
                      contents, error))
      return false;

    talk_base::scoped_ptr<buzz::XmlElement> audio_elem(
        new buzz::XmlElement(QN_GINGLE_AUDIO_CONTENT));
    CopyXmlChildren(content_elem, audio_elem.get());
    if (!ParseContent(CN_AUDIO, NS_GINGLE_AUDIO,
                      audio_elem.get(), content_parsers,
                      contents, error))
      return false;
  } else {
    if (!ParseContent(CN_OTHER, content_type,
                      content_elem, content_parsers,
                      contents, error))
      return false;
  }
  return true;
}

buzz::XmlElement* WriteContent(const ContentInfo& content,
			       const ContentParserMap& parsers,
			       WriteError* error) {
  ContentParser* parser = GetContentParser(parsers, content.type);
  if (parser == NULL) {
    BadWrite("unknown content type: " + content.type, error);
    return NULL;
  }

  buzz::XmlElement* elem = NULL;
  if (!parser->WriteContent(content.description, &elem, error))
    return NULL;

  return elem;
}

bool WriteGingleContents(const std::vector<ContentInfo>& contents,
			 const ContentParserMap& parsers,
			 XmlElements* elems,
			 WriteError* error) {
  if (contents.size() == 1) {
    buzz::XmlElement* elem = WriteContent(contents.front(), parsers, error);
    if (!elem)
      return false;

    elems->push_back(elem);
  } else if (contents.size() == 2 &&
      contents.at(0).type == NS_GINGLE_AUDIO &&
      contents.at(1).type == NS_GINGLE_VIDEO) {
     // Special-case audio + video contents so that they are "merged"
     // into one "video" content.
    buzz::XmlElement* audio = WriteContent(contents.at(0), parsers, error);
    if (!audio)
      return false;

    buzz::XmlElement* video = WriteContent(contents.at(1), parsers, error);
    if (!video) {
      delete audio;
      return false;
    }

    CopyXmlChildren(audio, video);
    elems->push_back(video);
    delete audio;
  } else {
    return BadWrite("Gingle protocol may only have one content.", error);
  }

  return true;
}

bool ParseFirstContentType(const buzz::XmlElement* action_elem,
                           std::string* content_type,
                           ParseError* error) {
  const buzz::XmlElement* content_elem;
  return ParseGingleContentType(
      action_elem, content_type, &content_elem, error);
}

bool ParseSessionInitiate(const buzz::XmlElement* action_elem,
                          const ContentParserMap& content_parsers,
                          SessionInitiate* init, ParseError* error) {
  if (!ParseGingleContents(action_elem, content_parsers,
                           &init->contents, error))
    return false;

  if (!ParseGingleTransportName(action_elem, &(init->transport_name), error))
    return false;

  return true;
}

bool WriteSessionInitiate(const SessionInitiate& init,
                          const ContentParserMap& content_parsers,
                          SignalingProtocol protocol,
                          XmlElements* elems,
                          WriteError* error) {
  if (!WriteGingleContents(init.contents, content_parsers, elems, error))
    return false;

  // We don't have any candidates yet, so only send the transport
  // name.  Send candidates asynchronously later with transport-info
  // or candidates messages.
  if (!WriteGingleTransportWithoutCandidates(
          init.transport_name, protocol, elems, error))
    return false;

  return true;
}

bool ParseSessionAccept(const buzz::XmlElement* action_elem,
                        const ContentParserMap& content_parsers,
                        SessionAccept* accept, ParseError* error) {
  if (!ParseGingleContents(action_elem, content_parsers,
                           &accept->contents, error))
    return false;

  return true;
}

bool WriteSessionAccept(const SessionAccept& accept,
                        const ContentParserMap& content_parsers,
                        XmlElements* elems,
                        WriteError* error) {
  return WriteGingleContents(accept.contents, content_parsers, elems, error);
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

bool WriteSessionTerminate(const SessionTerminate& term,
                           XmlElements* elems,
                           WriteError* error) {
  elems->push_back(new buzz::XmlElement(
      buzz::QName(true, NS_EMPTY, term.reason)));
  return true;
}

bool ParseTransportInfo(const buzz::XmlElement* action_elem,
                        const TransportParserMap& trans_parsers,
                        TransportInfo* info, ParseError* error) {
  return ParseGingleTransport(action_elem, trans_parsers,
                              &(info->transport_name), &(info->candidates),
                              error);
}

bool WriteTransportInfo(const TransportInfo& info,
                        const TransportParserMap& trans_parsers,
                        SignalingProtocol protocol,
                        XmlElements* elems,
                        WriteError* error) {
  return WriteGingleTransport(info.transport_name, info.candidates,
                              trans_parsers, protocol,
                              elems, error);
}


}  // namespace cricket
