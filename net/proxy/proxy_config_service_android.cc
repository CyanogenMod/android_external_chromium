// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy/proxy_config_service_android.h"

#include "net/proxy/proxy_config.h"

namespace net {

void ProxyConfigServiceAndroid::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ProxyConfigServiceAndroid::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool ProxyConfigServiceAndroid::GetLatestProxyConfig(ProxyConfig* config) {
  if (!config)
    return false;

  if (m_proxy.empty()) {
    *config = ProxyConfig::CreateDirect();
  } else {
    config->proxy_rules().ParseFromString(m_proxy);
  }
  return true;
}

void ProxyConfigServiceAndroid::UpdateProxySettings(std::string& proxy) {
  if (proxy == m_proxy)
    return;

  m_proxy = proxy;
  ProxyConfig config;
  config.proxy_rules().ParseFromString(m_proxy);
  FOR_EACH_OBSERVER(Observer, observers_, OnProxyConfigChanged(config));
}

} // namespace net
