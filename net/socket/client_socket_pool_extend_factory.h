/**
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other *materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation, nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.

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
 **/

#ifndef CLIENT_SOCKET_POOL_EXTEND_FACTORY_H_
#define CLIENT_SOCKET_POOL_EXTEND_FACTORY_H_

#include <string>
#include "net/base/address_list.h"
#include "net/socket/client_socket_pool_base.h"
#include "net/socket/client_socket.h"

namespace net
{

//Main ClientSocketPoolExtend interface
class IClientSocketPoolExtend
{
public:
  IClientSocketPoolExtend(){};
  virtual ~IClientSocketPoolExtend(){};
  virtual void OnConnectJobResolved(
      int result,
      const std::string& group_name,
      net::internal::ClientSocketPoolBaseHelper::Group* group,
      AddressList* addrlist,
      bool allow_reorder) = 0;
  virtual bool AssignIdleSocketToGroup(
      const internal::ClientSocketPoolBaseHelper::Request* request,
      const std::string& group_name,
      internal::ClientSocketPoolBaseHelper::Group* group) = 0;
  virtual std::string ReleaseSocket(
      const ClientSocket* socket,
      const std::string& group_name) = 0;
  virtual void RemoveGroup(
      const std::string& group_name,
      internal::ClientSocketPoolBaseHelper::Group* group) = 0;
  virtual void ProcessPendingRequest(
      const std::string& group_name,
      internal::ClientSocketPoolBaseHelper::Group* group) = 0;

private:
  DISALLOW_COPY_AND_ASSIGN(IClientSocketPoolExtend);
};

//Manager ClientSocketPoolExtend interface
class IClientSocketPoolExtendManager
{
public:
  IClientSocketPoolExtendManager() {};
  virtual ~IClientSocketPoolExtendManager() {};

  // factory call once per pool. Should be deleted when done
  virtual IClientSocketPoolExtend* GetClientSocketPoolExtend(internal::ClientSocketPoolBaseHelper* pool) = 0;

  virtual bool initialize() = 0;
  virtual void finalize() = 0;

private:
  DISALLOW_COPY_AND_ASSIGN(IClientSocketPoolExtendManager);
};


class ClientSocketPoolExtendFactory {
public:
  static ClientSocketPoolExtendFactory* GetInstance();
  IClientSocketPoolExtendManager* GetManager();
  static IClientSocketPoolExtendManager* GetDummy();

private:
  ClientSocketPoolExtendFactory();
  ~ClientSocketPoolExtendFactory();
  IClientSocketPoolExtendManager *manager_;
  const char *library_name_;
  bool is_manager_initialized_;
};

}; //end namespace net
#endif /* CLIENT_SOCKET_POOL_EXTEND_FACTORY_H_ */
