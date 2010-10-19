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

#ifndef TALK_P2P_BASE_SESSION_H_
#define TALK_P2P_BASE_SESSION_H_

#include <list>
#include <map>
#include <string>
#include <vector>

#include "talk/p2p/base/sessionmessages.h"
#include "talk/p2p/base/sessionmanager.h"
#include "talk/base/socketaddress.h"
#include "talk/p2p/base/sessionclient.h"
#include "talk/p2p/base/sessionid.h"
#include "talk/p2p/base/parsing.h"
#include "talk/p2p/base/port.h"
#include "talk/xmllite/xmlelement.h"
#include "talk/xmpp/constants.h"

class JingleMessageHandler;

namespace cricket {

class P2PTransportChannel;
class Transport;
class TransportChannel;
class TransportChannelProxy;
class TransportChannelImpl;

// We add "type" to the errors because it's need for
// SignalErrorMessage.
struct SessionError : ParseError {
  buzz::QName type;

  // if unset, assume type is a parse error
  SessionError() : ParseError(), type(buzz::QN_STANZA_BAD_REQUEST) {}

  void SetType(const buzz::QName type) {
    this->type = type;
  }
};

// TODO(juberti): Consider simplifying the dependency from Voice/VideoChannel
// on Session. Right now the Channel class requires a BaseSession, but it only
// uses CreateChannel/DestroyChannel. Perhaps something like a
// TransportChannelFactory could be hoisted up out of BaseSession, or maybe
// the transports could be passed in directly.

// A BaseSession manages general session state. This includes negotiation
// of both the application-level and network-level protocols:  the former
// defines what will be sent and the latter defines how it will be sent.  Each
// network-level protocol is represented by a Transport object.  Each Transport
// participates in the network-level negotiation.  The individual streams of
// packets are represented by TransportChannels.  The application-level protocol
// is represented by SessionDecription objects.
class BaseSession : public sigslot::has_slots<>,
                    public talk_base::MessageHandler {
 public:
  enum State {
    STATE_INIT = 0,
    STATE_SENTINITIATE,       // sent initiate, waiting for Accept or Reject
    STATE_RECEIVEDINITIATE,   // received an initiate. Call Accept or Reject
    STATE_SENTACCEPT,         // sent accept. begin connecting transport
    STATE_RECEIVEDACCEPT,     // received accept. begin connecting transport
    STATE_SENTMODIFY,         // sent modify, waiting for Accept or Reject
    STATE_RECEIVEDMODIFY,     // received modify, call Accept or Reject
    STATE_SENTREJECT,         // sent reject after receiving initiate
    STATE_RECEIVEDREJECT,     // received reject after sending initiate
    STATE_SENTREDIRECT,       // sent direct after receiving initiate
    STATE_SENTTERMINATE,      // sent terminate (any time / either side)
    STATE_RECEIVEDTERMINATE,  // received terminate (any time / either side)
    STATE_INPROGRESS,         // session accepted and in progress
    STATE_DEINIT,             // session is being destroyed
  };

  enum Error {
    ERROR_NONE = 0,      // no error
    ERROR_TIME = 1,      // no response to signaling
    ERROR_RESPONSE = 2,  // error during signaling
    ERROR_NETWORK = 3,   // network error, could not allocate network resources
  };

  explicit BaseSession(talk_base::Thread *signaling_thread);
  virtual ~BaseSession();

  // Updates the state, signaling if necessary.
  void SetState(State state);

  // Updates the error state, signaling if necessary.
  void SetError(Error error);

  // Handles messages posted to us.
  virtual void OnMessage(talk_base::Message *pmsg);

  // Returns the current state of the session.  See the enum above for details.
  // Each time the state changes, we will fire this signal.
  State state() const { return state_; }
  sigslot::signal2<BaseSession *, State> SignalState;

  // Returns the last error in the session.  See the enum above for details.
  // Each time the an error occurs, we will fire this signal.
  Error error() const { return error_; }
  sigslot::signal2<BaseSession *, Error> SignalError;

  // Creates a new channel with the given name.  This method may be called
  // immediately after creating the session.  However, the actual
  // implementation may not be fixed until transport negotiation completes.
  virtual TransportChannel* CreateChannel(const std::string& name) = 0;

  // Returns the channel with the given name.
  virtual TransportChannel* GetChannel(const std::string& name) = 0;

  // Destroys the given channel.
  virtual void DestroyChannel(TransportChannel* channel) = 0;

  // Invoked when we notice that there is no matching channel on our peer.
  sigslot::signal2<Session*, const std::string&> SignalChannelGone;

  // Returns the application-level description given by our client.
  // If we are the recipient, this will be NULL until we send an accept.
  const SessionDescription* local_description() const {
    return local_description_;
  }
  // Takes ownership of SessionDescription*
  bool set_local_description(const SessionDescription* sdesc) {
    if (sdesc != local_description_) {
      delete local_description_;
      local_description_ = sdesc;
    }
    return true;
  }

  // Returns the application-level description given by the other client.
  // If we are the initiator, this will be NULL until we receive an accept.
  const SessionDescription* remote_description() const {
    return remote_description_;
  }
  // Takes ownership of SessionDescription*
  bool set_remote_description(const SessionDescription* sdesc) {
    if (sdesc != remote_description_) {
      delete remote_description_;
      remote_description_ = sdesc;
    }
    return true;
  }

  // When we receive an initiate, we create a session in the
  // RECEIVEDINITIATE state and respond by accepting or rejecting.
  // Takes ownership of session description.
  virtual bool Accept(const SessionDescription* sdesc) = 0;
  virtual bool Reject() = 0;

  // At any time, we may terminate an outstanding session.
  virtual bool Terminate() = 0;

  // The worker thread used by the session manager
  virtual talk_base::Thread *worker_thread() = 0;

  // Returns the JID of this client.
  const std::string &name() const { return name_; }

  // Returns the JID of the other peer in this session.
  const std::string &remote_name() const { return remote_name_; }

  // Set the JID of the other peer in this session.
  // Typically the remote_name_ is set when the session is initiated.
  // However, sometimes (e.g when a proxy is used) the peer name is
  // known after the BaseSession has been initiated and it must be updated
  // explicitly.
  void set_remote_name(const std::string& name) { remote_name_ = name; }

  // Holds the ID of this session, which should be unique across the world.
  const SessionID& id() const { return id_; }

 protected:
  State state_;
  Error error_;
  const SessionDescription* local_description_;
  const SessionDescription* remote_description_;
  SessionID id_;
  // We don't use buzz::Jid because changing to buzz:Jid here has a
  // cascading effect that requires an enormous number places to
  // change to buzz::Jid as well.
  std::string name_;

  std::string remote_name_;
  talk_base::Thread *signaling_thread_;
};

// A specific Session created by the SessionManager, using XMPP for protocol.
class Session : public BaseSession {
 public:
  // Returns the manager that created and owns this session.
  SessionManager* session_manager() const { return session_manager_; }

  // the worker thread used by the session manager
  talk_base::Thread *worker_thread() {
    return session_manager_->worker_thread();
  }

  // Returns the XML namespace identifying the type of this session.
  const std::string& content_type() const { return content_type_; }

  // Returns the client that is handling the application data of this session.
  SessionClient* client() const { return client_; }

  // Indicates whether we initiated this session.
  bool initiator() const { return initiator_; }

  // Fired whenever we receive a terminate message along with a reason
  sigslot::signal2<Session*, const std::string&> SignalReceivedTerminateReason;

  // Returns the transport that has been negotiated or NULL if negotiation is
  // still in progress.
  Transport* transport() const { return transport_; }

  // Takes ownership of session description.
  bool Initiate(const std::string& to,
                const SessionDescription* sdesc);

  // When we receive an initiate, we create a session in the
  // RECEIVEDINITIATE state and respond by accepting or rejecting.
  // Takes ownership of session description.
  virtual bool Accept(const SessionDescription* sdesc);
  virtual bool Reject();

  // At any time, we may terminate an outstanding session.
  virtual bool Terminate();

  // The two clients in the session may also send one another arbitrary XML
  // messages, which are called "info" messages.  Both of these functions take
  // ownership of the XmlElements and delete them when done.
  void SendInfoMessage(const XmlElements& elems);
  sigslot::signal2<Session*, const XmlElements&> SignalInfoMessage;

  // Maps passed to serialization functions.
  TransportParserMap GetTransportParsers();
  ContentParserMap GetContentParsers();

  // Creates a new channel with the given name.  This method may be called
  // immediately after creating the session.  However, the actual
  // implementation may not be fixed until transport negotiation completes.
  virtual TransportChannel* CreateChannel(const std::string& name);

  // Returns the channel with the given name.
  virtual TransportChannel* GetChannel(const std::string& name);

  // Destroys the given channel.
  virtual void DestroyChannel(TransportChannel* channel);

  // Handles messages posted to us.
  virtual void OnMessage(talk_base::Message *pmsg);

 private:
  typedef std::map<std::string, TransportChannelProxy*> ChannelMap;

  SessionManager *session_manager_;
  bool initiator_;
  std::string content_type_;
  SessionClient* client_;
  // TODO(pthatcher): reenable redirect the Jingle way
  // std::string redirect_target_;

  Transport* transport_;
  bool transport_negotiated_;
  // in order to resend candidates, we need to know what we sent.
  Candidates sent_candidates_;
  ChannelMap channels_;
  // Keeps track of what protocol we are speaking.  This was
  // previously done using "compatibility_mode_".  Now
  // "compatibility_mode_" is when the protocol is PROTOCOL_GINGLE.
  // But, it's no longer a binary value, since we can have
  // PROTOCOL_JINGLE and PROTOCOL_HYBRID.
  SignalingProtocol current_protocol_;

  // Creates or destroys a session.  (These are called only SessionManager.)
  Session(SessionManager *session_manager,
          const std::string& name,
          const SessionID& id,
          const std::string& content_type,
          SessionClient* client);
  ~Session();

  // To improve connection time, this creates the channels on the most common
  // transport type and initiates connection.
  void ConnectDefaultTransportChannels(Transport* transport);
  void ConnectTransportChannels(Transport* transport);

  void SetTransport(Transport* transport);

  // Called when the first channel of a transport begins connecting.  We use
  // this to start a timer, to make sure that the connection completes in a
  // reasonable amount of time.
  void OnTransportConnecting(Transport* transport);

  // Called when a transport changes its writable state.  We track this to make
  // sure that the transport becomes writable within a reasonable amount of
  // time.  If this does not occur, we signal an error.
  void OnTransportWritable(Transport* transport);

  // Called when a transport requests signaling.
  void OnTransportRequestSignaling(Transport* transport);

  // Called when a transport signals that it has a message to send.   Note that
  // these messages are just the transport part of the stanza; they need to be
  // wrapped in the appropriate session tags.
  void OnTransportCandidatesReady(Transport* transport,
                                  const Candidates& candidates);

  // Called when a transport signals that it found an error in an incoming
  // message.
  void OnTransportSendError(Transport* transport,
                            const buzz::XmlElement* stanza,
                            const buzz::QName& name,
                            const std::string& type,
                            const std::string& text,
                            const buzz::XmlElement* extra_info);

  // Called when we notice that one of our local channels has no peer, so it
  // should be destroyed.
  void OnTransportChannelGone(Transport* transport, const std::string& name);

  // When the session needs to send signaling messages, it beings by requesting
  // signaling.  The client should handle this by calling OnSignalingReady once
  // it is ready to send the messages.
  // (These are called only by SessionManager.)
  sigslot::signal1<Session*> SignalRequestSignaling;
  void OnSignalingReady();

  // Send various kinds of session messages.
  void SendInitiateMessage(const SessionDescription* sdesc);
  void SendAcceptMessage(const SessionDescription* sdesc);
  void SendRejectMessage();
  void SendTerminateMessage();
  void SendTransportInfoMessage(const Candidates& candidates);

  // Sends a message of the given type to the other client.
  void SendMessage(ActionType type, const XmlElements& action_elems);

  // Sends a message back to the other client indicating that we have received
  // and accepted their message.
  void SendAcknowledgementMessage(const buzz::XmlElement* stanza);

  // Once signaling is ready, the session will use this signal to request the
  // sending of each message.  When messages are received by the other client,
  // they should be handed to OnIncomingMessage.
  // (These are called only by SessionManager.)
  sigslot::signal2<Session *, const buzz::XmlElement*> SignalOutgoingMessage;
  void OnIncomingMessage(const SessionMessage& msg);

  void OnFailedSend(const buzz::XmlElement* orig_stanza,
                    const buzz::XmlElement* error_stanza);

  // Invoked when an error is found in an incoming message.  This is translated
  // into the appropriate XMPP response by SessionManager.
  sigslot::signal6<BaseSession*,
                   const buzz::XmlElement*,
                   const buzz::QName&,
                   const std::string&,
                   const std::string&,
                   const buzz::XmlElement*> SignalErrorMessage;

  // Handlers for the various types of messages.  These functions may take
  // pointers to the whole stanza or to just the session element.
  bool OnInitiateMessage(const SessionMessage& msg, SessionError* error);
  bool OnAcceptMessage(const SessionMessage& msg, SessionError* error);
  bool OnRejectMessage(const SessionMessage& msg, SessionError* error);
  bool OnInfoMessage(const SessionMessage& msg);
  bool OnTerminateMessage(const SessionMessage& msg, SessionError* error);
  bool OnTransportInfoMessage(const SessionMessage& msg, SessionError* error);
  bool OnTransportAcceptMessage(const SessionMessage& msg, SessionError* error);

  // Verifies that we are in the appropriate state to receive this message.
  bool CheckState(State state, SessionError* error);

  friend class SessionManager;  // For access to constructor, destructor,
                                // and signaling related methods.
};

}  // namespace cricket

#endif  // TALK_P2P_BASE_SESSION_H_
