/*
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
 */
#include "net/socket/client_socket_pool_extend_bridge.h"
#include "net/socket/client_socket_handle.h"

namespace net {
//static
bool ClientSocketPoolExtendBridge::AssignIdleSocketToGroup(
    internal::ClientSocketPoolBaseHelper* pool,
    const internal::ClientSocketPoolBaseHelper::Request* request,
    internal::ClientSocketPoolBaseHelper::Group* group,
    bool avoid_used_sockets) {
  return pool->AssignIdleSocketToGroup(request, group, avoid_used_sockets);
}

//static
ClientSocket* ClientSocketPoolExtendBridge::GetRequestSocket(const internal::ClientSocketPoolBaseHelper::Request* request) {
  return request->handle()->socket();
}

//static
const internal::ClientSocketPoolBaseHelper::Request* ClientSocketPoolExtendBridge::GetPendingRequest(
    internal::ClientSocketPoolBaseHelper::Group* group) {
  if (group->pending_requests().empty()) {
    return NULL;
  }
  const internal::ClientSocketPoolBaseHelper::Request* request = *group->pending_requests().begin();
  if (IsRequestSharable(request)) {
    return request;
  } else {
    return NULL;
  }
}

//static
bool ClientSocketPoolExtendBridge::IsRequestSharable(const internal::ClientSocketPoolBaseHelper::Request* request) {
  return (request->handle()
          && !(request->flags() & internal::ClientSocketPoolBaseHelper::NO_IDLE_SOCKETS));
}

//static
bool ClientSocketPoolExtendBridge::IsGroupBetterPriority(
    internal::ClientSocketPoolBaseHelper::Group* group,
    internal::ClientSocketPoolBaseHelper::Group* other_group) {
  // Compare the |TopPendingPriority| of two groups
  // Empty pending request queue is worst priority
  DCHECK(group);
  DCHECK(other_group);
  if (group->pending_requests().empty()) {
    return false;
  }
  if (other_group->pending_requests().empty()) {
    return true;
  }
  return (group->TopPendingPriority() < other_group->TopPendingPriority());
}

//static
void ClientSocketPoolExtendBridge::RemoveProcessedPendingRequest(
    internal::ClientSocketPoolBaseHelper* pool,
    const std::string& group_name,
    internal::ClientSocketPoolBaseHelper::Group* group) {
  pool->RemoveProcessedPendingRequest(group_name, group, OK);
}

} //namespace net


