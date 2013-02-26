// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Copyright (c) 2011, The Linux Foundation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_PRECONNECT_H_
#define CHROME_BROWSER_NET_PRECONNECT_H_
#pragma once

#include "net/http/preconnect.h"

#include "chrome/browser/net/url_info.h"

namespace chrome_browser_net {

void PreconnectOnUIThread(const GURL& url,
                          UrlInfo::ResolutionMotivation motivation,
                          int count);

// Try to preconnect.  Typically used by predictor when a subresource probably
// needs a connection. |count| may be used to request more than one connection
// be established in parallel.
void PreconnectOnIOThread(const GURL& url,
                          UrlInfo::ResolutionMotivation motivation,
                          int count);
}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_PRECONNECT_H_
