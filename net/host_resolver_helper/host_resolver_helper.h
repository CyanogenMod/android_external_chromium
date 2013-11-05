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
#ifndef __DNSRESOLVERHELPER_H__
#define __DNSRESOLVERHELPER_H__

#include <vector>
#include <string>

#include "config.h"
#include "net/base/host_resolver.h"
#include "net/base/address_list.h"
#include "net/base/net_log.h"
#include "net/base/net_errors.h"
#include "base/message_loop.h"
#include "net/base/completion_callback.h"
#include "hosts_provider.h"

//This class does DNS pre-resolution and should be used by net::HostResolver
//It's lifetime is depending on the host resolver and they should be created and destroyed
// in the correct order
//TODO: make sure the resolver is never destroyed before the pre-resolver
class HostResolverHelper: public net::HostResolver::HostnameResolverExt {
public:
    HostResolverHelper(net::HostResolver* hostresolver);
    virtual ~HostResolverHelper();

    ///////////////////////////////////////////////////////////////////////////////////////
    // net::HostResolver::HostnamePreresolver interface
    virtual void Resolve() {
        MessageLoop::current()->PostDelayedTask(FROM_HERE, NewRunnableFunction(&HostResolverHelper::DoResolve, this),
                500);
    }
    ///////////////////////////////////////////////////////////////////////////////////////

    //task which is executed through the message loop
    static void DoResolve(HostResolverHelper* obj);

    //call it to connect with the hostnames provider
    void Init(HostsProvider* provider);

private:

    //to be called when hosts pre-resolution is requested (worker function)
    bool StartHostsResolution();

private:
    int num_of_hosts_to_resolve;
    net::HostResolver* hostresolver_;
    HostsProvider* hostname_provider_;
    // Delegate interface, for notification when the ResolveRequest completes.

    class HostInfo: public base::RefCounted<HostInfo> {
    public:
        net::AddressList addrlist;
        net::HostResolver::RequestInfo reqinfo;
        net::HostResolver::RequestHandle reqhandle;
        bool pending;
        net::CompletionCallbackImpl<HostInfo> completion_callback_;

        HostInfo(const std::string& hostname);
        ~HostInfo() {
        }

        void OnLookupFinished(int result);

    };

    std::vector<scoped_refptr<HostInfo> > hostinfo_list_;

    //used to cancel all pending requests before issuing new ones
    void CancelAllRequests();
    void PrepareRequestsData(const std::vector<std::string>& hostnames);

};

//intialization for the webkit
net::HostResolver::HostnameResolverExt* CreateResolverIPObserver(net::HostResolver* hostResolver);

#endif
