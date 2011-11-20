// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Copyright (c) 2012, Code Aurora Forum. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/request_priority.h"
#include "base/logging.h"

namespace net {

RequestPriority DetermineRequestPriority(ResourceType::Type type)
{
  //__android_log_print(ANDROID_LOG_VERBOSE, "Preload",  "DetermineRequestPriority, type = %d", type);

  // Determine request priority based on how critical this resource typically
  // is to user-perceived page load performance. Important considerations are:
  // * Can this resource block the download of other resources.
  // * Can this resource block the rendering of the page.
  // * How useful is the page to the user if this resource is not loaded yet.
  switch (type) {
    // Main frames are the highest priority because they can block nearly every
    // type of other resource and there is no useful display without them.
    // Sub frames are a close second, however it is a common pattern to wrap
    // ads in an iframe or even in multiple nested iframes. It is worth
    // investigating if there is a better priority for them.
    case ResourceType::MAIN_FRAME:
    case ResourceType::SUB_FRAME:
      return HIGHEST;

    // Stylesheets and scripts can block rendering and loading of other
    // resources. Fonts can block text from rendering.
    case ResourceType::STYLESHEET:
    case ResourceType::SCRIPT:
    case ResourceType::FONT_RESOURCE:
      return MEDIUM;

    // Sub resources, objects and media are lower priority than potentially
    // blocking stylesheets, scripts and fonts, but are higher priority than
    // images because if they exist they are probably more central to the page
    // focus than images on the page.
    case ResourceType::SUB_RESOURCE:
    case ResourceType::OBJECT:
    case ResourceType::MEDIA:
    case ResourceType::WORKER:
    case ResourceType::SHARED_WORKER:
      return LOW;

    // Images are the "lowest" priority because they typically do not block
    // downloads or rendering and most pages have some useful content without
    // them.
    case ResourceType::IMAGE:
      return LOWEST;

    // Prefetches are at a lower priority than even LOWEST, since they
    // are not even required for rendering of the current page.
    case ResourceType::PREFETCH:
      return IDLE;

    default:
      // When new resource types are added, their priority must be considered.
      NOTREACHED();
      return LOW;
  }
}

}  // namespace net

