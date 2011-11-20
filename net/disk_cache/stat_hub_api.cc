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

#include <unistd.h>
#include <string>
#include <cutils/properties.h>

#include "build/build_config.h"
#include "googleurl/src/gurl.h"
#include "base/compiler_specific.h"
#include "base/task.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "net/base/completion_callback.h"
#include "net/base/net_log.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"
#include "net/base/load_flags.h"
#include "net/base/io_buffer.h"
#include "net/disk_cache/hash.h"
#if defined(STAT_HUB_PRECONNECT_ENABLED)
    #include "net/http/preconnect.h"
#endif //defined(STAT_HUB_PRECONNECT_ENABLED)
#include "net/http/http_cache_transaction.h"
#include "net/http/http_request_info.h"
#include "net/http/http_request_headers.h"

#include "stat_hub.h"
#include "stat_hub_api.h"

#define READ_BUF_SIZE               (50*1024)

class FetchRequest {
 public:
     explicit FetchRequest(std::string& dest, std::string& headers):
        dest_(dest),
        headers_(headers),
        ALLOW_THIS_IN_INITIALIZER_LIST(start_callback_(this, &FetchRequest::OnStartComplete)),
        ALLOW_THIS_IN_INITIALIZER_LIST(read_callback_(this, &FetchRequest::OnReadComplete)),
        read_in_progress_(false),
        buf_(NULL){
     }

     void StartFetch(net::HttpCache* cache){
        request_info_.reset(new net::HttpRequestInfo());
        if(StatHubIsVerboseEnabled()) {
            LOG(INFO) << "Fetch: " << dest_.spec().c_str();
        }
        request_info_->url = dest_;
        request_info_->motivation = net::HttpRequestInfo::PRECONNECT_MOTIVATED;
        request_info_->priority = net::LOWEST;
        request_info_->method = net::HttpRequestHeaders::kGetMethod;
        request_info_->load_flags |= net::LOAD_PREFETCH;
        request_info_->extra_headers.AddHeadersFromString(headers_);
        int rv = cache->CreateTransaction(&trans_);
        rv = trans_->Start(request_info_.get(), &start_callback_, net::BoundNetLog());
        if (rv != net::ERR_IO_PENDING) {
            delete this;
        }
     }

 private:

     virtual ~FetchRequest() {
     }

  void Read() {
         int rv = trans_->Read(buf_, READ_BUF_SIZE, &read_callback_);
         if (rv >= 0) {
             delete this;
         }
         else {
             if (rv == net::ERR_IO_PENDING) {
                 read_in_progress_ = true;
             }
             else {
                 LOG(INFO) << "FetchRequest::Read : ERROR " << rv << ":" << dest_.spec().c_str();
                 delete this;
             }
         }
    }

  void OnStartComplete(int error_code) {
      if (error_code == net::OK) {
        buf_ = new net::IOBuffer(READ_BUF_SIZE);

        Read();
      }
      else {
          LOG(INFO) << "FetchRequest::OnStartComplete : ERROR " << error_code << ":" << dest_.spec().c_str();
          delete this;
      }
  }

  void OnReadComplete(int error_code) {
      read_in_progress_ = false;
      if (error_code == net::OK) {
          delete this;
      }
      else {
        Read();
      }
  }

  GURL dest_;
  std::string headers_;
  scoped_ptr<net::HttpRequestInfo> request_info_;
  scoped_ptr<net::HttpTransaction> trans_;

  net::CompletionCallbackImpl<FetchRequest> start_callback_;
  net::CompletionCallbackImpl<FetchRequest> read_callback_;

  bool read_in_progress_;
  scoped_refptr<net::IOBuffer> buf_;

  DISALLOW_COPY_AND_ASSIGN(FetchRequest);
};

static void DoPreconnect(net::HttpCache* cache, std::string* dest, uint32 count) {
    if(StatHubIsVerboseEnabled()) {
        LOG(INFO) << "Preconnect: " << dest->c_str() << " : " << count;
    }
    if (NULL!=cache) {
        net::HttpNetworkSession* session = cache->GetSession();
        if (NULL!=session) {
            #if defined(STAT_HUB_PRECONNECT_ENABLED)
                net::Preconnect::DoPreconnect(session, GURL(*dest), count);
            #endif //defined(STAT_HUB_PRECONNECT_ENABLED)
        }
    }
    delete dest;
}

static void DoFetch(net::HttpCache* cache, std::string* dest, std::string* headers) {
    FetchRequest* fetch = new FetchRequest(*dest, *headers);
    fetch->StartFetch(cache);
    delete dest;
    delete headers;
}


// ======================================= Exports ==============================================

bool StatHubIsVerboseEnabled() {
    return stat_hub::StatHub::GetInstance()->IsVerboseEnabled();
}

StatHubVerboseLevel StatHubGetVerboseLevel() {
    return stat_hub::StatHub::GetInstance()->GetVerboseLevel();
}

base::Time StatHubGetSystemTime() {
    return base::Time::NowFromSystemTime();
}

int StatHubGetTimeDeltaInMs(const base::Time& start_time, const base::Time& finish_time) {
    base::TimeDelta delta = finish_time - start_time;
    return (int)delta.InMilliseconds(); //int64
}

const char* StatHubGetHostFromUrl(std::string& url, std::string& host) {
    GURL dest(url);
    host = dest.GetOrigin().spec();
    return host.c_str();
}

unsigned int StatHubHash(const char* str) {
    return disk_cache::Hash(str, strlen(str));
}

void StatHubPreconnect(MessageLoop* message_loop, net::HttpCache* cache, const char* url,  uint32 count) {
    message_loop->PostTask(FROM_HERE, NewRunnableFunction(&DoPreconnect, cache, new std::string(url), count));
}

void StatHubFetch(MessageLoop* message_loop, net::HttpCache* cache, const char* url, const char* headers) {
    message_loop->PostTask(FROM_HERE, NewRunnableFunction(&DoFetch, cache, new std::string(url), new std::string(headers)));
}

bool StatHubGetDBmetaData(const char* key, std::string& val) {
    return stat_hub::StatHub::GetInstance()->GetDBmetaData(key, val);
}

bool StatHubSetDBmetaData(const char* key, const char* val) {
    return stat_hub::StatHub::GetInstance()->SetDBmetaData(key, val);
}

net::HttpCache* StatHubGetHttpCache() {
    return stat_hub::StatHub::GetInstance()->GetHttpCache();
}

// ================================ StatHub SQL Interface ====================================

bool StatHubBeginTransaction(sql::Connection* db) {
    return db->BeginTransaction();
}

bool StatHubCommitTransaction(sql::Connection* db) {
    return db->CommitTransaction();
}

bool StatHubDoesTableExist(sql::Connection* db, const char* table_name) {
    return db->DoesTableExist(table_name);
}

bool StatHubExecute(sql::Connection* db, const char* sql) {
    return db->Execute(sql);
}

sql::Statement* StatHubGetStatement(sql::Connection* db, const sql::StatementID& id, const char* sql) {
    if(NULL!=db && NULL!=sql) {
        return new sql::Statement(db->GetCachedStatement(id, sql));
    }
    return NULL;
}

void StatHubReleaseStatement(sql::Statement* st) {
    if (NULL!=st) {
        delete st;
    }
}

bool StatHubStatementStep(sql::Statement* st) {
    if (NULL!=st) {
        return st->Step();
    }
    return false;
}

bool StatHubStatementRun(sql::Statement* st) {
    if (NULL!=st) {
        return st->Run();
    }
    return false;
}

void StatHubStatementReset(sql::Statement* st) {
    if (NULL!=st) {
        st->Reset();
    }
}

int StatHubStatementColumnInt(sql::Statement* st, int col) {
    if (NULL!=st) {
        return st->ColumnInt(col);
    }
    return 0;
}

int64 StatHubStatementColumnInt64(sql::Statement* st, int col) {
    if (NULL!=st) {
        return st->ColumnInt64(col);
    }
    return 0;
}

bool StatHubStatementColumnBool(sql::Statement* st, int col) {
    if (NULL!=st) {
        return st->ColumnBool(col);
    }
    return false;
}

std::string StatHubStatementColumnString(sql::Statement* st, int col) {
    if (NULL!=st) {
        return st->ColumnString(col);
    }
    return std::string("");
}

bool StatHubStatementBindInt(sql::Statement* st, int col, int val) {
    if (NULL!=st) {
        return st->BindInt(col, val);
    }
    return false;
}

bool StatHubStatementBindInt64(sql::Statement* st, int col, int64 val) {
    if (NULL!=st) {
        return st->BindInt64(col, val);
    }
    return false;
}

bool StatHubStatementBindBool(sql::Statement* st, int col, bool val) {
    if (NULL!=st) {
        return st->BindBool(col, val);
    }
    return false;
}

bool StatHubStatementBindCString(sql::Statement* st, int col, const char* val) {
    if (NULL!=st) {
        return st->BindCString(col, val);
    }
    return false;
}

// ============================ StatHub Functional Interface Proxies ===============================

void CmdProxy(StatHubTimeStamp timestamp, unsigned short cmd, void* param1, int sizeofparam1, void* param2, int sizeofparam2) {
    stat_hub::StatHub::GetInstance()->Cmd(timestamp, cmd, param1, sizeofparam1, param2, sizeofparam2);
    if (sizeofparam1) {
        delete (char*)param1;
    }
    if (sizeofparam2) {
        delete (char*)param2;
    }
}

// ================================ StatHub Functional Interface ====================================
void StatHubCmd(unsigned short cmd, void* param1, int sizeofparam1, void* param2, int sizeofparam2){
    unsigned int cmd_mask = stat_hub::StatHub::GetInstance()->GetCmdMask();

    if ((cmd>INPUT_CMD_USER_DEFINED || (cmd_mask&(1<<cmd))) && stat_hub::StatHub::GetInstance()->IsReady()) {
        // create persistence storage to safely pass data to another thread
        char* tmp_param1 = (char*)param1;
        char* tmp_param2 = (char*)param2;
        if (sizeofparam1) {
            tmp_param1 = new char[sizeofparam1];
            memcpy(tmp_param1, param1, sizeofparam1);
        }
        if (sizeofparam2) {
            tmp_param2 = new char[sizeofparam2];
            memcpy(tmp_param2, param2, sizeofparam2);
        }
        stat_hub::StatHub::GetInstance()->GetThread()->message_loop()->PostTask( FROM_HERE, NewRunnableFunction(
            &CmdProxy, base::Time::NowFromSystemTime(), cmd, (void*)tmp_param1, sizeofparam1, (void*)tmp_param2, sizeofparam2));
    }
}

void StatHubUpdateMainUrl(const char* url) {
    if(NULL!=url) {
        StatHubCmd(INPUT_CMD_WK_MAIN_URL, (void*)url, strlen(url)+1, NULL, 0);
    }
}

void StatHubUpdateSubUrl(const char* main_url, const char* sub_url) {
    if(NULL!=main_url && NULL!=sub_url) {
        StatHubCmd(INPUT_CMD_WK_SUB_URL_REQUEST, (void*)main_url, strlen(main_url)+1, (void*)sub_url, strlen(sub_url)+1);
    }
}

void StatHubMainUrlLoaded(const char* url) {
    if(NULL!=url) {
        StatHubCmd(INPUT_CMD_WK_MAIN_URL_LOADED, (void*)url, strlen(url)+1, NULL, 0);
    }
}

bool StatHubIsProcReady(const char* name) {
    return stat_hub::StatHub::GetInstance()->IsProcReady(name);
}
