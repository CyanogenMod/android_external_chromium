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

// API for the network plug-in
#ifndef STAT_HUB_API_H_
#define STAT_HUB_API_H_

#include <string>
#include "base/time.h"
#include "app/sql/connection.h"
#include "app/sql/statement.h"
#include "net/http/http_cache.h"
#include "stat_hub_cmd_api.h"

#define PROP_VAL_TO_STR(val) PROP_VAL_TO_STR_HELPER(val)
#define PROP_VAL_TO_STR_HELPER(val) #val
#define STAT_MAX_PARAM_LEN  2048

#define STAT_HUB_IS_VERBOSE_LEVEL_ERROR     (StatHubGetVerboseLevel()>=STAT_HUB_VERBOSE_LEVEL_ERROR)
#define STAT_HUB_IS_VERBOSE_LEVEL_WARNING   (StatHubGetVerboseLevel()>=STAT_HUB_VERBOSE_LEVEL_WARNING)
#define STAT_HUB_IS_VERBOSE_LEVEL_INFO      (StatHubGetVerboseLevel()>=STAT_HUB_VERBOSE_LEVEL_INFO)
#define STAT_HUB_IS_VERBOSE_LEVEL_DEBUG     (StatHubGetVerboseLevel()>=STAT_HUB_VERBOSE_LEVEL_DEBUG)

typedef enum StatHubVerboseLevel {
    STAT_HUB_VERBOSE_LEVEL_DISABLED,// 0
    STAT_HUB_VERBOSE_LEVEL_ERROR,   // 1
    STAT_HUB_VERBOSE_LEVEL_WARNING, // 2
    STAT_HUB_VERBOSE_LEVEL_INFO,    // 3
    STAT_HUB_VERBOSE_LEVEL_DEBUG    // 4
} StatHubVerboseLevel;

typedef base::Time StatHubTimeStamp;
class MessageLoop;

extern bool StatHubIsVerboseEnabled()
    __attribute__ ((visibility ("default"), used));
extern StatHubVerboseLevel StatHubGetVerboseLevel()
    __attribute__ ((visibility ("default"), used));
extern base::Time StatHubGetSystemTime()
    __attribute__ ((visibility ("default"), used));
extern int StatHubGetTimeDeltaInMs(const base::Time& start_time, const base::Time& finish_time)
    __attribute__ ((visibility ("default"), used));
extern const char* StatHubGetHostFromUrl(std::string& url, std::string& host)
    __attribute__ ((visibility ("default"), used));
extern void StatHubPreconnect(MessageLoop* message_loop, net::HttpCache* cache, const char* url, uint32 count)
    __attribute__ ((visibility ("default"), used));
extern void StatHubFetch(MessageLoop* message_loop, net::HttpCache* cache, const char* url, const char* headers)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubGetDBmetaData(const char* key, std::string& val)
    __attribute__ ((visibility ("default"), used));
extern bool StatHubSetDBmetaData(const char* key, const char* val)
    __attribute__ ((visibility ("default"), used));
extern net::HttpCache* StatHubGetHttpCache()
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
