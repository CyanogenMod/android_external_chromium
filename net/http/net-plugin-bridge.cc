/*
* Copyright (c) 2011, 2012 The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "net/http/http_cache_transaction.h"

#include "build/build_config.h"

#include <unistd.h>

#include <string>

#include "base/compiler_specific.h"
#include "net/http/http_response_headers.h"
#include "net/http/preconnect.h"

#include "net/http/net-plugin-bridge.h"
#include "net/http/net-plugin-bridge-exports.h"
#include "net/host_resolver_helper/dyn_lib_loader.h"
#include <dlfcn.h>

static void (*DoObserveRevalidation)(const net::HttpResponseInfo* resp,
    const net::HttpRequestInfo* req, net::HttpCache* cache) = NULL;

static void InitOnce() {
  static bool initialized = false;
  if (!initialized) {
      initialized = true;
      void* fh = LibraryManager::GetLibraryHandle("qnet-plugin.so");
      if (fh) {
          *(void **)(&DoObserveRevalidation) = LibraryManager::GetLibrarySymbol(fh, "DoObserveRevalidation");
      }
  }
}

void ObserveRevalidation(const net::HttpResponseInfo* resp,
    const net::HttpRequestInfo* req, net::HttpCache* cache) {
  InitOnce();
  if (DoObserveRevalidation) {
      DoObserveRevalidation(resp, req, cache);
  }
}

bool HeadersIsRedirect(const net::HttpResponseHeaders* headers,
    std::string& location) {
  return headers->IsRedirect(&location);
}

GURL GurlResolveOrigin(const net::HttpRequestInfo* req,
    std::string& location) {
  return req->url.Resolve(location).GetOrigin();
}

GURL GurlOrigin(const net::HttpRequestInfo* req) {
  return req->url.GetOrigin();
}

void NetPreconnect(net::HttpNetworkSession* session, GURL const& url) {
  net::Preconnect::DoPreconnect(session, url);
}
