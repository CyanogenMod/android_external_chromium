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

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <cutils/properties.h>
#include <cutils/log.h>

#include "app/sql/statement.h"
#include "app/sql/transaction.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/time.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/task.h"
#include "base/threading/thread.h"
#include "net/disk_cache/hash.h"

#include "stat_hub.h"

#define STAT_HUB_DYNAMIC_BIND_ON

namespace stat_hub {

#define FLUSH_DB_TIMEOUT_THRESHOLD_DEF  30000

typedef enum {
    INPUT_STATE_READ_MARKER,
    INPUT_STATE_READ_CMD,
    INPUT_STATE_READ_STRING_LEN,
    INPUT_STATE_READ_STRING_DATA,
    INPUT_STATE_READ_INT32,
} InputState;

const char* kPropNameEnabled = "net.sh.enabled";
const char* kPropNameDbpath = "net.sh.dbpath";
const char* kPropNameVerbose = "net.sh.verbose";
const char* kPropNameFlushDelay = "net.sh.flushdelay";
const char* kPropNameEnabledAppName = "net.sh.appname";
const char* kPropNamePlugin = "net.sh.plugin";
const char* kPropNameClearEnabled = "net.sh.clrenablde";
const char* kPropNamePerfEnabled = "net.sh.prfenabled";

const char* kEnabledAppName = "com.android.browser";
const char* kDefaultDbPath = "/data/data/com.android.browser/databases/db.sql";

void DoFlushDB(StatHub* database) {
    database->FlushDBrequest();
}

// Version number of the database.
static const int kCurrentVersionNumber = 1;
static const int kCompatibleVersionNumber = 1;

//=========================================================================
StatHub* StatHub::GetInstance() {
    static StatHub hub;
    if (!hub.IsReady() && 0==hub.under_construction_) {
        hub.under_construction_ = 1;
        hub.Init();
        hub.under_construction_ = 2;
    }
    return &hub;
}

//=========================================================================
StatHub::StatHub() :
    db_(NULL),
    ready_(false),
    flush_db_required_(false),
    flush_db_scheduled_(false),
    message_loop_(NULL),
    http_cache_(NULL),
    first_processor_(NULL),
    thread_(NULL),
    flush_delay_(FLUSH_DB_TIMEOUT_THRESHOLD_DEF),
    verbose_level_(STAT_HUB_VERBOSE_LEVEL_DISABLED),
    clear_enabled_(true),
    under_construction_(0),
    performance_enabled_(false)
{
    STAT_PLUGIN_IF_DEFINE(FetchGetRequestInfo)
    STAT_PLUGIN_IF_DEFINE(FetchStartComplete)
    STAT_PLUGIN_IF_DEFINE(FetchReadComplete)
    STAT_PLUGIN_IF_DEFINE(FetchDone)

    STAT_PLUGIN_IF_DEFINE(IsInDC)
    STAT_PLUGIN_IF_DEFINE(IsPreloaded)
    STAT_PLUGIN_IF_DEFINE(ReleasePreloaded)
    STAT_PLUGIN_IF_DEFINE(IsPreloaderEnabled)

    cmd_mask_ |= (1<<SH_CMD_WK_MEMORY_CACHE);
    cmd_mask_ |= (1<<SH_CMD_WK_MAIN_URL);
}

StatHub::~StatHub() {
    Release();
}

//=========================================================================
void* StatHub::LoadPlugin(const char* name) {
    if (IsVerboseEnabled()) {
        SLOGI("netstack: STAT_HUB - Loading plugin: %s", name);
    }
    StatProcessorGenericPlugin* plugin = new StatProcessorGenericPlugin(name);
    void* fh = plugin->OpenPlugin();
    if (NULL!=fh) {
        if (RegisterProcessor(plugin)) {
            SLOGI("netstack: STAT_HUB - Succeeded to load plugin: %s", name);
            return fh;
        }
    }
    delete plugin;
    SLOGE("netstack: STAT_HUB - Failed to load plugin: %s", name);
    return NULL;
}

//=========================================================================
bool StatHub::RegisterProcessor(StatProcessor* processor) {
    if (NULL!=processor) {
        std::string proc_name;
        std::string proc_version;

        if (!processor->OnGetProcInfo(proc_name, proc_version)) {
            SLOGE("netstack: STAT_HUB - Processor name is undefined");
            return false;
        }
        if (IsProcRegistered(proc_name.c_str())) {
            SLOGE("netstack: STAT_HUB - Processor %s already registered", proc_name.c_str());
            return false;
        }
        processor->next_ = first_processor_;
        first_processor_ = processor;
        return true;
    }
    return false;
}

//=========================================================================
StatProcessor* StatHub::DeleteProcessor(StatProcessor* processor) {
    if (NULL!=processor) {
        StatProcessor* next = processor->next_;
        if (first_processor_==processor) {
            first_processor_ = next;
        }
        else {
            for (StatProcessor* tmp_processor=first_processor_; tmp_processor!=NULL; tmp_processor=tmp_processor->next_ ) {
                if (tmp_processor->next_==processor) {
                    tmp_processor->next_=next;
                    break;
                }
            }
        }
        delete processor;
        return next;
    }
    return NULL;
}

//=========================================================================
bool StatHub::IsProcRegistered(const char* name) {
    std::string proc_name;
    std::string proc_version;

    for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_) {
        if (processor->OnGetProcInfo(proc_name, proc_version)) {
            if (proc_name==name) {
                return true;
            }
        }
    }
    return false;
}

//=========================================================================
bool StatHub::IsProcReady(const char* name) {
    if (IsReady()) {
        std::string proc_name;
        std::string proc_version;

        for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_) {
            if (processor->OnGetProcInfo(proc_name, proc_version)) {
                if (proc_name==name) {
                //size_t found = proc_name.find(name);
                //if (found != std::string::npos) {
                    if (STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
                        SLOGD("netstack: STAT_HUB - Processor %s is ready", name);
                    }
                    return true;
                }
            }
        }
    }
    if (IsVerboseEnabled()) {
        SLOGI("netstack: STAT_HUB - Processor %s is NOT ready", name);
    }
    return false;
}

//=========================================================================
bool StatHub::Init() {
    char value[PROPERTY_VALUE_MAX] = {'\0'};

    if (ready_) {
        SLOGE("netstack: STAT_HUB - Already initialized");
        return false;
    }

    //load mandatory plugins
    LoadPlugin("libdnshostprio.so");
    void* fh = LoadPlugin("spl_proc_plugin.so");
    if (fh) {
        STAT_PLUGIN_IMPORT(fh, FetchGetRequestInfo);
        STAT_PLUGIN_IMPORT(fh, FetchStartComplete);
        STAT_PLUGIN_IMPORT(fh, FetchReadComplete);
        STAT_PLUGIN_IMPORT(fh, FetchDone);

        STAT_PLUGIN_IMPORT(fh, IsInDC);
        STAT_PLUGIN_IMPORT(fh, IsPreloaded);
        STAT_PLUGIN_IMPORT(fh, ReleasePreloaded);
        STAT_PLUGIN_IMPORT(fh, IsPreloaderEnabled);
    }
    stat_hub::StatHub::GetInstance()->LoadPlugin("pp_proc_plugin.so");
    //        stat_hub::StatHub::GetInstance()->LoadPlugin("pageload_proc_plugin.so");

    property_get(kPropNameEnabled, value, "1"); //!!!!!!!!! ENABLED by default !!!!!!!!!
    if (!atoi(value)) {
        SLOGW("netstack: STAT_HUB - Disabled");
        return false;
    }

    property_get(kPropNameVerbose, value, "0"); //STAT_HUB_VERBOSE_LEVEL_DISABLED - 0
    verbose_level_ = (StatHubVerboseLevel)atoi(value);
    if (IsVerboseEnabled()) {
        SLOGI("netstack: STAT_HUB - Verbose Level: %d", verbose_level_);
    }

    property_get(kPropNameClearEnabled, value, "1");
    if (!atoi(value)) {
        clear_enabled_ = false;
        SLOGI("netstack: STAT_HUB - Cache Clear Disabled");
    }

    property_get(kPropNamePerfEnabled, value, "0");
    if (atoi(value)) {
        performance_enabled_ = true;
        SetPerfTimeStamp(StatHubGetSystemTime());
        SLOGI("netstack: STAT_HUB - Performance Piggyback Enabled");
    }

    //Check application restriction
    property_get(kPropNameEnabledAppName, value, kEnabledAppName);
    enabled_app_name_ = value;

    char path[128] = {'\0'};
    pid_t pid = getpid();
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
    int fd = open(path, O_RDONLY);
    int rd_len = read(fd, path , sizeof(path)-1);
    if (0 > rd_len) {
        rd_len = 0;
    }
    path[rd_len] = 0;
    close(fd);

    if(IsVerboseEnabled()) {
        SLOGI("netstack: STAT_HUB - Prc Name: %s (%d)", path ,(int)pid);
    }
    if (strcmp(path, enabled_app_name_.c_str())) {
        SLOGE("netstack:  STAT_HUB - App %s isn't supported", path);
        return false;
    }

    base::Time start(StatHubGetSystemTime());

    property_get(kPropNameFlushDelay, value, PROP_VAL_TO_STR(FLUSH_DB_TIMEOUT_THRESHOLD_DEF));
    flush_delay_ = atoi(value);
    if (flush_delay_<=0) {
        flush_delay_ = FLUSH_DB_TIMEOUT_THRESHOLD_DEF;
    }
    if(IsVerboseEnabled()) {
        SLOGI("netstack: STAT_HUB - Flush delay: %d", flush_delay_);
    }

    property_get(kPropNameDbpath, value, kDefaultDbPath);
    db_path_ = value;

    if(IsVerboseEnabled()) {
        SLOGI("netstack: STAT_HUB - DB path: %s", db_path_.c_str());
    }

    #if defined(NOT_NOW)
        db_->set_page_size(2048);
        db_->set_cache_size(32);
        //Run the database in exclusive mode. Nobody else should be accessing the
        //database while we're running, and this will give somewhat improved perf.
        db_->set_exclusive_locking();
    #endif //defined(NOT_NOW)
    db_ = new sql::Connection();
    if (!db_->Open(FilePath(db_path_.c_str()))) {
        SLOGE("netstack: STAT_HUB - Unable to open DB %s", db_path_.c_str());
        Release();
        return false;
    }

    // Scope initialization in a transaction so we can't be partially initialized.
    if (!StatHubBeginTransaction(db_)) {
        SLOGE("netstack: STAT_HUB - Unable to start transaction");
        Release();
        return false;
    }

    // Create tables.
    if (!InitTables()) {
        SLOGE("netstack: STAT_HUB - Unable to initialize DB tables");
        Release();
        return false;
    }

#ifdef STAT_HUB_DYNAMIC_BIND_ON
    //load arbitrary plugins
    for(int index=1; ; index++) {
        std::ostringstream index_str;
        index_str << "." << index;
        std::string plugin_prop_name = kPropNamePlugin + index_str.str();
        property_get(plugin_prop_name.c_str(), value, "");
        if (!value[0]) {
            break;
        }
        LoadPlugin(value);
    }
#endif // STAT_HUB_DYNAMIC_BIND_ON

    for (StatProcessor* processor=first_processor_; processor!=NULL;) {
        std::string proc_name = "Undefined";
        std::string proc_version ="0.0.0";
        processor->OnGetProcInfo(proc_name, proc_version);
        if(!processor->OnInit(db_)) {
            SLOGE("netstack: STAT_HUB - Processor %s (v%s) initialization failed",
                proc_name.c_str(), proc_version.c_str());
            processor = DeleteProcessor(processor);
        } else {
            SLOGI("netstack: STAT_HUB - Processor %s (v%s) is ready",
                proc_name.c_str(), proc_version.c_str());
            unsigned int cmd_mask;
            if (processor->OnGetCmdMask(cmd_mask)) {
                cmd_mask_ |= cmd_mask;
            }
            processor=processor->next_ ;
        }
    }

    // Initialization is complete.
    if (!StatHubCommitTransaction(db_)) {
        SLOGE("netstack: STAT_HUB - Unable to commit transaction");
        Release();
        return false;
    }

    for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_ ) {
        processor->OnFetchDb(db_);
    }

    thread_ = new base::Thread("event_handler");
    if (!thread_->StartWithOptions(base::Thread::Options(MessageLoop::TYPE_IO, 0))) {
        SLOGE("netstack: STAT_HUB - Event thread start error");
        Release();
        return false;
    }

    ready_ = true;
    if(IsVerboseEnabled()) {
        SLOGI("netstack: STAT_HUB - Init DB Time: %d" ,StatHubGetTimeDeltaInMs(start, StatHubGetSystemTime()));
    }

    SLOGI("netstack: STAT_HUB - Initialized");
    return true;
}

//=========================================================================
void StatHub::Release() {
    if(IsVerboseEnabled()) {
        SLOGI("netstack: STAT_HUB - Release");
    }

    //thread
    if(NULL!=thread_) {
        delete thread_;
        thread_ = NULL;
    }

    //processors
    StatProcessor* next_processor;
    for (StatProcessor* processor=first_processor_; processor!=NULL; processor=next_processor ) {
        next_processor = processor->next_;
        delete processor;
    }
    first_processor_ = NULL;

    //DataBase
    if (NULL!=db_) {
        db_->Close();
        delete db_;
        db_ = NULL;
    }

    //Rest
    flush_db_required_ = false;
    flush_db_scheduled_ = false;
    ready_ = false;
}

//=========================================================================
bool StatHub::InitTables() {
    if (!StatHubDoesTableExist(db_, "meta")) {
        if (!StatHubExecute(db_, "CREATE TABLE meta ("
            "key LONGVARCHAR NOT NULL UNIQUE PRIMARY KEY,"
            "value LONGVARCHAR"
            ")")) {
                return false;
        }
    }
    return true;
}

//=========================================================================
bool StatHub::GetDBmetaData(const char* key, std::string& val) {
    bool ret = false;

    sql::Statement* statement = StatHubGetStatement(db_, SQL_FROM_HERE,
        "SELECT * FROM meta WHERE key=?");
    StatHubStatementBindCString(statement, 0 , key);
    if(StatHubStatementStep(statement)) {
        ret = true;
        val = StatHubStatementColumnString(statement, 1);
    }
    StatHubReleaseStatement(statement);
    return ret;
}

//=========================================================================
bool StatHub::SetDBmetaData(const char* key, const char* val) {
    bool ret = true;

    sql::Statement* statement = StatHubGetStatement(db_, SQL_FROM_HERE,
        "INSERT OR REPLACE INTO meta "
        "(key, value) "
        "VALUES (?,?)");
    StatHubStatementBindCString(statement, 0 , key);
    StatHubStatementBindCString(statement, 1 , val);
    ret = StatHubStatementRun(statement);
    StatHubReleaseStatement(statement);
    return ret;
}

//=========================================================================
void StatHub::MainUrlLoaded(const char* url) {
    flush_db_request_time_ = StatHubGetSystemTime();
    flush_db_required_ = true;
    if (!flush_db_scheduled_) {
        flush_db_scheduled_ = true;
        if(IsVerboseEnabled()) {
            SLOGI("netstack: STAT_HUB - Request DB flush (%d)", flush_delay_ );
        }
        thread_->message_loop()->PostDelayedTask(FROM_HERE, NewRunnableFunction(&DoFlushDB, this), flush_delay_);
    }
}

//=========================================================================
void StatHub::Cmd(StatHubCmd* cmd) {
    if(NULL!=cmd) {
        if (IsPerfEnabled()) {
            StatHubTimeStamp time_stamp = StatHubGetSystemTime();
            if (StatHubGetTimeDeltaInMs(stat_hub::StatHub::GetInstance()->GetPerfTimeStamp(), time_stamp)>=50)
            {
                SetPerfTimeStamp(time_stamp);
                //pID
                char path[512] = {'\0'};
                pid_t pid = getpid();
                //stat
                snprintf(path, sizeof(path), "/proc/%d/stat", pid);
                int fd = open(path, O_RDONLY);
                if (-1!=fd) {
                    int rd_len = read(fd, path , sizeof(path)-1);
                    if (0 > rd_len) {
                        rd_len = 0;
                    }
                    path[rd_len] = 0;
                    cmd->SetStat(path);
                    close(fd);
                }
            }
        }
        StatHubCmdType cmd_id = cmd->GetCmd();
        StatHubActionType action_id = cmd->GetAction();
        if (STAT_HUB_IS_VERBOSE_LEVEL_DEBUG && STAT_HUB_DEV_LOG_ENABLED) {
            SLOGD("netstack: STAT_HUB - StatHub::Cmd CMD:%d Action:%d", cmd->GetCmd(), cmd->GetAction());
        }
        if (clear_enabled_ && SH_CMD_WK_MEMORY_CACHE==cmd_id && SH_ACTION_CLEAR==action_id) {
            for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_ ) {
                processor->OnClearDb(db_);
            }
        }
        for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_ ) {
            processor->OnCmd(cmd);
        }
        if (SH_CMD_WK_MAIN_URL==cmd_id && SH_ACTION_DID_FINISH==action_id) {
            const char* url = cmd->GetParamAsString(0);
            MainUrlLoaded(url);
        }
    }
}

//=========================================================================
void StatHub::FlushDBrequest() {
    if(IsVerboseEnabled()) {
        SLOGD("netstack: StatHub::FlushDBrequest : Start.");
    }

    int delta = StatHubGetTimeDeltaInMs(flush_db_request_time_, StatHubGetSystemTime());
    flush_db_scheduled_ = false;
    if (flush_db_required_) {
        if(IsVerboseEnabled()) {
            SLOGI("netstack: STAT_HUB -  Flush: %d", delta);
        }

        if (delta>=flush_delay_) {
           FlushDB();
        }
        else {
            if (!flush_db_scheduled_) {
               flush_db_scheduled_ = true;
               if(IsVerboseEnabled()) {
                   SLOGI("netstack: STAT_HUB - Restart in: %d", flush_delay_ - delta);
               }
               thread_->message_loop()->PostDelayedTask(FROM_HERE, NewRunnableFunction(&DoFlushDB, this), flush_delay_ - delta);
            }
        }
    }
}

//=========================================================================
bool StatHub::FlushDB() {
    if(IsVerboseEnabled()) {
        SLOGD("netstack: StatHub::FlushDB: Begin.");
    }
    base::Time start(StatHubGetSystemTime());

    for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_ ) {
        processor->OnFlushDb(db_);
    }

    if(IsVerboseEnabled()) {
        SLOGD("netstack: StatHub::FlushDB time : %d", StatHubGetTimeDeltaInMs(start, StatHubGetSystemTime()));
    }
    return true;
}

}  // namespace stat_hub

