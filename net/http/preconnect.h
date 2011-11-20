// Copyright (c) 2006-2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A Preconnect instance maintains state while a TCP/IP connection is made, and
// and then released into the pool of available connections for future use.

#ifndef NET_HTTP_PRECONNECT_H__
#define NET_HTTP_PRECONNECT_H__
#pragma once

#include "base/scoped_ptr.h"
#include "net/base/completion_callback.h"
#include "net/base/net_log.h"
#include "net/http/http_request_info.h"
#include "net/http/http_stream_factory.h"

namespace net {

class ProxyInfo;
struct SSLConfig;

class Preconnect {
 public:
  // Try to preconnect.  Typically used by predictor when a subresource probably
  // needs a connection. |count| may be used to request more than one connection
  // be established in parallel.
  static void DoPreconnect(HttpNetworkSession* session, const GURL& url,
      int count = 1, HttpRequestInfo::RequestMotivation motivation =
          HttpRequestInfo::PRECONNECT_MOTIVATED);

 private:
  explicit Preconnect(HttpNetworkSession* session);
  virtual ~Preconnect();

  void OnPreconnectComplete(int error_code);

  // Request actual connection, via interface that tags request as needed for
  // preconnect only (so that they can be merged with connections needed for
  // navigations).
  void Connect(const GURL& url, int count,
      HttpRequestInfo::RequestMotivation motivation);

  HttpNetworkSession * session_;
  // HttpRequestInfo used for connecting.
  scoped_ptr<HttpRequestInfo> request_info_;

  // SSLConfig used for connecting.
  scoped_ptr<SSLConfig> ssl_config_;

  // ProxyInfo used for connecting.
  scoped_ptr<ProxyInfo> proxy_info_;

  // A net log to use for this preconnect.
  BoundNetLog net_log_;

  // Our preconnect.
  scoped_ptr<HttpStreamRequest> stream_request_;

  CompletionCallbackImpl<Preconnect> io_callback_;

  DISALLOW_COPY_AND_ASSIGN(Preconnect);
};
}  // namespace net
#endif // NET_HTTP_HTTP_REQUEST_INFO_H__

