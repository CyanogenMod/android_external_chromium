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

#ifndef TCP_FIN_AGGREGATION_FACTORY_H_
#define TCP_FIN_AGGREGATION_FACTORY_H_

#include "tcp_fin_aggregation.h"
#include "client_socket_pool_base.h"
#include "base/synchronization/lock.h"
#include "tcp_fin_aggregation_bridge.h"

namespace net {
class ITCPFinAggregation;
namespace internal {
  class ClientSocketPoolBaseHelper;
}

static ITCPFinAggregation* (*tcpfin_create_)(internal::ClientSocketPoolBaseHelper* pool_base_helper) = NULL;

class TCPFinAggregationFactory {

public:


  static TCPFinAggregationFactory* GetTCPFinFactoryInstance(internal::ClientSocketPoolBaseHelper* pool_base_helper);

  ITCPFinAggregation* GetTCPFinAggregation(){ return m_pTCPFin;}

private:

  ITCPFinAggregation* m_pTCPFin;

  static TCPFinAggregationFactory* s_pFactory;

  static base::Lock m_mutex;

  TCPFinAggregationFactory(internal::ClientSocketPoolBaseHelper* pool_base_helper);

  ~TCPFinAggregationFactory();

  void InitTCPFinAggregation(internal::ClientSocketPoolBaseHelper* pool_base_helper);

  DISALLOW_COPY_AND_ASSIGN(TCPFinAggregationFactory);
};

} // namespace net
#endif
