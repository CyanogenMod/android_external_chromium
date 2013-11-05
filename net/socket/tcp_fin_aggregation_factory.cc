// Copyright (c) 2011, The Linux Foundation. All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of The Linux Foundation nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "tcp_fin_aggregation_factory.h"
#include "tcp_fin_aggregation_bridge.h"
#include <dlfcn.h>
#include <cutils/log.h>

namespace net {

base::Lock TCPFinAggregationFactory::m_mutex;

TCPFinAggregationFactory* TCPFinAggregationFactory::s_pFactory = NULL;

TCPFinAggregationFactory* TCPFinAggregationFactory::GetTCPFinFactoryInstance(internal::ClientSocketPoolBaseHelper* pool_base_helper) {
  base::AutoLock myLock(TCPFinAggregationFactory::m_mutex);
  if(s_pFactory == NULL) {
    s_pFactory = new TCPFinAggregationFactory(pool_base_helper);
  }
  return s_pFactory;
}

TCPFinAggregationFactory::TCPFinAggregationFactory(internal::ClientSocketPoolBaseHelper* pool_base_helper):m_pTCPFin(NULL) {
  InitTCPFinAggregation(pool_base_helper);
}

void TCPFinAggregationFactory::InitTCPFinAggregation(internal::ClientSocketPoolBaseHelper* pool_base_helper) {
  void* libHandle = dlopen("libtcpfinaggr.so", RTLD_LAZY);
  if (!libHandle)
  {
    SLOGD("dl error message %s", dlerror());
  }

  if(libHandle) {
    SLOGD("%s: libtcpfinaggr.so successfully loaded", __FILE__);
    *(void **)(&tcpfin_create_) = dlsym(libHandle, "createTCPFinAggregation");

    if(tcpfin_create_) {
      SLOGD("%s,: TCP Fin Aggregation initializing method was found in libtcpfinaggr.so", __FILE__);
      m_pTCPFin = tcpfin_create_(pool_base_helper);
      return;
    }
    ::dlclose(libHandle);
    SLOGD("Failed to load createTCPFinAggregation symbol in libtcpfinaggr.so");
  }
}
}; // namespace net

void DecrementIdleCount(net::internal::ClientSocketPoolBaseHelper* pool_base_helper)
{
  pool_base_helper->DecrementIdleCount();
}
void RemoveGroup(net::internal::ClientSocketPoolBaseHelper* pool_base_helper, const std::string& group_name)
{
  pool_base_helper->RemoveGroup(group_name);
}
bool ShouldCleanup(net::internal::IdleSocket* idle_socket, base::Time now, base::TimeDelta timeout)
{
  return idle_socket->ShouldCleanup(now, timeout);
}
base::Time GetCurrentTime()
{
  return base::Time::Now();
}
