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

// API for the network plug-in
#ifndef STAT_HUB_API_H_
#define STAT_HUB_API_H_

#include <string>
#include <algorithm>
#include <vector>
#include "base/time.h"
#include "app/sql/connection.h"
#include "app/sql/statement.h"
#include "net/http/http_cache.h"
#include "stat_hub_cmd_api.h"

#define PROP_VAL_TO_STR(val) PROP_VAL_TO_STR_HELPER(val)
#define PROP_VAL_TO_STR_HELPER(val) #val
#define STAT_MAX_PARAM_LEN  2048
#define STAT_HUB_MAX_NUM_OF_ACTIONS     10

#define STAT_HUB_DEV_LOG_ENABLED    (false)

#define STAT_HUB_IS_VERBOSE_LEVEL_ERROR     (StatHubGetVerboseLevel()>=STAT_HUB_VERBOSE_LEVEL_ERROR)
#define STAT_HUB_IS_VERBOSE_LEVEL_WARNING   (StatHubGetVerboseLevel()>=STAT_HUB_VERBOSE_LEVEL_WARNING)
#define STAT_HUB_IS_VERBOSE_LEVEL_INFO      (StatHubGetVerboseLevel()>=STAT_HUB_VERBOSE_LEVEL_INFO)
#define STAT_HUB_IS_VERBOSE_LEVEL_DEBUG     (StatHubGetVerboseLevel()>=STAT_HUB_VERBOSE_LEVEL_DEBUG)

#define STAT_HUB_CMD_HANDLER_INIT(handlers_list, cmd_mask, cmd_id, handler, ...) \
    { \
        StatHubActionType act[] = {__VA_ARGS__, SH_ACTION_LAST}; \
        handlers_list.push_back(StatHubCmdHandlerType(cmd_id, handler, act)); \
        cmd_mask |= (1<<cmd_id); \
    }

typedef enum StatHubVerboseLevel {
    STAT_HUB_VERBOSE_LEVEL_DISABLED,// 0
    STAT_HUB_VERBOSE_LEVEL_ERROR,   // 1
    STAT_HUB_VERBOSE_LEVEL_WARNING, // 2
    STAT_HUB_VERBOSE_LEVEL_INFO,    // 3
    STAT_HUB_VERBOSE_LEVEL_DEBUG    // 4
} StatHubVerboseLevel;

typedef enum StatHubMimeType{
    StatHubMimeImageResource,      // 0
    StatHubMimeCSSStyleSheet,      // 1
    StatHubMimeScript,             // 2
    StatHubMimeFontResource,       // 3
    StatHubMimeXSLStyleSheet,      // 4
    StatHubMimeLinkResource,       // 5
    StatHubMimeUnDefined           // 6
} StatHubMimeType;


typedef base::Time StatHubTimeStamp;

template <typename T> void DeleteStatHubParam(T *p) {
    delete p;
}

class MessageLoop;

namespace net {
    class URLRequestContext;
}

class StatHubCmd {
public:

inline StatHubCmdType GetCmd() {
        return cmd_;
    }

inline StatHubActionType GetAction() {
        return action_;
    }

inline unsigned int GetCookie() {
        return cookie_;
    }

inline StatHubTimeStamp& GetStartTimeStamp() {
        return start_timestamp_;
    }

inline StatHubTimeStamp& GetCommitTimeStamp() {
        return commit_timestamp_;
    }

inline void AddParamAsUint32(unsigned int param) {
        params_.push_back(new StatHubCmdParam(param));
    }

inline void AddParamAsString(const char* param) {
        params_.push_back(new StatHubCmdParam(param));
    }

inline void AddParamAsBuf(const void* param, unsigned int size) {
        params_.push_back(new StatHubCmdParam(param, size));
    }

inline void AddParamAsBool(bool param) {
        params_.push_back(new StatHubCmdParam(param));
    }

inline void SetStartTimeStamp(StatHubTimeStamp timestamp) {
        start_timestamp_ = timestamp;
    }

inline void SetCommitTimeStamp(StatHubTimeStamp timestamp) {
        commit_timestamp_ = timestamp;
    }

inline void SetStat(char* stat) {
        stat_ = stat;
    }

inline std::string& GetStat() {
        return stat_;
    }

    unsigned int GetParamAsUint32(unsigned int param_index) {
        if (param_index<params_.size()) {
            return (unsigned int)params_[param_index]->param_;
        }
        return 0;
    }

    const char* GetParamAsString(unsigned int param_index) {
        if (param_index<params_.size()) {
            return (const char*)params_[param_index]->param_;
        }
        return NULL;
    }

    void* GetParamAsBuf(unsigned int param_index, unsigned int& size) {
        if (param_index<params_.size()) {
            size = params_[param_index]->param_size_;
            return params_[param_index]->param_;
        }
        size =0;
        return NULL;
    }

    bool GetParamAsBool(unsigned int param_index) {
        if (param_index<params_.size()) {
            return (bool)params_[param_index]->param_;
        }
        return false;
    }

static StatHubCmd* Create(StatHubCmdType cmd, StatHubActionType action = SH_ACTION_NONE, unsigned int cookie = 0);
static void Release(StatHubCmd* cmd);

    void IncReference() {
        referenced_++;
    }

private:
    StatHubCmd(StatHubCmdType cmd, StatHubActionType action, unsigned int cookie):
       cmd_(cmd), action_(action), cookie_(cookie), referenced_(1) {
    }

    StatHubCmd() {};

    virtual ~StatHubCmd() {
        std::for_each(params_.begin(), params_.end(), DeleteStatHubParam<StatHubCmdParam>);
    }

    class StatHubCmdParam
    {
    public:
        StatHubCmdParam(unsigned int param): param_size_(0) {
            param_ = (void*)param;
        }

        StatHubCmdParam(const char* param);
        StatHubCmdParam(const void* param, unsigned int param_size);

        StatHubCmdParam(bool param): param_size_(0) {
            param_ = (void*)param;
        }

        ~StatHubCmdParam() {
            if (0!=param_size_ && NULL!=param_) {
                delete (char*)param_;
            }
        }

    private:
        friend class StatHubCmd;
        StatHubCmdParam() {}

        void*           param_;
        unsigned int    param_size_;
    };

    typedef std::vector<StatHubCmdParam*> StatHubCmdParamsType;

    StatHubCmdType          cmd_;
    StatHubActionType       action_;
    unsigned int            cookie_;
    StatHubTimeStamp        start_timestamp_;
    StatHubTimeStamp        commit_timestamp_;
    StatHubCmdParamsType    params_;

    unsigned int            referenced_;

    //performance
    std::string             stat_;
};

typedef bool (*StatHubCmdHandler)(StatHubCmd* cmd);

class StatHubCmdHandlerType {
public:
    StatHubCmdHandlerType(StatHubCmdType cmd, StatHubCmdHandler handler, StatHubActionType act[]):
        cmd_(cmd), handler_(handler) {
        for (act_size_=0; act[act_size_]!=SH_ACTION_LAST; act_size_++);
        if (act_size_>0) {
            for (unsigned int index = 0; index<act_size_ && index<STAT_HUB_MAX_NUM_OF_ACTIONS; index++) {
                act_[index] = act[index];
            }
        }
    }

    ~StatHubCmdHandlerType() {}

inline StatHubCmdHandler CheckHandler(StatHubCmdType cmd, StatHubActionType action) {
        if (cmd_==cmd) {
            for (unsigned int index = 0; index<act_size_; index++) {
                if (act_[index]==action) {
                    return handler_;
                }
            }
        }
        return NULL;
    }

    StatHubCmdType      cmd_;
    StatHubCmdHandler   handler_;
    StatHubActionType   act_[STAT_HUB_MAX_NUM_OF_ACTIONS];
    unsigned int        act_size_;
};

typedef std::vector<StatHubCmdHandlerType> StatHubCmdHandlersList;

// ================================ StatHub Interface ====================================
extern bool StatHubIsVerboseEnabled()
    __attribute__ ((visibility ("default"), used));
extern bool StatHubIsPerfEnabled()
    __attribute__ ((visibility ("default"), used));
extern StatHubVerboseLevel StatHubGetVerboseLevel()
    __attribute__ ((visibility ("default"), used));
extern base::Time StatHubGetSystemTime()
    __attribute__ ((visibility ("default"), used));
extern int StatHubGetTimeDeltaInMs(const base::Time& start_time, const base::Time& finish_time)
    __attribute__ ((visibility ("default"), used));
extern const char* StatHubGetHostFromUrl(const std::string& url, std::string& host)
    __attribute__ ((visibility ("default"), used));
const char* StatHubGetHostPortFromUrl(const std::string& url, std::string& host_port)
    __attribute__ ((visibility ("default"), used));
int StatHubGetMaxSocketsPerGroup()
    __attribute__ ((visibility ("default"), used));
bool StatHubFetch(void* cookie, net::URLRequestContext* context)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubGetDBmetaData(const char* key, std::string& val)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubSetDBmetaData(const char* key, const char* val)
    __attribute__ ((visibility ("default"), used));
extern void StatHubSetHttpCache(net::HttpCache* cache)
    __attribute__ ((visibility ("default"), used));
extern net::HttpCache* StatHubGetHttpCache()
    __attribute__ ((visibility ("default"), used));
extern void StatHubSetIoMessageLoop(MessageLoop* message_loop)
    __attribute__ ((visibility ("default"), used));
extern MessageLoop* StatHubGetIoMessageLoop()
    __attribute__ ((visibility ("default"), used));

// ================================ StatHub SQL Interface ====================================
extern bool StatHubBeginTransaction(sql::Connection* db)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubCommitTransaction(sql::Connection* db)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubDoesTableExist(sql::Connection* db, const char* table_name)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubExecute(sql::Connection* db, const char* sql)
    __attribute__ ((visibility ("default"), used));
extern sql::Statement* StatHubGetStatement(sql::Connection* db, const sql::StatementID& id, const char* sql)
    __attribute__ ((visibility ("default"), used));
extern void StatHubReleaseStatement(sql::Statement* st)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubStatementStep(sql::Statement* st)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubStatementRun(sql::Statement* st)
    __attribute__ ((visibility ("default"), used));
extern void StatHubStatementReset(sql::Statement* st)
    __attribute__ ((visibility ("default"), used));
extern int StatHubStatementColumnInt(sql::Statement* st, int col)
    __attribute__ ((visibility ("default"), used));
extern int64 StatHubStatementColumnInt64(sql::Statement* st, int col)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubStatementColumnBool(sql::Statement* st, int col)
    __attribute__ ((visibility ("default"), used));
extern std::string StatHubStatementColumnString(sql::Statement* st, int col)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubStatementBindInt(sql::Statement* st, int col, int val)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubStatementBindInt64(sql::Statement* st, int col, int64 val)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubStatementBindBool(sql::Statement* st, int col, bool val)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubStatementBindCString(sql::Statement* st, int col, const char* val)
    __attribute__ ((visibility ("default"), used));

#endif /* STAT_HUB_API_H_ */
