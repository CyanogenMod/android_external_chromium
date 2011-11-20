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

#ifndef NET_STAT_HUB_H_
#define NET_STAT_HUB_H_
#pragma once

#include "googleurl/src/gurl.h"
#include "app/sql/connection.h"
#include "app/sql/init_status.h"
#include <dlfcn.h>

#include "stat_hub_api.h"

class MessageLoop;

namespace base {
    class Time;
    class Thread;
}

namespace stat_hub {

extern const char* kEnabledAppName;

typedef void(* event_cb)(int fd, short event, void* arg);

class StatProcessor {
public:
    StatProcessor(): next_(NULL){
    }

virtual ~StatProcessor() {
    }

    //Events
virtual bool OnInit(sql::Connection* db, MessageLoop* message_loop)=0;
virtual bool OnFetchDb(sql::Connection* db)=0;
virtual bool OnFlushDb(sql::Connection* db)=0;
virtual bool OnClearDb(sql::Connection* db)=0;
virtual bool OnCmd(StatHubTimeStamp timestamp, unsigned short cmd, void* param1, int sizeofparam1, void* param2, int sizeofparam2) {return false;}
virtual bool OnGetProcName(std::string& name)=0;
virtual bool OnGetCmdMask(unsigned int& cmd_mask)=0;

private:
    friend class StatHub;

    StatProcessor* next_;
};

#define STAT_PLUGIN_METHOD_0(name, ret) \
    ret (*Do##name)(); \
    virtual ret name() { if(NULL!=Do##name) return Do##name(); return (ret)0;}

#define STAT_PLUGIN_METHOD_1(name, ret, type1, param1) \
    ret (*Do##name)(type1 param1); \
    virtual ret name(type1 param1) { if(NULL!=Do##name) return Do##name(param1); return (ret)0;}

#define STAT_PLUGIN_METHOD_2(name, ret, type1, param1, type2, param2) \
    ret (*Do##name)(type1 param1, type2 param2); \
    virtual ret name(type1 param1,type2 param2) { if(NULL!=Do##name) return Do##name(param1, param2); return (ret)0;}

#define STAT_PLUGIN_METHOD_3(name, ret, type1, param1, type2, param2, type3, param3) \
    ret (*Do##name)(type1 param1, type2 param2, type3 param3); \
    virtual ret name(type1 param1,type2 param2,type3 param3) \
        { if(NULL!=Do##name) return Do##name(param1, param2, param3); return (ret)0;}

#define STAT_PLUGIN_METHOD_4(name, ret, type1, param1, type2, param2, type3, param3, type4, param4) \
    ret (*Do##name)(type1 param1, type2 param2, type3 param3, type4 param4); \
    virtual ret name(type1 param1,type2 param2,type3 param3, type4 param4) \
        { if(NULL!=Do##name) return Do##name(param1, param2, param3, param4); return (ret)0;}

#define STAT_PLUGIN_METHOD_5(name, ret, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5) \
    ret (*Do##name)(type1 param1, type2 param2, type3 param3, type4 param4, type5 param5); \
    virtual ret name(type1 param1,type2 param2,type3 param3, type4 param4, type5 param5) \
        { if(NULL!=Do##name) return Do##name(param1, param2, param3, param4, param5); return (ret)0;}

#define STAT_PLUGIN_METHOD_6(name, ret, type1, param1, type2, param2, type3, param3, type4, param4, type5, param5, type6, param6) \
    ret (*Do##name)(type1 param1, type2 param2, type3 param3, type4 param4, type5 param5, type6 param6); \
    virtual ret name(type1 param1,type2 param2,type3 param3, type4 param4, type5 param5, type6 param6) \
        { if(NULL!=Do##name) return Do##name(param1, param2, param3, param4, param5, param6); return (ret)0;}

#define STAT_PLUGIN_IF_DEFINE(name) \
    Do##name = NULL;

#define STAT_PLUGIN_IMPORT(handle, name) \
    *(void **)(&Do##name) = dlsym(handle, #name);

class StatProcessorGenericPlugin : public StatProcessor {
public:
    StatProcessorGenericPlugin(const char* name) :
        initialized_(false) {
        if (NULL!=name) {
            name_ = name;
        }
        STAT_PLUGIN_IF_DEFINE(OnInit)
        STAT_PLUGIN_IF_DEFINE(OnFetchDb)
        STAT_PLUGIN_IF_DEFINE(OnFlushDb)
        STAT_PLUGIN_IF_DEFINE(OnClearDb)
        STAT_PLUGIN_IF_DEFINE(OnCmd)
        STAT_PLUGIN_IF_DEFINE(OnGetProcName)
        STAT_PLUGIN_IF_DEFINE(OnGetCmdMask)
    }

virtual ~StatProcessorGenericPlugin() {
    }

    STAT_PLUGIN_METHOD_2(OnInit, bool, sql::Connection*, db, MessageLoop*, message_loop)
    STAT_PLUGIN_METHOD_1(OnFetchDb, bool, sql::Connection*, db)
    STAT_PLUGIN_METHOD_1(OnFlushDb, bool, sql::Connection*, db)
    STAT_PLUGIN_METHOD_1(OnClearDb, bool, sql::Connection*, db)
    STAT_PLUGIN_METHOD_6(OnCmd, bool, StatHubTimeStamp, timestamp, unsigned short, cmd, void*, param1, int, sizeofparam1, void*, param2, int, sizeofparam2)
    STAT_PLUGIN_METHOD_1(OnGetProcName, bool, std::string&, name)
    STAT_PLUGIN_METHOD_1(OnGetCmdMask, bool, unsigned int&, cmd_mask)

    bool OpenPlugin(void* fh=NULL) {
        if (!initialized_) {
            if (NULL==fh && !name_.empty()) {
                fh = dlopen(name_.c_str(), RTLD_NOW);
            }
            if (fh) {
                initialized_ = true;
                STAT_PLUGIN_IMPORT(fh, OnInit)
                STAT_PLUGIN_IMPORT(fh, OnFetchDb)
                STAT_PLUGIN_IMPORT(fh, OnFlushDb)
                STAT_PLUGIN_IMPORT(fh, OnClearDb)
                STAT_PLUGIN_IMPORT(fh, OnCmd)
                STAT_PLUGIN_IMPORT(fh, OnGetProcName)
                STAT_PLUGIN_IMPORT(fh, OnGetCmdMask)
            }
            else {
                if(!name_.empty()) {
                LOG(INFO) << "Failed to open plugin:" << name_.c_str();
                }
            }
        }
        return initialized_;
    }

private:
    bool initialized_;
    std::string name_;
};

class StatHub {
public:

virtual ~StatHub();

static StatHub* GetInstance();

    void RegisterProcessor(StatProcessor* processor);
    StatProcessor* DeleteProcessor(StatProcessor* processor);

    bool Init(const std::string& db_path, MessageLoop* message_loop, net::HttpCache* http_cache);
    void Release();
    bool LoadPlugin(const char* name);

    void UpdateMainUrl(const char* url);
    void UpdateSubUrl(const char* main_url,const char* sub_url);
    void MainUrlLoaded(const char* url);
    void Cmd(StatHubTimeStamp timestamp, unsigned short cmd, void* param1, int sizeofparam1, void* param2, int sizeofparam2);

    void FlushDBrequest();
    bool FlushDB();

    bool GetDBmetaData(const char* key, std::string& val);
    bool SetDBmetaData(const char* key, const char* val);

    bool IsReady() {
        return ready_;
    }
    bool IsProcReady(const char *name);
    MessageLoop* GetMessageLoop() {
        return message_loop_;
    }

    net::HttpCache* GetHttpCache() {
        return http_cache_;
    }

    sql::Connection* GetDb() {
        return db_;
    }

    base::Thread* GetThread() {
        return thread_;
    }

    bool IsVerboseEnabled() {
        return (verbose_level_!=STAT_HUB_VERBOSE_LEVEL_DISABLED);
    }

    StatHubVerboseLevel GetVerboseLevel() {
        return verbose_level_;
    }

    unsigned int GetCmdMask() {
        return cmd_mask_;
    }

 private:

    StatHub();

    #if defined(NOT_NOW)
        // Vacuums the database. This will cause sqlite to defragment and collect
        // unused space in the file. It can be VERY SLOW.
        void Vacuum();
    #endif //defined(NOT_NOW)

    // Creates tables, returning true if the table already exists
    // or was successfully created.
    bool InitTables();

    sql::Connection* db_;

    std::string db_path_;
    bool ready_;
    bool flush_db_required_;
    bool flush_db_scheduled_;
    base::Time flush_db_request_time_;

    MessageLoop* message_loop_;
    net::HttpCache* http_cache_;

    std::string enabled_app_name_;

    StatProcessor*  first_processor_;

    // Separate thread on which we run blocking read for notify events.
    base::Thread* thread_;
    int flush_delay_;
    StatHubVerboseLevel verbose_level_;

    DISALLOW_COPY_AND_ASSIGN(StatHub);
    unsigned int    cmd_mask_;
};

}  // namespace stat_hub

#endif  // NET_STAT_HUB_H_
