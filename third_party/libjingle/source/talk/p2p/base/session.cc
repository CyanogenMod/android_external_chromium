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

#include "talk/p2p/base/session.h"
#include "talk/base/common.h"
#include "talk/base/logging.h"
#include "talk/base/helpers.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/jid.h"
#include "talk/p2p/base/sessionclient.h"
#include "talk/p2p/base/transport.h"
#include "talk/p2p/base/transportchannelproxy.h"
#include "talk/p2p/base/p2ptransport.h"
#include "talk/p2p/base/p2ptransportchannel.h"

#include "talk/p2p/base/constants.h"

namespace {

const uint32 MSG_TIMEOUT = 1;
const uint32 MSG_ERROR = 2;
const uint32 MSG_STATE = 3;

}  // namespace

namespace cricket {

bool BadMessage(const buzz::QName type,
                const std::string& text,
                SessionError* err) {
  err->SetType(type);
  err->SetText(text);
  return false;
}

BaseSession::BaseSession(talk_base::Thread *signaling_thread)
    : state_(STATE_INIT), error_(ERROR_NONE),
      description_(NULL), remote_description_(NULL),
      signaling_thread_(signaling_thread) {
}

BaseSession::~BaseSession() {
  delete remote_description_;
  delete description_;
}

void BaseSession::SetState(State state) {
  ASSERT(signaling_thread_->IsCurrent());
  if (state != state_) {
    state_ = state;
    SignalState(this, state_);
    signaling_thread_->Post(this, MSG_STATE);
  }
}

void BaseSession::SetError(Error error) {
  ASSERT(signaling_thread_->IsCurrent());
  if (error != error_) {
    error_ = error;
    SignalError(this, error);
    if (error_ != ERROR_NONE)
      signaling_thread_->Post(this, MSG_ERROR);
  }
}

void BaseSession::OnMessage(talk_base::Message *pmsg) {
  switch (pmsg->message_id) {
  case MSG_TIMEOUT:
    // Session timeout has occured.
    SetError(ERROR_TIME);
    break;

  case MSG_ERROR:
    // Any of the defined errors is most likely fatal.
    Terminate();
    break;

  case MSG_STATE:
    switch (state_) {
    case STATE_SENTACCEPT:
    case STATE_RECEIVEDACCEPT:
      SetState(STATE_INPROGRESS);
      break;

    case STATE_SENTREJECT:
    case STATE_RECEIVEDREJECT:
      Terminate();
      break;

    default:
      // Explicitly ignoring some states here.
      break;
    }
    break;
  }
}


Session::Session(SessionManager *session_manager, const std::string& name,
                 const SessionID& id, const std::string& session_type,
                 SessionClient* client) :
    BaseSession(session_manager->signaling_thread()) {
  ASSERT(session_manager->signaling_thread()->IsCurrent());
  ASSERT(client != NULL);
  session_manager_ = session_manager;
  name_ = name;
  id_ = id;
  session_type_ = session_type;
  client_ = client;
  error_ = ERROR_NONE;
  state_ = STATE_INIT;
  initiator_ = false;
  SetTransport(new P2PTransport(session_manager_->worker_thread(),
                                session_manager_->port_allocator()));
  transport_negotiated_ = false;
  current_protocol_ = PROTOCOL_GINGLE2;
}

Session::~Session() {
  ASSERT(signaling_thread_->IsCurrent());

  ASSERT(state_ != STATE_DEINIT);
  state_ = STATE_DEINIT;
  SignalState(this, state_);

  for (ChannelMap::iterator iter = channels_.begin();
       iter != channels_.end();
       ++iter) {
    iter->second->SignalDestroyed(iter->second);
    delete iter->second;
  }

  delete transport_;
}

bool Session::Initiate(const std::string &to,
                       const SessionDescription *description) {
  ASSERT(signaling_thread_->IsCurrent());

  // Only from STATE_INIT
  if (state_ != STATE_INIT)
    return false;

  // Setup for signaling.
  remote_name_ = to;
  initiator_ = true;
  set_local_description(description);

  SendInitiateMessage(description);
  SetState(Session::STATE_SENTINITIATE);

  // We speculatively start attempting connection of the P2P transports.
  ConnectDefaultTransportChannels(transport_);
  return true;
}

bool Session::Accept(const SessionDescription *description) {
  ASSERT(signaling_thread_->IsCurrent());

  // Only if just received initiate
  if (state_ != STATE_RECEIVEDINITIATE)
    return false;

  // Setup for signaling.
  initiator_ = false;
  set_local_description(description);

  // Wait for ChooseTransport to complete
  if (!transport_negotiated_)
    return true;

  SendAcceptMessage();
  SetState(Session::STATE_SENTACCEPT);
  return true;
}

bool Session::Reject() {
  ASSERT(signaling_thread_->IsCurrent());

  // Reject is sent in response to an initiate or modify, to reject the
  // request
  if (state_ != STATE_RECEIVEDINITIATE && state_ != STATE_RECEIVEDMODIFY)
    return false;

  // Setup for signaling.
  initiator_ = false;

  SendRejectMessage();
  SetState(STATE_SENTREJECT);

  return true;
}

bool Session::Terminate() {
  ASSERT(signaling_thread_->IsCurrent());

  // Either side can terminate, at any time.
  switch (state_) {
    case STATE_SENTTERMINATE:
    case STATE_RECEIVEDTERMINATE:
      return false;

    case STATE_SENTREJECT:
    case STATE_RECEIVEDREJECT:
      // We don't need to send terminate if we sent or received a reject...
      // it's implicit.
      break;

    default:
      SendTerminateMessage();
      break;
  }

  SetState(STATE_SENTTERMINATE);
  return true;
}

// only used by app/win32/fileshare.cc
void Session::SendInfoMessage(const XmlElements& elems) {
  ASSERT(signaling_thread_->IsCurrent());
  SendMessage(ACTION_SESSION_INFO, elems);
}

void Session::SetTransport(Transport* transport) {
  transport_ = transport;
  transport->SignalConnecting.connect(
      this, &Session::OnTransportConnecting);
  transport->SignalWritableState.connect(
      this, &Session::OnTransportWritable);
  transport->SignalRequestSignaling.connect(
      this, &Session::OnTransportRequestSignaling);
  transport->SignalCandidatesReady.connect(
      this, &Session::OnTransportCandidatesReady);
  transport->SignalTransportError.connect(
      this, &Session::OnTransportSendError);
  transport->SignalChannelGone.connect(
      this, &Session::OnTransportChannelGone);
}

void Session::ConnectDefaultTransportChannels(Transport* transport) {
  for (ChannelMap::iterator iter = channels_.begin();
       iter != channels_.end();
       ++iter) {
    ASSERT(!transport->HasChannel(iter->first));
    transport->CreateChannel(iter->first, session_type());
  }
  transport->ConnectChannels();
}

void Session::ConnectTransportChannels(Transport* transport) {
  ASSERT(signaling_thread_->IsCurrent());

  // Create implementations for all of the channels if they don't exist.
  for (ChannelMap::iterator iter = channels_.begin();
       iter != channels_.end();
       ++iter) {
    TransportChannelProxy* channel = iter->second;
    TransportChannelImpl* impl = transport->GetChannel(channel->name());
    if (impl == NULL)
      impl = transport->CreateChannel(channel->name(), session_type());
    ASSERT(impl != NULL);
    channel->SetImplementation(impl);
  }

  // Have this transport start connecting if it is not already.
  // (We speculatively connect the most common transport right away.)
  transport->ConnectChannels();
}

TransportParserMap Session::GetTransportParsers() {
  TransportParserMap parsers;
  parsers[transport_->name()] = transport_;
  return parsers;
}

FormatParserMap Session::GetFormatParsers() {
  FormatParserMap parsers;
  parsers[session_type_] = client_;
  return parsers;
}

TransportChannel* Session::CreateChannel(const std::string& name) {
  ASSERT(channels_.find(name) == channels_.end());
  ASSERT(!transport_->HasChannel(name));

  TransportChannelProxy* channel =
      new TransportChannelProxy(name, session_type_);
  channels_[name] = channel;
  if (transport_negotiated_) {
    channel->SetImplementation(transport_->CreateChannel(name, session_type_));
  } else if (state_ == STATE_SENTINITIATE) {
    transport_->CreateChannel(name, session_type());
  }
  return channel;
}

TransportChannel* Session::GetChannel(const std::string& name) {
  ChannelMap::iterator iter = channels_.find(name);
  return (iter != channels_.end()) ? iter->second : NULL;
}

void Session::DestroyChannel(TransportChannel* channel) {
  ChannelMap::iterator iter = channels_.find(channel->name());
  ASSERT(iter != channels_.end());
  ASSERT(channel == iter->second);
  channels_.erase(iter);
  channel->SignalDestroyed(channel);
  delete channel;
}

void Session::OnSignalingReady() {
  ASSERT(signaling_thread_->IsCurrent());
  transport_->OnSignalingReady();
}

void Session::OnTransportConnecting(Transport* transport) {
  // This is an indication that we should begin watching the writability
  // state of the transport.
  OnTransportWritable(transport);
}

void Session::OnTransportWritable(Transport* transport) {
  ASSERT(signaling_thread_->IsCurrent());
  ASSERT(transport == transport_);

  // If the transport is not writable, start a timer to make sure that it
  // becomes writable within a reasonable amount of time.  If it does not, we
  // terminate since we can't actually send data.  If the transport is writable,
  // cancel the timer.  Note that writability transitions may occur repeatedly
  // during the lifetime of the session.

  signaling_thread_->Clear(this, MSG_TIMEOUT);
  if (transport->HasChannels() && !transport->writable()) {
    signaling_thread_->PostDelayed(
        session_manager_->session_timeout() * 1000, this, MSG_TIMEOUT);
  }
}

void Session::OnTransportRequestSignaling(Transport* transport) {
  ASSERT(signaling_thread_->IsCurrent());
  SignalRequestSignaling(this);
}

void Session::OnTransportCandidatesReady(Transport* transport,
                                         const Candidates& candidates) {
  ASSERT(signaling_thread_->IsCurrent());
  if (!transport_negotiated_) {
    for (Candidates::const_iterator iter = candidates.begin();
         iter != candidates.end();
         ++iter) {
      sent_candidates_.push_back(*iter);
    }
  }
  SendTransportInfoMessage(candidates);
}

void Session::OnTransportSendError(Transport* transport,
                                   const buzz::XmlElement* stanza,
                                   const buzz::QName& name,
                                   const std::string& type,
                                   const std::string& text,
                                   const buzz::XmlElement* extra_info) {
  ASSERT(signaling_thread_->IsCurrent());
  SignalErrorMessage(this, stanza, name, type, text, extra_info);
}

void Session::OnTransportChannelGone(Transport* transport,
                                     const std::string& name) {
  ASSERT(signaling_thread_->IsCurrent());
  SignalChannelGone(this, name);
}

void Session::OnIncomingMessage(const SessionMessage& msg) {
  ASSERT(signaling_thread_->IsCurrent());
  ASSERT(state_ == STATE_INIT || msg.from == remote_name_);

  // PROTOCOL_GINGLE is effectively the old compatibility_mode_ which
  // meant "talking to old client".  We can flip to
  // compatibility_mode_, but not back.
  if (msg.protocol == PROTOCOL_GINGLE) {
    current_protocol_ = PROTOCOL_GINGLE;
  }

  if (msg.type == ACTION_TRANSPORT_INFO &&
      current_protocol_ == PROTOCOL_GINGLE &&
      !transport_negotiated_) {
    // We have sent some already (using transport-info), and we need
    // to re-send them using the candidates message.
    if (sent_candidates_.size() > 0) {
      SendTransportInfoMessage(sent_candidates_);
    }
    sent_candidates_.clear();
  }

  bool valid = false;
  SessionError error;
  switch (msg.type) {
    case ACTION_SESSION_INITIATE:
      valid = OnInitiateMessage(msg, &error);
      break;
    case ACTION_SESSION_INFO:
      valid = OnInfoMessage(msg);
      break;
    case ACTION_SESSION_ACCEPT:
      valid = OnAcceptMessage(msg, &error);
      break;
    case ACTION_SESSION_REJECT:
      valid = OnRejectMessage(msg, &error);
      break;
    case ACTION_SESSION_TERMINATE:
      valid = OnTerminateMessage(msg, &error);
      break;
    case ACTION_TRANSPORT_INFO:
      valid = OnTransportInfoMessage(msg, &error);
      break;
    default:
      valid = BadMessage(buzz::QN_STANZA_BAD_REQUEST,
                         "unknown session message type",
                         &error);
  }

  if (valid) {
    SendAcknowledgementMessage(msg.stanza);
  } else {
    SignalErrorMessage(this, msg.stanza, error.type,
                       "modify", error.text, NULL);
  }
}

void Session::OnFailedSend(const buzz::XmlElement* orig_stanza,
                           const buzz::XmlElement* error_stanza) {
  ASSERT(signaling_thread_->IsCurrent());

  SessionMessage msg;
  ParseError parse_error;
  if (!ParseSessionMessage(orig_stanza, &msg, &parse_error)) {
    LOG(LERROR) << "Error parsing failed send: " << parse_error.text
                << ":" << orig_stanza;
    return;
  }

  std::string error_type = "cancel";

  const buzz::XmlElement* error = error_stanza->FirstNamed(buzz::QN_ERROR);
  ASSERT(error != NULL);
  if (error) {
    ASSERT(error->HasAttr(buzz::QN_TYPE));
    error_type = error->Attr(buzz::QN_TYPE);

    LOG(LERROR) << "Session error:\n" << error->Str() << "\n"
                << "in response to:\n" << orig_stanza->Str();
  }

  if (msg.type == ACTION_TRANSPORT_INFO) {
    // Transport messages frequently generate errors because they are sent right
    // when we detect a network failure.  For that reason, we ignore such
    // errors, because if we do not establish writability again, we will
    // terminate anyway.  The exceptions are transport-specific error tags,
    // which we pass on to the respective transport.
    for (const buzz::XmlElement* elem = error->FirstElement();
         NULL != elem; elem = elem->NextElement()) {
      if (transport_->name() == elem->Name().Namespace()) {
        transport_->OnTransportError(elem);
      }
    }
  } else if ((error_type != "continue") && (error_type != "wait")) {
    // We do not set an error if the other side said it is okay to continue
    // (possibly after waiting).  These errors can be ignored.
    SetError(ERROR_RESPONSE);
  }
}

bool Session::OnInitiateMessage(const SessionMessage& msg,
                                SessionError* error) {
  if (!CheckState(STATE_INIT, error))
    return false;

  SessionInitiate init;
  if (!ParseSessionInitiate(msg.action_elem, GetFormatParsers(), &init, error))
    return false;

  if (transport_->name() != init.transport_name)
    return BadMessage(buzz::QN_STANZA_NOT_ACCEPTABLE,
                      "no supported transport in offer",
                      error);

  initiator_ = false;
  remote_name_ = msg.from;
  set_remote_description(init.AdoptFormat());
  SetState(STATE_RECEIVEDINITIATE);

  // User of Session may listen to state change and call Reject().
  if (state_ != STATE_SENTREJECT && !transport_negotiated_) {
    transport_negotiated_ = true;
    ConnectTransportChannels(transport_);

    // If the user wants to accept, allow that now
    if (description_) {
      Accept(description_);
    }
  }
  return true;
}

bool Session::OnAcceptMessage(const SessionMessage& msg, SessionError* error) {
  if (!CheckState(STATE_SENTINITIATE, error))
    return false;

  SessionAccept accept;
  if (!ParseSessionAccept(msg.action_elem, GetFormatParsers(), &accept, error))
    return false;

  set_remote_description(accept.AdoptFormat());
  SetState(STATE_RECEIVEDACCEPT);
  return true;
}

bool Session::OnRejectMessage(const SessionMessage& msg, SessionError* error) {
  if (!CheckState(STATE_SENTINITIATE, error))
    return false;

  SetState(STATE_RECEIVEDREJECT);
  return true;
}

// Only used by app/win32/fileshare.cc.
bool Session::OnInfoMessage(const SessionMessage& msg) {
  SignalInfoMessage(this, CopyOfXmlChildren(msg.action_elem));
  return true;
}

bool Session::OnTerminateMessage(const SessionMessage& msg,
                                 SessionError* error) {
  SessionTerminate term;
  if (!ParseSessionTerminate(msg.action_elem, &term, error))
    return false;

  SignalReceivedTerminateReason(this, term.reason);
  if (term.debug_reason != buzz::STR_EMPTY) {
    LOG(LS_VERBOSE) << "Received error on call: " << term.debug_reason;
  }
  return true;
}

bool Session::OnTransportInfoMessage(const SessionMessage& msg,
                                     SessionError* error) {
  TransportInfo info;
  if (!ParseTransportInfo(msg.action_elem, GetTransportParsers(), &info, error))
    return false;

  if (transport_->name() == info.transport_name) {
    transport_->OnRemoteCandidates(info.candidates);
    if (!transport_negotiated_) {
      transport_negotiated_ = true;
      ConnectTransportChannels(transport_);
    }
  }
  return true;
}

bool Session::CheckState(State state, SessionError* error) {
  ASSERT(state_ == state);
  if (state_ != state) {
    return BadMessage(buzz::QN_STANZA_NOT_ALLOWED,
                      "message not allowed in current state",
                      error);
  }
  return true;
}

void Session::OnMessage(talk_base::Message *pmsg) {
  // preserve this because BaseSession::OnMessage may modify it
  BaseSession::State orig_state = state_;

  BaseSession::OnMessage(pmsg);

  switch (pmsg->message_id) {
  case MSG_STATE:
    switch (orig_state) {
    case STATE_SENTTERMINATE:
    case STATE_RECEIVEDTERMINATE:
      session_manager_->DestroySession(this);
      break;

    default:
      // Explicitly ignoring some states here.
      break;
    }
    break;
  }
}

void Session::SendInitiateMessage(const SessionDescription *description) {
  SessionInitiate init(transport_->name(), session_type_, description);
  XmlElements elems;
  WriteSessionInitiate(init, GetFormatParsers(), current_protocol_, &elems);
  SendMessage(ACTION_SESSION_INITIATE, elems);
}

void Session::SendAcceptMessage() {
  // TODO(pthatcher): When we support the Jingle standard, we need to
  // include at least an empty <transport> in the accept.
  std::string transport_name = "";
  SessionAccept accept(transport_name, session_type_, description_);

  XmlElements elems;
  WriteSessionAccept(accept, GetFormatParsers(), &elems);
  SendMessage(ACTION_SESSION_ACCEPT, elems);
}

void Session::SendRejectMessage() {
  XmlElements elems;
  SendMessage(ACTION_SESSION_REJECT, elems);
}

void Session::SendTerminateMessage() {
  XmlElements elems;
  SendMessage(ACTION_SESSION_TERMINATE, elems);
}

void Session::SendTransportInfoMessage(const Candidates& candidates) {
  TransportInfo info(transport_->name(), candidates);
  XmlElements elems;
  WriteTransportInfo(info, GetTransportParsers(), current_protocol_, &elems);
  SendMessage(ACTION_TRANSPORT_INFO, elems);
}

void Session::SendMessage(ActionType type, const XmlElements& action_elems) {
  SessionMessage msg(current_protocol_, type, id_.id_str(), id_.initiator());
  msg.to = remote_name_;

  talk_base::scoped_ptr<buzz::XmlElement> stanza(
      new buzz::XmlElement(buzz::QN_IQ));
  WriteSessionMessage(msg, action_elems, stanza.get());
  SignalOutgoingMessage(this, stanza.get());
}

void Session::SendAcknowledgementMessage(const buzz::XmlElement* stanza) {
  talk_base::scoped_ptr<buzz::XmlElement> ack(
      new buzz::XmlElement(buzz::QN_IQ));
  ack->SetAttr(buzz::QN_TO, remote_name_);
  ack->SetAttr(buzz::QN_ID, stanza->Attr(buzz::QN_ID));
  ack->SetAttr(buzz::QN_TYPE, "result");

  SignalOutgoingMessage(this, ack.get());
}

}  // namespace cricket
