/*
 * Copyright (C) 2011, 2012 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 *
 */

#include "build/build_config.h"

#include <unistd.h>

#include <string>

#include "base/compiler_specific.h"
#include "net/http/http_response_headers.h"
#include "net/http/preconnect.h"

#include "net/http/tcp-connections-bridge.h"
#include "net/http/tcp-connections-bridge-exports.h"
#include "net/http/http_network_session.h"
#include <dlfcn.h>
#include <cutils/log.h>

static void (*DoObserveConnections)(
    net::HttpNetworkSession* session,
    const GURL& url) = NULL;

static void InitOnce() {
  static bool initialized = false;
  if (!initialized) {
      initialized = true;
      void* fh = dlopen("tcp-connections.so", RTLD_LAZY);
      if (fh) {
          dlerror(); //see man dlopen
          *(void **)(&DoObserveConnections) = dlsym(fh, "DoObserveConnections");
      }
      if (NULL == DoObserveConnections) {
          SLOGD("Failed to load DoObserveConnections symbol in tcp-connections.so");
      }
  }
}

void ObserveConnections(
    net::HttpNetworkSession *session,
    const GURL& url
)
{
  InitOnce();
  if (DoObserveConnections) {
      DoObserveConnections(session, url);
  }
}

void NetPreconnect(net::HttpNetworkSession* session, GURL const& url, int numOfConnections) {
  net::Preconnect::DoPreconnect(session, url, numOfConnections);
}
