/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "net/socket/ssl_client_socket_openssl.h"

#include "net/base/net_errors.h"
#include "openssl/err.h"

namespace net {

#if 1
#define GotoState(s) next_handshake_state_ = s
#else
#define GotoState(s) do { LOG(INFO) << (void *)this << " " << __FUNCTION__ << \
                           " jump to state " << s; \
                           next_handshake_state_ = s; } while (0)
#endif


bool SSLClientSocketOpenSSL::InitOpenSSL() {
  if(ctx) {
    return true;
  }

  SSL_load_error_strings();
  if (!SSL_library_init())
    return false;

  const SSL_METHOD *pMethod = SSLv23_client_method();
  ctx = SSL_CTX_new(pMethod);

  if (!ctx) {
    return false;
  }

  SSL_CTX_set_verify(ctx, /*SSL_VERIFY_PEER*/ SSL_VERIFY_NONE, NULL/*callback*/); //void

  return true;
}

// Add some error checking ...
bool SSLClientSocketOpenSSL::Init() {
  if (!ctx) {
    return false;
  }

  ssl = SSL_new(ctx);

  bio_read = BIO_new(BIO_s_mem());
  bio_write = BIO_new(BIO_s_mem());

  write_buf_size = 1024; // Why does this not work? BIO_get_write_buf_size(bio_write, 1000);

  if (!SSL_set_tlsext_host_name(ssl, hostname_.c_str()))
    return false;

  SSL_set_bio(ssl, bio_read, bio_write); // void

  // Connect as client
  SSL_set_connect_state(ssl); // void

  return true;
}

SSL_CTX* SSLClientSocketOpenSSL::ctx = NULL;

SSLClientSocketOpenSSL::SSLClientSocketOpenSSL(ClientSocketHandle* transport_socket,
                                               const std::string& hostname,
                                               const SSLConfig& ssl_config)
    : transport_send_busy_(false),
      transport_recv_busy_(false),
      user_connect_callback_(NULL),
      user_read_callback_(NULL),
      user_write_callback_(NULL),
      client_auth_cert_needed_(false),
      transport_(transport_socket),
      hostname_(hostname),
      ssl_config_(ssl_config),
      completed_handshake_(false),
      net_log_(transport_socket->socket()->NetLog()) {
  buffer_send_callback_ = new CompletionCallbackImpl<SSLClientSocketOpenSSL>(
      this, &SSLClientSocketOpenSSL::BufferSendComplete);
  buffer_recv_callback_ = new CompletionCallbackImpl<SSLClientSocketOpenSSL>(
      this, &SSLClientSocketOpenSSL::BufferRecvComplete);
}

SSLClientSocketOpenSSL::~SSLClientSocketOpenSSL() {
  delete buffer_send_callback_;
  delete buffer_recv_callback_;

  Disconnect();
}


// SSLClientSocket methods

void SSLClientSocketOpenSSL::GetSSLInfo(SSLInfo* ssl_info) {
}

void SSLClientSocketOpenSSL::GetSSLCertRequestInfo(
    SSLCertRequestInfo* cert_request_info) {
}

SSLClientSocket::NextProtoStatus
SSLClientSocketOpenSSL::GetNextProto(std::string* proto) {
  return kNextProtoUnsupported;
}

void SSLClientSocketOpenSSL::DoReadCallback(int rv) {
  // Since Run may result in Read being called, clear |user_read_callback_|
  // up front.
  CompletionCallback* c = user_read_callback_;
  user_read_callback_ = NULL;
  user_read_buf_ = NULL;
  user_read_buf_len_ = 0;
  c->Run(rv);
}

void SSLClientSocketOpenSSL::DoWriteCallback(int rv) {
  // Since Run may result in Write being called, clear |user_write_callback_|
  // up front.
  CompletionCallback* c = user_write_callback_;
  user_write_callback_ = NULL;
  user_write_buf_ = NULL;
  user_write_buf_len_ = 0;
  c->Run(rv);
}

// ClientSocket methods

// Should make a proper return error
// and call the callback with more info
int SSLClientSocketOpenSSL::Connect(CompletionCallback* callback) {
  if(!InitOpenSSL()) {
    return -1; // Anything but OK signals error
  }

  // Set up new ssl object
  if(!Init()) {
    return -1;
  }

  // Handshake happens here
  int err = SSL_connect(ssl);

  GotoState(STATE_HANDSHAKE);
  int rv = DoHandshakeLoop(net::OK);
  if (rv == ERR_IO_PENDING) {
    user_connect_callback_ = callback;
  }

  // (Optional) Tell loadlog we are starting and store a ref
  return rv > OK ? OK : rv;
}

void SSLClientSocketOpenSSL::Disconnect() {
  // Null all callbacks
  // Delete all buffers
  // Handshake not completed anymore
  //verifier_.reset();
  completed_handshake_ = false;
  transport_->socket()->Disconnect();
}

int SSLClientSocketOpenSSL::DoHandshakeLoop(int last_io_result) {
  bool network_moved;
  int rv = last_io_result;
  do {
    // Default to STATE_NONE for next state.
    // (This is a quirk carried over from the windows
    // implementation.  It makes reading the logs a bit harder.)
    // State handlers can and often do call GotoState just
    // to stay in the current state.
    State state = next_handshake_state_;
    GotoState(STATE_NONE);
    switch (state) {
      case STATE_NONE:
        // we're just pumping data between the buffer and the network
        break;
      case STATE_HANDSHAKE:
        rv = DoHandshake();
        break;
      case STATE_VERIFY_CERT:
//        DCHECK(rv == OK);
//        rv = DoVerifyCert(rv);
//       break;
      case STATE_VERIFY_CERT_COMPLETE:
//        rv = DoVerifyCertComplete(rv);
//        break;
      default:
//        rv = ERR_UNEXPECTED;
//        NOTREACHED() << "unexpected state";
        break;
    }

    // To avoid getting an ERR_IO_PENDING here after handshake complete.
    if (next_handshake_state_ == STATE_NONE)
      break;

    // Do the actual network I/O
    network_moved = DoTransportIO();
  } while ((rv != ERR_IO_PENDING || network_moved) &&
            next_handshake_state_ != STATE_NONE);
  return rv;
}

int SSLClientSocketOpenSSL::DoHandshake() {
  int rv = SSL_do_handshake(ssl);

  if (rv == 1) {
    // SSL handshake is completed.  Let's verify the certificate.
    // For now we are done, certificates not implemented yet
    // TODO: Implement certificates
    rv = OK;
    GotoState(STATE_NONE);
    completed_handshake_ = true;
  } else {
    int ssl_error = SSL_get_error(ssl, rv);

    // If not done, stay in this state
    if (ssl_error == SSL_ERROR_WANT_READ) {
      rv = ERR_IO_PENDING;
      GotoState(STATE_HANDSHAKE);
    } else if (ssl_error == SSL_ERROR_WANT_WRITE) {
      rv = ERR_IO_PENDING;
      GotoState(STATE_HANDSHAKE);
    } else {
      GotoState(STATE_HANDSHAKE);
    }
  }

  return rv;
}

bool SSLClientSocketOpenSSL::DoTransportIO() {
  bool network_moved = false;
  int nsent = BufferSend();
  int nreceived = BufferRecv();
  network_moved = (nsent > 0 || nreceived >= 0);

  return network_moved;
}

int SSLClientSocketOpenSSL::BufferSend(void) {
  size_t data_pending = BIO_ctrl_pending(bio_write);
  if (data_pending == 0) {
    return NULL;
  }

  scoped_refptr<IOBuffer> send_buffer = new IOBuffer(data_pending);
  int bytes_read = BIO_read(bio_write, send_buffer->data(), data_pending);

  if (bytes_read > 0) {
    int rv;
    do {
      rv = transport_->socket()->Write(send_buffer, data_pending, buffer_send_callback_);
      if (rv == ERR_IO_PENDING) {
        transport_send_busy_ = true;
        break; // will get callback
      } else if (rv < 0) {
        // just resend and pray
        //android_printLog(ANDROID_LOG_ERROR, "https", "transport->write returned %d", rv);
      } else if (rv != bytes_read) { // TODO: what to do about the rest of data read from openssl?
        //android_printLog(ANDROID_LOG_ERROR, "https", "Only sent %d out of %d bytes", rv, bytes_read);
      }
    } while (rv < ERR_IO_PENDING); // -1
    return rv;
  } else {
    int err = SSL_get_error(ssl, bytes_read);
    //android_printLog(ANDROID_LOG_DEBUG, "https", "SSL error in BufferSend: %d / %d (%d)", err, bytes_read, data_pending);
    return 0;
  }
}

void SSLClientSocketOpenSSL::BufferSendComplete(int result) {
  transport_send_busy_ = false;
  OnSendComplete(result);
}

int SSLClientSocketOpenSSL::BufferRecv(void) {
  if (transport_recv_busy_)
    return ERR_IO_PENDING;

  int max_write = write_buf_size;
//  Why does this not work?
//  int max_write = BIO_ctrl_get_write_guarantee(bio_read);
//  int mw        = BIO_ctrl_get_write_guarantee(bio_write);
//  android_printLog(ANDROID_LOG_DEBUG, "https", "BufferRecv can write at most %d", max_write);
  
  int rv;
  if (!max_write) {
    rv = ERR_IO_PENDING;
  } else {
    recv_buffer_ = new IOBuffer(max_write);
    rv = transport_->socket()->Read(recv_buffer_, max_write, buffer_recv_callback_);
    if (rv == ERR_IO_PENDING) {
      transport_recv_busy_ = true;
    } else {
      if (rv > 0) {
        int ret = BIO_write(bio_read, recv_buffer_->data(), rv);
        if (ret != rv) {
          //android_printLog(ANDROID_LOG_ERROR, "https", "wrote less than capacity, should not happen (%d > %d : %d)", rv, ret, max_write);
        }
      } else {
        BIO_set_mem_eof_return(bio_read, 0);
        BIO_shutdown_wr(bio_read);
      }
      recv_buffer_ = NULL;
    }
  }
  return rv;
}

void SSLClientSocketOpenSSL::BufferRecvComplete(int result) {
  if (result > 0) {
    int ret = BIO_write(bio_read, recv_buffer_->data(), result);
    if (ret != result) {
      //android_printLog(ANDROID_LOG_ERROR, "https", "BIO_write returned: %d (not %d)", ret, result);
    }
  } else {
    BIO_set_mem_eof_return(bio_read, 0);
    BIO_shutdown_wr(bio_read);
  }
  recv_buffer_ = NULL;
  transport_recv_busy_ = false;
  OnRecvComplete(result);
}

void SSLClientSocketOpenSSL::DoConnectCallback(int rv) {
  CompletionCallback* c = user_connect_callback_;
  user_connect_callback_ = NULL;
  c->Run(rv > OK ? OK : rv);
}

void SSLClientSocketOpenSSL::OnHandshakeIOComplete(int result) {
  int rv = DoHandshakeLoop(result);
  if (rv != ERR_IO_PENDING) {
    DoConnectCallback(rv);
  }
}

void SSLClientSocketOpenSSL::OnSendComplete(int result) {
  if (next_handshake_state_ != STATE_NONE) {
    // In handshake phase.
    OnHandshakeIOComplete(result);
    return;
  }

  // OnSendComplete may need to call DoPayloadRead while the renegotiation
  // handshake is in progress.
  int rv_read = ERR_IO_PENDING;
  int rv_write = ERR_IO_PENDING;
  bool network_moved;
  do {
      if (user_read_buf_)
          rv_read = DoPayloadRead();
      if (user_write_buf_)
          rv_write = DoPayloadWrite();
      network_moved = DoTransportIO();
  } while (rv_read == ERR_IO_PENDING &&
           rv_write == ERR_IO_PENDING &&
           network_moved);

  if (user_read_buf_ && rv_read != ERR_IO_PENDING)
      DoReadCallback(rv_read);
  if (user_write_buf_ && rv_write != ERR_IO_PENDING)
      DoWriteCallback(rv_write);
}

void SSLClientSocketOpenSSL::OnRecvComplete(int result) {
  if (next_handshake_state_ != STATE_NONE) {
    // In handshake phase.
    OnHandshakeIOComplete(result);
    return;
  }

  // Network layer received some data, check if client requested to read
  // decrypted data.
  if (!user_read_buf_) {
    //android_printLog(ANDROID_LOG_ERROR, "https", "recv with no user_read_buf_");
    return;
  }

  int rv = DoReadLoop(result);
  if (rv != ERR_IO_PENDING)
    DoReadCallback(rv);
}

bool SSLClientSocketOpenSSL::IsConnected() const {
  bool ret = completed_handshake_ && transport_->socket()->IsConnected();
  return ret;
}

bool SSLClientSocketOpenSSL::IsConnectedAndIdle() const {
  bool ret = completed_handshake_ && transport_->socket()->IsConnectedAndIdle();
  return ret;
}

int SSLClientSocketOpenSSL::GetPeerAddress(AddressList* addressList) const {
  return transport_->socket()->GetPeerAddress(addressList);
}

const BoundNetLog& SSLClientSocketOpenSSL::NetLog() const {
  return net_log_; 
}

// Socket methods

int SSLClientSocketOpenSSL::Read(IOBuffer* buf, int buf_len, CompletionCallback* callback) {
  user_read_buf_ = buf;
  user_read_buf_len_ = buf_len;

  int rv = DoReadLoop(OK);

  if (rv == ERR_IO_PENDING)
    user_read_callback_ = callback;
  else {
    user_read_buf_ = NULL;
    user_read_buf_len_ = 0;
  }

  return rv;
}

int SSLClientSocketOpenSSL::DoReadLoop(int result) {
  if (result < 0)
    return result;

  bool network_moved;
  int rv;
  do {
    rv = DoPayloadRead();
    network_moved = DoTransportIO();
  } while (rv == ERR_IO_PENDING && network_moved);

  return rv;
}

int SSLClientSocketOpenSSL::Write(IOBuffer* buf, int buf_len, CompletionCallback* callback) {
  user_write_buf_ = buf;
  user_write_buf_len_ = buf_len;

  int rv = DoWriteLoop(OK);

  if (rv == ERR_IO_PENDING)
    user_write_callback_ = callback;
  else {
    user_write_buf_ = NULL;
    user_write_buf_len_ = 0;
  }

  return rv;
}

int SSLClientSocketOpenSSL::DoWriteLoop(int result) {
  if (result < 0)
    return result;

  bool network_moved;
  int rv;
  do {
    rv = DoPayloadWrite();
    network_moved = DoTransportIO();
  } while (rv == ERR_IO_PENDING && network_moved);

  return rv;
}

bool SSLClientSocketOpenSSL::SetReceiveBufferSize(int32 size) {
  return true;
}

bool SSLClientSocketOpenSSL::SetSendBufferSize(int32 size) {
  return true;
}


int SSLClientSocketOpenSSL::DoPayloadRead() {
  int rv = SSL_read(ssl, user_read_buf_->data(), user_read_buf_len_);
  if (client_auth_cert_needed_) {
    // We don't need to invalidate the non-client-authenticated SSL session
    // because the server will renegotiate anyway.
    return ERR_SSL_CLIENT_AUTH_CERT_NEEDED;
  }

  if (rv >= 0) {
    return rv;
  }

  int err = SSL_get_error(ssl, rv);
  if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
    // This will trigger another read/write after DoTransportIO
    return ERR_IO_PENDING;
  }

  return rv; // return err instead?
}

int SSLClientSocketOpenSSL::DoPayloadWrite() {
  int rv = SSL_write(ssl, user_write_buf_->data(), user_write_buf_len_);

  if (rv >= 0) {
    return rv;
  }

  int err = SSL_get_error(ssl, rv);
  if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
    // This will trigger another read/write after DoTransportIO
    return ERR_IO_PENDING;
  }

  return rv; // return err instead ?
}

} // namespace net
