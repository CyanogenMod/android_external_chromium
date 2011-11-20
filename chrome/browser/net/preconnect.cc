// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Copyright (c) 2011, 2012 Code Aurora Forum. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/preconnect.h"

#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/browser_thread.h"
#include "net/base/net_log.h"
#include "net/base/ssl_config_service.h"
#include "net/http/http_network_session.h"
#include "net/http/http_request_info.h"
#include "net/http/http_stream_factory.h"
#include "net/http/http_transaction_factory.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace chrome_browser_net {

void PreconnectOnUIThread(
    const GURL& url,
    UrlInfo::ResolutionMotivation motivation,
    int count) {
  // Prewarm connection to Search URL.
  BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      NewRunnableFunction(PreconnectOnIOThread, url, motivation,
                          count));
  return;
}

void PreconnectOnIOThread(
    const GURL& url,
    UrlInfo::ResolutionMotivation motivation,
    int count) {
  URLRequestContextGetter* getter = Profile::GetDefaultRequestContext();
  if (!getter)
    return;
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    LOG(DFATAL) << "This must be run only on the IO thread.";
    return;
  }

  // We are now commited to doing the async preconnection call.
  UMA_HISTOGRAM_ENUMERATION("Net.PreconnectMotivation", motivation,
                            UrlInfo::MAX_MOTIVATED);

  net::URLRequestContext* context = getter->GetURLRequestContext();
  net::HttpTransactionFactory* factory = context->http_transaction_factory();
  net::HttpNetworkSession* session = factory->GetSession();

  // Translate the motivation from UrlRequest motivations to HttpRequest
  // motivations.
  net::HttpRequestInfo::RequestMotivation motivation_;
  switch (motivation) {
    case UrlInfo::OMNIBOX_MOTIVATED:
      request_info.motivation = net::HttpRequestInfo::OMNIBOX_MOTIVATED;
      break;
    case UrlInfo::LEARNED_REFERAL_MOTIVATED:
      request_info.motivation = net::HttpRequestInfo::PRECONNECT_MOTIVATED;
      break;
    case UrlInfo::SELF_REFERAL_MOTIVATED:
    case UrlInfo::EARLY_LOAD_MOTIVATED:
      request_info.motivation = net::HttpRequestInfo::EARLY_LOAD_MOTIVATED;
      break;
    default:
      // Other motivations should never happen here.
      NOTREACHED();
      break;
  }

  net::Preconnect::DoPreconnect(session, url, count, motivation_);
}

}  // namespace chrome_browser_net
