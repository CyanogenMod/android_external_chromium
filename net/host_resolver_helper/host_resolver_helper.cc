/** ---------------------------------------------------------------------------
 Copyright (c) 2011, The Linux Foundation. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
       disclaimer in the documentation and/or other materials provided
       with the distribution.
     * Neither the name of The Linux Foundation nor the names of its
       contributors may be used to endorse or promote products derived
       from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 -----------------------------------------------------------------------------**/
#include "host_resolver_helper.h"

#include <config.h>
#include <unistd.h>

#include "net/base/address_list.h"
#include "net/base/host_port_pair.h"
#include "net/base/completion_callback.h"
#include "net/base/host_port_pair.h"
#include "net/base/host_resolver.h"
#include "net/base/net_errors.h"
#include "net/base/net_log.h"
#include <cutils/properties.h>
#include "dyn_lib_loader.h"

#define NUM_HOSTS_TO_RESOLVE 30

HostResolverHelper::HostResolverHelper(net::HostResolver* hostresolver) :
        num_of_hosts_to_resolve(NUM_HOSTS_TO_RESOLVE), hostresolver_(
                hostresolver), hostname_provider_(NULL)
{
    char value[PROPERTY_VALUE_MAX] = { '\0' };
    property_get("net.dnshostprio.num_hosts", value, NULL);
    if (NULL != value && value[0] != '\0') {
        int host_num = atol(value);
        if (host_num <= 0) {
            host_num = NUM_HOSTS_TO_RESOLVE;
        }
        num_of_hosts_to_resolve = host_num;
    }
}

HostResolverHelper::~HostResolverHelper() {
}

void HostResolverHelper::Init(HostsProvider* provider) {
    hostname_provider_ = provider;
    LOG(INFO) << "DNSPreResolver::Init got hostprovider:" << provider;
    if (hostresolver_)
        hostresolver_->SetResolverExt(this);
}

void HostResolverHelper::DoResolve(HostResolverHelper* obj) {
    //TODO: Check there is another task like this in the queue and
    //      skip execution of the current  one
    obj->StartHostsResolution();
}

void HostResolverHelper::CancelAllRequests() {
    //loop through all requests and cancel those that still pending
    for (unsigned int i = 0; i < hostinfo_list_.size(); i++) {
        if (hostinfo_list_[i]->pending) {
            hostinfo_list_[i]->pending = false;
            hostresolver_->CancelRequest(hostinfo_list_[i]->reqhandle);
        }
    }
}

void HostResolverHelper::PrepareRequestsData(const std::vector<std::string>& hostnames) {
    const int actual_hosts_num = hostnames.size();
    hostinfo_list_.clear();
    hostinfo_list_.reserve(actual_hosts_num);
    for (int i = 0; i < actual_hosts_num; i++) {
        hostinfo_list_.push_back(new HostInfo(hostnames[i].c_str()));
    }
}

bool HostResolverHelper::StartHostsResolution() {
    if (NULL == hostresolver_) {
        return false;
    }
    if (NULL == hostname_provider_) {
        return false;
    }

    //get hosts from the provider
    std::vector < std::string > hostnames;
    hostname_provider_->GetHosts(num_of_hosts_to_resolve, hostnames);
    if (0 == hostnames.size()) {
        return true;
    }
    //free old requests if any still active
    CancelAllRequests();
    PrepareRequestsData(hostnames);

    const int actual_hosts_num = hostnames.size();
    //now issue resolve for each host
    for (int i = 0; i < actual_hosts_num; i++) {
        //TODO: set request priority
        hostinfo_list_[i]->pending = true;
        int rv = hostresolver_->Resolve(hostinfo_list_[i]->reqinfo, &hostinfo_list_[i]->addrlist,
                &hostinfo_list_[i]->completion_callback_, &hostinfo_list_[i]->reqhandle, net::BoundNetLog());
        //in case we got it resolved synchronously (or error) - set as not pending
        if (rv != net::ERR_IO_PENDING) {
            hostinfo_list_[i]->pending = false;
        }
    }
    return true;
}

void HostResolverHelper::HostInfo::OnLookupFinished(int result) {
    pending = false;
}

HostResolverHelper::HostInfo::HostInfo(const std::string& hostname) :
        reqinfo(net::HostPortPair(hostname, 80)), reqhandle(NULL), pending(false), completion_callback_(this,
                &HostInfo::OnLookupFinished) {
}


net::HostResolver::HostnameResolverExt* CreateResolverIPObserver(net::HostResolver* hostResolver) {
    char value[PROPERTY_VALUE_MAX] = { '\0' };
    const char* DNS_PRIORITY_EXTERNAL_LIB = "libdnshostprio.so";

    if (NULL == hostResolver) {
        return NULL;
    }
    property_get("net.dnshostprio.enable", value, NULL);
    if (NULL != value && value[0] == '0') {
        return NULL;
    }
    HostResolverHelper* dnsPreresolver = new HostResolverHelper(hostResolver);
    if (NULL == dnsPreresolver) {
        return NULL;
    }
    HostsProvider* provider = GET_DYNAMIC_OBJECT_INTERFACE_PTR(DNS_PRIORITY_EXTERNAL_LIB,HostsProvider);
    dnsPreresolver->Init(provider);
    return dnsPreresolver;
}
