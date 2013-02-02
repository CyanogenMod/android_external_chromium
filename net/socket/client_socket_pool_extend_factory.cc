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

#include "net/socket/client_socket_pool_extend_factory.h"
#include "net/socket/client_socket_pool_extend_bridge.h"
#include "net/socket/client_socket_handle.h"
#include "net/host_resolver_helper/dyn_lib_loader.h"
#include "net/base/net_errors.h"

#define LOG_TAG "SockPoolExtend"
#include <cutils/log.h>

const char* kLibraryName = "libsocketpoolextend.so";

namespace net
{

class ClientSocketPoolExtend : public net::IClientSocketPoolExtendManager, net::IClientSocketPoolExtend
{
public:
  ClientSocketPoolExtend() {
    SLOGI("Using default ClientSocketPoolExtend");
  }
  ~ClientSocketPoolExtend() {};

  IClientSocketPoolExtend* GetClientSocketPoolExtend(internal::ClientSocketPoolBaseHelper* pool) {
    return this;
  }

  bool initialize() {
    return false;
  }

  void finalize() {
  }

  void OnConnectJobResolved(
      int result,
      const std::string& group_name,
      internal::ClientSocketPoolBaseHelper::Group* group,
      AddressList* addrlist,
      bool allow_reorder) {
  }

  bool AssignIdleSocketToGroup(
      const internal::ClientSocketPoolBaseHelper::Request* request,
      const std::string& group_name,
      internal::ClientSocketPoolBaseHelper::Group* group) {
    return false;
  }

  std::string ReleaseSocket(const ClientSocket* socket, const std::string& group_name) {
    return group_name;
  }

  void RemoveGroup(const std::string& group_name, internal::ClientSocketPoolBaseHelper::Group* group) {
  }

  void ProcessPendingRequest(
      const std::string& group_name,
      internal::ClientSocketPoolBaseHelper::Group* group) {
  }
};

// static
ClientSocketPoolExtendFactory* ClientSocketPoolExtendFactory::GetInstance() {
  static ClientSocketPoolExtendFactory factory;
  return &factory;
}

//static
IClientSocketPoolExtendManager* ClientSocketPoolExtendFactory::GetDummy() {
  static ClientSocketPoolExtend dummy;
  return &dummy;
}

ClientSocketPoolExtendFactory::ClientSocketPoolExtendFactory() :
      manager_(NULL),
      library_name_(NULL),
      is_manager_initialized_(false)
{
}

IClientSocketPoolExtendManager* ClientSocketPoolExtendFactory::GetManager() {
  if (is_manager_initialized_) {
    return manager_;
  }

  manager_ = GET_DYNAMIC_OBJECT_INTERFACE_PTR(kLibraryName,IClientSocketPoolExtendManager);
  if (manager_) {
    library_name_ = kLibraryName;
    SLOGI("%s successfully loaded", library_name_);
    is_manager_initialized_ = manager_->initialize();
    if (is_manager_initialized_) {
      SLOGI("%s is ready", library_name_);
      return manager_;
    } else {
      manager_ = NULL;
      SLOGI("%s could not start, unloading", library_name_);
      LibraryManager::ReleaseLibraryHandle(library_name_);
      library_name_ = NULL;
    }
  }
  manager_ = GetDummy();
  is_manager_initialized_ = true;
  return manager_;
}

ClientSocketPoolExtendFactory::~ClientSocketPoolExtendFactory() {
  if (is_manager_initialized_ && manager_) {
    manager_->finalize();
  }
  if (library_name_ ) {
    LibraryManager::ReleaseLibraryHandle(library_name_);
    SLOGI("%s is unloaded", library_name_);
  } else {
    // dummy. do nothing
  }
}

}; //end namespace network

