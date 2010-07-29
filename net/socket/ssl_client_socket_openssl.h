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

#ifndef NET_SOCKET_SSL_CLIENT_SOCKET_OPENSSL_H_
#define NET_SOCKET_SSL_CLIENT_SOCKET_OPENSSL_H_

#include "base/scoped_ptr.h"
#include "net/base/io_buffer.h"
#include "net/base/ssl_config_service.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/client_socket_handle.h"

#include "openssl/ssl.h"

namespace net {

class SSLCertRequestInfo;
class SSLConfig;
class SSLInfo;

// An SSL client socket implemented with OpenSSL.
class SSLClientSocketOpenSSL : public SSLClientSocket {
 public:
  // Takes ownership of the transport_socket, which may already be connected.
  // The given hostname will be compared with the name(s) in the server's
  // certificate during the SSL handshake.  ssl_config specifies the SSL
  // settings.
  SSLClientSocketOpenSSL(ClientSocketHandle* transport_socket,
                     const std::string& hostname,
                     const SSLConfig& ssl_config);
  ~SSLClientSocketOpenSSL();

  // SSLClientSocket methods:
  virtual void GetSSLInfo(SSLInfo* ssl_info);
  virtual void GetSSLCertRequestInfo(SSLCertRequestInfo* cert_request_info);
  virtual NextProtoStatus GetNextProto(std::string* proto);

  // ClientSocket methods:
  virtual int Connect(CompletionCallback* callback);
  virtual void Disconnect();
  virtual bool IsConnected() const;
  virtual bool IsConnectedAndIdle() const;
  virtual int GetPeerAddress(AddressList*) const;
  virtual const BoundNetLog& NetLog() const;

  // Socket methods:
  virtual int Read(IOBuffer* buf, int buf_len, CompletionCallback* callback);
  virtual int Write(IOBuffer* buf, int buf_len, CompletionCallback* callback);
  virtual bool SetReceiveBufferSize(int32 size);
  virtual bool SetSendBufferSize(int32 size);

 private:
  // Initializes OpenSSL SSL options.  Returns a net error code.
  bool InitializeSSLOptions();
  bool InitOpenSSL();
  bool Init();
  void DoReadCallback(int result);
  void DoWriteCallback(int result);

  bool DoTransportIO();
  int DoHandshake();
  void DoConnectCallback(int result);

  void OnHandshakeIOComplete(int result);
  void OnSendComplete(int result);
  void OnRecvComplete(int result);

  int DoHandshakeLoop(int last_io_result);
  int DoReadLoop(int result);
  int DoWriteLoop(int result);
  int DoPayloadRead();
  int DoPayloadWrite();

  int BufferSend(void);
  int BufferRecv(void);
  void BufferSendComplete(int result);
  void BufferRecvComplete(int result);

  CompletionCallbackImpl<SSLClientSocketOpenSSL> *buffer_send_callback_;
  CompletionCallbackImpl<SSLClientSocketOpenSSL> *buffer_recv_callback_;
  bool transport_send_busy_;
  bool transport_recv_busy_;
  scoped_refptr<IOBuffer> recv_buffer_;

  CompletionCallback* user_connect_callback_;
  CompletionCallback* user_read_callback_;
  CompletionCallback* user_write_callback_;

  // Used by Read function.
  scoped_refptr<IOBuffer> user_read_buf_;
  int user_read_buf_len_;

  // Used by Write function.
  scoped_refptr<IOBuffer> user_write_buf_;
  int user_write_buf_len_;

  // Stores client authentication information between ClientAuthHandler and
  // GetSSLCertRequestInfo calls.
  std::vector<scoped_refptr<X509Certificate> > client_certs_;
  bool client_auth_cert_needed_;
  
  // OpenSSL stuff
  static SSL_CTX* ctx;
  SSL* ssl;
  BIO* bio_read;
  BIO* bio_write;
  size_t write_buf_size;

  scoped_ptr<ClientSocketHandle> transport_;
  std::string hostname_;
  SSLConfig ssl_config_;

  bool completed_handshake_;

  enum State {
    STATE_NONE,
    STATE_HANDSHAKE,
    STATE_VERIFY_CERT,
    STATE_VERIFY_CERT_COMPLETE,
  };
  State next_handshake_state_;
  BoundNetLog net_log_;
};

}  // namespace net

#endif // NET_SOCKET_SSL_CLIENT_SOCKET_OPENSSL_H_

