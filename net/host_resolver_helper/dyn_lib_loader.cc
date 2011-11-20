/** ---------------------------------------------------------------------------
 Copyright (c) 2011, Code Aurora Forum. All rights reserved.

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
#include "dyn_lib_loader.h"

LibraryManager* LibraryManager::GetInstance() {
    static LibraryManager mgr;
    return &mgr;
}

LibraryManager::~LibraryManager() {
    for (LibDictionary::iterator it = libdict.begin(); it != libdict.end();
            it++) {
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
    }
    return handle;
}

void* LibraryManager::GetSymbolInternal(const std::string& libname,    const std::string& symbol) {
    return LoadLibrarySymbol(GetLibraryHandleInternal(libname), symbol);
}

void * LibraryManager::LoadLibrarySymbol(const MODULE_HANDLE_TYPE& lh,
        const std::string& symname) {
    void* symptr = NULL;
    if (lh) {
        symptr = dlsym(lh, symname.c_str());
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

