/** ---------------------------------------------------------------------------
 Copyright (c) 2011, 2012 The Linux Foundation. All rights reserved.

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
#include "dyn_lib_loader.h"
#include <cutils/log.h>

LibraryManager* LibraryManager::GetInstance() {
    static LibraryManager mgr;
    return &mgr;
}

LibraryManager::~LibraryManager() {
    for (LibDictionary::iterator it = libdict.begin(); it != libdict.end(); it++) {
        if (it->second) {
            ReleaseLibraryModule(it->second);
        }
    }
}

LibraryManager::MODULE_HANDLE_TYPE LibraryManager::GetLibraryHandleInternal(const std::string& libname) {
    LibHandle& handle = libdict[libname];
    if (NULL == handle) {
        //load module
        handle = LoadLibraryModule(libname);
        if (NULL == handle) {
            SLOGE("netstack: LIB_MGR - Error loading lib %s", libname.c_str());
            return handle;
        }
        SLOGI("netstack: LIB_MGR - Lib loaded: %s", libname.c_str());
    }
    handle.IncRefCount();
    return handle;
}

int LibraryManager::ReleaseLibraryHandleInternal(const std::string& libname) {
    int refcount = 0;
    LibHandle& handle = libdict[libname];
    if (NULL != handle) {
        refcount = handle.DecRefCount();
        if (0 == refcount) {
            //release module
            ReleaseLibraryModule(handle);
            SLOGI("netstack: LIB_MGR - Lib %s unloaded", libname.c_str());
            libdict.erase(libname);
        }
    }
    return refcount;
}

void* LibraryManager::GetSymbolInternal(const std::string& libname,    const std::string& symbol, bool optional) {
    return LoadLibrarySymbol(GetLibraryHandleInternal(libname), symbol, optional);
}

void* LibraryManager::LoadLibrarySymbol(const MODULE_HANDLE_TYPE& lh, const std::string& symname, bool optional) {
    void* symptr = NULL;
    if (lh) {
        const char *error;

        dlerror(); //see man dlopen
        symptr = dlsym(lh, symname.c_str());
        if (NULL != (error = dlerror())) {
            symptr = NULL;
            if (!optional) {
                SLOGE("netstack: LIB_MGR - Failed to load symbol %s", symname.c_str());
            }
        }
    }
    return symptr;
}

LibraryManager::MODULE_HANDLE_TYPE LibraryManager::LoadLibraryModule(const std::string& libname) {
    return dlopen(libname.c_str(), RTLD_LAZY);
}

void LibraryManager::ReleaseLibraryModule(const MODULE_HANDLE_TYPE& lh) {
    if (lh) {
        dlclose(lh);
    }
}

