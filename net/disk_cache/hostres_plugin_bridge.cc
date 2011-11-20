/** ---------------------------------------------------------------------------
 Copyright (c) 2011, 2012 Code Aurora Forum. All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:
     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
       disclaimer in the documentation and/or other materials provided
       with the distribution.
     * Neither the name of Code Aurora Forum, Inc. nor the names of its
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
#include "hostres_plugin_bridge.h"
#include "net/disk_cache/stat_hub.h"
#include "net/host_resolver_helper/dyn_lib_loader.h"
#include "base/logging.h"

class HostResProcessor : public stat_hub::StatProcessorGenericPlugin {
public:
    HostResProcessor(const char* name):StatProcessorGenericPlugin(name) {
    }

    virtual ~HostResProcessor() {
    }

private:
    DISALLOW_COPY_AND_ASSIGN(HostResProcessor);
};

const char* hostres_plugin_name = "libdnshostprio.so";

stat_hub::StatProcessor* StatHubCreateHostResPlugin()
{
    static bool initialized = false;
    if (!initialized) {
        LOG(INFO) << "StatHubCreateHostResPlugin initializing...";
        initialized = true;
        HostResProcessor* hp = new HostResProcessor(hostres_plugin_name);
        void* fh = LibraryManager::GetLibraryHandle(hostres_plugin_name);
        if (fh) {
            LOG(INFO) << "StatHubCreateHostResPlugin lib loaded";
            const char* fn = NULL;
            bool dll_ok = false;

            dll_ok = hp->OpenPlugin(fh);
            if (dll_ok) {
                LOG(INFO) << "StatHubCreateHostResPlugin plugin connected";;
                return hp;
            }
        }
        else {
            LOG(INFO) << "netstack: Failed to open plugin:" << hostres_plugin_name;
        }
        delete hp;
    }

    LOG(INFO) << "netstack: Failed to find symbols in plugin: " << hostres_plugin_name;
    return NULL;
}

