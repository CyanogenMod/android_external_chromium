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
#ifndef __DYN_LIB_LOADER_H__
#define __DYN_LIB_LOADER_H__

#include <map>
#include <string>
#include <dlfcn.h>
#include "dyn_lib_loader_decl.h"

template<class T>
inline T* GetInterfacePtr(void* funptr) {
    typedef T* (*InterfaceFunPtr)(void);
    if (NULL==funptr) {
        return NULL;
    }
    InterfaceFunPtr fun = (InterfaceFunPtr) (funptr);
    return fun();
}

//This macro is used by module which wishes to use the interface object exported by some other module
// libname - is the dynamic library full name, interface_class - is the interface class for which
// the object will be imported
// EXAMPLE: Interface* ptr = GET_DYNAMIC_OBJECT_INTERFACE_PTR("libmy.so",Interface);
#define GET_DYNAMIC_OBJECT_INTERFACE_PTR(__libname__,__interface_class__) \
    GetInterfacePtr<__interface_class__>(LibraryManager::GetFunctionPtr<__interface_class__>(__libname__,"Get" #__interface_class__ "Object"))

//LibraryManager  Connects to shared library reusing the lib handle
//if called from mutiple places. Get retrieve exported functions and library handle.
//This class is module level singletone.
class LibraryManager {
public:

    typedef void* MODULE_HANDLE_TYPE;
    template<class RET_TYPE>
    static RET_TYPE* GetFunctionPtr(const char* libname, const char* symbol, bool optional=false) {
        return static_cast<RET_TYPE*>(GetInstance()->GetSymbolInternal(libname, symbol, optional));
    }

    static void* GetLibrarySymbol(const MODULE_HANDLE_TYPE& lh, const std::string& symname, bool optional=false) {
        return GetInstance()->LoadLibrarySymbol(lh, symname, optional);
    }

    static MODULE_HANDLE_TYPE GetLibraryHandle(const char* libname) {
        return GetInstance()->GetLibraryHandleInternal(libname);
    }

    static int ReleaseLibraryHandle(const char* libname) {
        return GetInstance()->ReleaseLibraryHandleInternal(libname);
    }

private:
    template<class MODULE_HANDLE_TYPE>
    class LibHandleType {
    public:
        LibHandleType() :
                handle(NULL),
                refcount(0) {
        }

        LibHandleType(MODULE_HANDLE_TYPE h) :
                handle(h) {
        }

        operator MODULE_HANDLE_TYPE() {
            return handle;
        }

        int IncRefCount () {
            refcount++;
            return refcount;
        }

        int DecRefCount () {
            if (!refcount) {
                refcount--;
            }
            return refcount;
        }

    private:
        MODULE_HANDLE_TYPE handle;
        int refcount;
    };

    typedef LibHandleType<MODULE_HANDLE_TYPE> LibHandle;
    typedef std::map<std::string, LibHandle> LibDictionary;

    static LibraryManager* GetInstance();
    LibraryManager() {}
    ~LibraryManager();
    void* GetSymbolInternal(const std::string& libname,    const std::string& symbol, bool optional);
    MODULE_HANDLE_TYPE GetLibraryHandleInternal(const std::string& libname);
    int ReleaseLibraryHandleInternal(const std::string& libname);
    void * LoadLibrarySymbol(const MODULE_HANDLE_TYPE& lh, const std::string& symname, bool optional);
    MODULE_HANDLE_TYPE LoadLibraryModule(const std::string& libname);
    void ReleaseLibraryModule(const MODULE_HANDLE_TYPE& lh);

    std::map<std::string, LibHandle> libdict;
};

#endif //__DYN_LIB_LOADER_H__
