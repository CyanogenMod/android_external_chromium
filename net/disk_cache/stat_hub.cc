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

namespace stat_hub {

#define FLUSH_DB_TIMEOUT_THRESHOLD_DEF  30000

typedef enum {
    INPUT_STATE_READ_MARKER,
    INPUT_STATE_READ_CMD,
    INPUT_STATE_READ_STRING_LEN,
    INPUT_STATE_READ_STRING_DATA,
    INPUT_STATE_READ_INT32,
} InputState;

const char* kPropNameEnabled = "stathub.enabled";
const char* kPropNameDbpath = "stathub.dbpath";
const char* kPropNameVerbose = "stathub.verbose";
const char* kPropNameFlushDelay = "stathub.flushdelay";
const char* kPropNameEnabledAppName = "stathub.appname";
const char* kPropNamePlugin = "stathub.plugin";

const char* kEnabledAppName = "com.android.browser";

void DoFlushDB(StatHub* database) {
    database->FlushDBrequest();
}

// Version number of the database.
static const int kCurrentVersionNumber = 1;
static const int kCompatibleVersionNumber = 1;

StatHub* StatHub::GetInstance() {
    static StatHub hub;
    return &hub;
}

StatHub::StatHub() :
    db_(NULL),
    ready_(false),
    flush_db_required_(false),
    flush_db_scheduled_(false),
    first_processor_(NULL),
    thread_(NULL),
    flush_delay_(FLUSH_DB_TIMEOUT_THRESHOLD_DEF),
    verbose_level_(STAT_HUB_VERBOSE_LEVEL_DISABLED) {

    cmd_mask_ |= (1<<INPUT_CMD_WK_MMC_CLEAR);
    cmd_mask_ |= (1<<INPUT_CMD_WK_MAIN_URL_LOADED);
}

StatHub::~StatHub() {
    Release();
}

bool StatHub::LoadPlugin(const char* name) {
    if (IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::Init - Load plugin: " << name;
    }
    StatProcessorGenericPlugin* plugin = new StatProcessorGenericPlugin(name);
    if( plugin->OpenPlugin()) {
        RegisterProcessor(plugin);
        LOG(INFO) << "netstack: succeeded to load StatHub plugin: " << name;
        return true;
    }

    LOG(INFO) << "netstack: failed to load StatHub plugin: " << name;
    return false;
}

void StatHub::RegisterProcessor(StatProcessor* processor) {
    if (NULL!=processor) {
        processor->next_ = first_processor_;
        first_processor_ = processor;
    }
}

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

bool StatHub::IsProcReady(const char* name) {
    if (IsReady()) {
        std::string proc_name;

        for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_) {
            if (processor->OnGetProcName(proc_name)) {
                //if (proc_name==name) {
                size_t found = proc_name.find(name);
                if (found != std::string::npos) {
                    if (IsVerboseEnabled()) {
                        LOG(INFO) << "StatHub::IsProcReady:(true) for:" << name;
                    }
                    return true;
                }
            }
        }
    }
    if (IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::(false) for:" << name;
    }
    return false;
}

bool StatHub::Init(const std::string& db_path, MessageLoop* message_loop, net::HttpCache* http_cache) {
    char value[PROPERTY_VALUE_MAX] = {'\0'};

    if (ready_) {
        LOG(INFO) << "StatHub::Init - Already initializes";
        return false;
    }

    if (db_path.empty() || NULL==message_loop) {
        LOG(ERROR) << "StatHub::Init - Bad parameters";
        return false;
    }

    property_get(kPropNameEnabled, value, "1"); //!!!!!!!!! ENABLED by default !!!!!!!!!
    if (!atoi(value)) {
        LOG(INFO) << "StatHub::Init - Disabled";
        return false;
    }

    property_get(kPropNameVerbose, value, "0"); //STAT_HUB_VERBOSE_LEVEL_DISABLED - 0
    verbose_level_ = (StatHubVerboseLevel)atoi(value);
    if (IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::Init - Verbose Level: " << verbose_level_;
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
        LOG(INFO) << "CacheDatabase::Init - Prc Name: " << path << "(" << (int)pid << ")";
    }
    if (strcmp(path, enabled_app_name_.c_str())) {
        LOG(ERROR) << "StatHub::Init - App " << path << " isn't supported.";
        return false;
    }

    base::Time start(StatHubGetSystemTime());

    property_get(kPropNameFlushDelay, value, PROP_VAL_TO_STR(FLUSH_DB_TIMEOUT_THRESHOLD_DEF));
    flush_delay_ = atoi(value);
    if (flush_delay_<=0) {
        flush_delay_ = FLUSH_DB_TIMEOUT_THRESHOLD_DEF;
    }
    if(IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::Init - Flush delay: "  << flush_delay_;
    }

    std::string db_path_def = db_path + "/db.sql";
    property_get(kPropNameDbpath, value, db_path_def.c_str());
    db_path_ = value;

    if(IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::Init - DB path: "  << db_path.c_str();
        LOG(INFO) << "StatHub::Init - Finale DB path: "  << db_path_.c_str();
    }

    message_loop_ = message_loop;
    http_cache_ = http_cache;

    #if defined(NOT_NOW)
        db_->set_page_size(2048);
        db_->set_cache_size(32);
        //Run the database in exclusive mode. Nobody else should be accessing the
        //database while we're running, and this will give somewhat improved perf.
        db_->set_exclusive_locking();
    #endif //defined(NOT_NOW)
    db_ = new sql::Connection();
    if (!db_->Open(FilePath(db_path_.c_str()))) {
        LOG(ERROR) << "StatHub::Init - Unable to open DB " << db_path_.c_str();
        Release();
        return false;
    }

    // Scope initialization in a transaction so we can't be partially initialized.
    if (!StatHubBeginTransaction(db_)) {
        LOG(ERROR) << "StatHub::Init - Unable to start transaction";
        Release();
        return false;
    }

    // Create tables.
    if (!InitTables()) {
        LOG(ERROR) << "StatHub::Init - Unable to initialize DB tables";
        Release();
        return false;
    }

    //load mandatory plugins
    LoadPlugin("pp_proc_plugin.so");
    LoadPlugin("pageload_proc_plugin.so");

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

    std::string proc_name;
    for (StatProcessor* processor=first_processor_; processor!=NULL;) {
        if (!processor->OnGetProcName(proc_name)) {
            proc_name = "Undefined";
        }
        if(!processor->OnInit(db_, message_loop_)) {
            LOG(INFO) << "StatHub::Init - processor " << proc_name.c_str() << " initialization failed!";
            processor = DeleteProcessor(processor);
        } else {
            LOG(INFO) << "StatHub::Init - processor " << proc_name.c_str() << " is ready.";
            unsigned int cmd_mask;
            if (processor->OnGetCmdMask(cmd_mask)) {
                cmd_mask_ |= cmd_mask;
            }
            processor=processor->next_ ;
        }
    }

    // Initialization is complete.
    if (!StatHubCommitTransaction(db_)) {
        LOG(ERROR) << "StatHub::Init - Unable to commist transaction";
        Release();
        return false;
    }

    for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_ ) {
        processor->OnFetchDb(db_);
    }

    thread_ = new base::Thread("event_handler");
    if (!thread_->StartWithOptions(base::Thread::Options(MessageLoop::TYPE_IO, 0))) {
        LOG(ERROR) << "StatHub::Init event thread start error";
        Release();
        return false;
    }

    ready_ = true;
    if(IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::Init: Init DB Time: " << StatHubGetTimeDeltaInMs(start, StatHubGetSystemTime());
    }

    LOG(INFO) << "netstack: StatHub was initialized";
    return true;
}

void StatHub::Release() {
    if(IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::Release";
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

void StatHub::MainUrlLoaded(const char* url) {
    flush_db_request_time_ = StatHubGetSystemTime();
    flush_db_required_ = true;
    if (!flush_db_scheduled_) {
        flush_db_scheduled_ = true;
        if(IsVerboseEnabled()) {
            LOG(INFO) << "CacheDatabase::MainUrlLoaded : Request DB flush (" << flush_delay_ << ")";
        }
        message_loop_->PostDelayedTask(FROM_HERE, NewRunnableFunction(&DoFlushDB, this), flush_delay_);
    }
}

void StatHub::Cmd(StatHubTimeStamp timestamp, unsigned short cmd, void* param1, int sizeofparam1, void* param2, int sizeofparam2) {
    switch (cmd) {
        case INPUT_CMD_WK_MMC_CLEAR:
            for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_ ) {
                processor->OnClearDb(db_);
            }
            break;
        default:
            for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_ ) {
                processor->OnCmd(timestamp, cmd, param1, sizeofparam1, param2, sizeofparam2);
            }
            break;
    }
    if (INPUT_CMD_WK_MAIN_URL_LOADED==cmd) {
        MainUrlLoaded((const char*)param1);
    }
}

void StatHub::FlushDBrequest() {
    if(IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::FlushDBrequest : Start";
    }

    int delta = StatHubGetTimeDeltaInMs(flush_db_request_time_, StatHubGetSystemTime());
    flush_db_scheduled_ = false;
    if (flush_db_required_) {
        if(IsVerboseEnabled()) {
            LOG(INFO) << "StatHub::FlushDBrequest : Flush - " << delta;
        }

        if (delta>=flush_delay_) {
           FlushDB();
        }
        else {
            if (!flush_db_scheduled_) {
               flush_db_scheduled_ = true;
               if(IsVerboseEnabled()) {
                   LOG(INFO) << "StatHub::FlushDBrequest : Restart - " << flush_delay_ - delta;
               }
               thread_->message_loop()->PostDelayedTask(FROM_HERE, NewRunnableFunction(&DoFlushDB, this), flush_delay_ - delta);
            }
        }
    }
}

bool StatHub::FlushDB() {
    if(IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::FlushDB: Begin";
    }
    base::Time start(StatHubGetSystemTime());

    for (StatProcessor* processor=first_processor_; processor!=NULL; processor=processor->next_ ) {
        processor->OnFlushDb(db_);
    }

    if(IsVerboseEnabled()) {
        LOG(INFO) << "StatHub::FlushDB time :" << StatHubGetTimeDeltaInMs(start, StatHubGetSystemTime());
    }
    return true;
}

}  // namespace stat_hub

