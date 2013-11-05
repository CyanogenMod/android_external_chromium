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

#include <unistd.h>
#include <string>
#include <set>
#include <stdio.h>
#include <cutils/properties.h>
#include <cutils/log.h>

#include "build/build_config.h"
#include "googleurl/src/gurl.h"
#include "base/compiler_specific.h"
#include "base/task.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "net/base/host_port_pair.h"
#include "net/base/completion_callback.h"
#include "net/base/filter.h"
#include "net/base/net_log.h"
#include "net/base/net_errors.h"
#include "net/base/io_buffer.h"
#include "net/disk_cache/hash.h"
#include "net/http/http_cache_transaction.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_info.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "net/socket/client_socket_pool_manager.h"

#include "stat_hub.h"
#include "stat_hub_api.h"

typedef std::multimap<unsigned int, StatHubCmd*> StatHubCmdMapType;
StatHubCmdMapType stat_hub_cmd_map_;

typedef std::set<unsigned int> StatHubContextSetType;
StatHubContextSetType stat_hub_context_set_;

// ======================================= Fetch Interface ==============================================
#define READ_BUF_SIZE               (50*1024)

class FetchRequest : public net::URLRequest::Delegate {
public:
    explicit FetchRequest(void* cookie):
        cookie_(cookie),
        read_in_progress_(false),
        no_context_(false),
        buf_(NULL),
        ALLOW_THIS_IN_INITIALIZER_LIST(start_callback_(this, &FetchRequest::OnStartComplete)),
        ALLOW_THIS_IN_INITIALIZER_LIST(read_callback_(this, &FetchRequest::OnReadComplete)) {
    }

    //=========================================================================
    bool StartFetch(net::URLRequestContext* context) {
        StatHubContextSetType::iterator context_iter = stat_hub_context_set_.find((unsigned int)context);
        if (context_iter == stat_hub_context_set_.end()) {
            SLOGE("netstack: STAT_HUB - Undefined context %08X for %s",
                (unsigned int)context, request_info_->url.spec().c_str());
            delete this;
            return false;
        }
        net::HttpRequestInfo* request_info = stat_hub::StatHub::GetInstance()->FetchGetRequestInfo(cookie_);
        if (NULL==request_info) {
            delete this;
            return false;
        }
        request_info_.reset(request_info);
        request_.reset(new net::URLRequest(request_info_->url, this));
        if (STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
            SLOGD("netstack: STAT_HUB - Fetch with context: %s (%08X)", request_info_->url.spec().c_str(), (int)context);
        }
        request_->SetExtraRequestHeaders(request_info_->extra_headers);
        request_->set_method(request_info->method);
        request_->set_load_flags(request_info->load_flags);
        request_->set_priority(request_info->priority);
        request_->set_context(context);
        request_->Start();
        return true;
    }

    //=========================================================================
    bool StartFetch() {
        net::HttpRequestInfo* request_info = stat_hub::StatHub::GetInstance()->FetchGetRequestInfo(cookie_);
        if (NULL==request_info) {
            delete this;
            return false;
        }
        request_info_.reset(request_info);

        int rv = StatHubGetHttpCache()->CreateTransaction(&trans_);
        if (rv!=net::OK) {
            SLOGE("netstack: STAT_HUB - Unable to create Fetch transaction: %s", request_info_->url.spec().c_str());
            delete this;
            return false;
        }
        no_context_ = true;
        StatHubCmd* cmd = StatHubCmdCreate(SH_CMD_CH_URL_REQUEST, SH_ACTION_WILL_START);
        if (NULL!=cmd) {
            StatHubCmdAddParamAsString(cmd, request_info_->url.spec().c_str());
            StatHubCmdAddParamAsString(cmd, request_info_->extra_headers.ToString().c_str());
            StatHubCmdAddParamAsBool(cmd, true);
            StatHubCmdCommit(cmd);
        }
        if (STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
            SLOGD("netstack: STAT_HUB - Fetch without context: %s", request_info_->url.spec().c_str());
        }
        rv = trans_->Start(request_info_.get(), &start_callback_, net::BoundNetLog());
        if (rv!=net::ERR_IO_PENDING) {
            OnStartComplete(rv);
        }
        return true;
    }

    // Called upon a server-initiated redirect.  The delegate may call the
    // request's Cancel method to prevent the redirect from being followed.
    // Since there may be multiple chained redirects, there may also be more
    // than one redirect call.
    //
    // When this function is called, the request will still contain the
    // original URL, the destination of the redirect is provided in 'new_url'.
    // If the delegate does not cancel the request and |*defer_redirect| is
    // false, then the redirect will be followed, and the request's URL will be
    // changed to the new URL.  Otherwise if the delegate does not cancel the
    // request and |*defer_redirect| is true, then the redirect will be
    // followed once FollowDeferredRedirect is called on the URLRequest.
    //
    // The caller must set |*defer_redirect| to false, so that delegates do not
    // need to set it if they are happy with the default behavior of not
    // deferring redirect.
virtual void OnReceivedRedirect(net::URLRequest* new_request, const GURL& new_url, bool* defer_redirect)
    {
        if (STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
            SLOGD("netstack: STAT_HUB - Fetch redirect canceled: %s -> %s",
                request_info_->url.spec().c_str(), new_url.spec().c_str());
        }
        if (NULL!=new_request) {
            new_request->Cancel();
        }
    }

    // After calling Start(), the delegate will receive an OnResponseStarted
    // callback when the request has completed.  If an error occurred, the
    // request->status() will be set.  On success, all redirects have been
    // followed and the final response is beginning to arrive.  At this point,
    // meta data about the response is available, including for example HTTP
    // response headers if this is a request for a HTTP resource.
virtual void OnResponseStarted(net::URLRequest* request) {
        int error_code = net::ERR_UNEXPECTED;
        const net::HttpResponseInfo* response_info = NULL;
        if (NULL!=request &&request->status().is_success()) {
            error_code = net::OK;
            response_info = &request->response_info();
        }
        if (!OnStartCompleteHelper(error_code, response_info)) {
            request->Cancel();
        }
        else {
            if (error_code == net::OK) {
                StartReadFromRequest();
            }
        }
    }

    // Called when the a Read of the response body is completed after an
    // IO_PENDING status from a Read() call.
    // The data read is filled into the buffer which the caller passed
    // to Read() previously.
    //
    // If an error occurred, request->status() will contain the error,
    // and bytes read will be -1.
virtual void OnReadCompleted(net::URLRequest* request, int bytesRead) {
        if (NULL!=request && request->status().is_success() && -1!=bytesRead) {
            ReadDone(bytesRead);
            StartReadFromRequest();
        }
        else {
            Finish(net::ERR_UNEXPECTED);
        }
    }

private:

    //=========================================================================
    bool OnStartCompleteHelper(int error_code, const net::HttpResponseInfo* response_info) {
        if (STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
            SLOGD("netstack: STAT_HUB - Fetch transaction started: %s (%d)", request_info_->url.spec().c_str(), error_code);
        }

        if (error_code == net::OK) {
            if (!stat_hub::StatHub::GetInstance()->FetchStartComplete(cookie_, response_info, error_code)) {
                if (STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
                    SLOGD("netstack: STAT_HUB - Transaction can't be started: %s",
                        request_info_->url.spec().c_str());
                }
                return false;
            }
            if (no_context_) {
                StatHubCmd* cmd = StatHubCmdCreate(SH_CMD_CH_URL_REQUEST, SH_ACTION_DID_START);
                if (NULL!=cmd) {
                    StatHubCmdAddParamAsString(cmd, request_info_->url.spec().c_str());
                    StatHubCmdAddParamAsBuf(cmd, response_info->headers->raw_headers().data(), response_info->headers->raw_headers().size());
                    StatHubCmdCommit(cmd);
                }
            }
            buf_ = new net::IOBuffer(READ_BUF_SIZE);
        }
        else {
            SLOGE("netstack: STAT_HUB - Fetch ERROR while starting transaction %d : %s",
                error_code, request_info_->url.spec().c_str());
            Finish(error_code);
        }
        return true;
    }

    //=========================================================================
    void Finish(int error_code) {
        if(STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
            SLOGD("netstack: STAT_HUB - Fetch done: %s (%d)", request_info_->url.spec().c_str(), error_code);
        }
        stat_hub::StatHub::GetInstance()->FetchDone(cookie_, error_code);
        if(no_context_) {
            StatHubCmd* cmd = StatHubCmdCreate(SH_CMD_CH_URL_REQUEST, SH_ACTION_DID_FINISH);
            if (NULL!=cmd) {
                StatHubCmdAddParamAsString(cmd, request_info_->url.spec().c_str());
                StatHubCmdCommit(cmd);
            }
        }
        delete this;
    }

    //=========================================================================
    void ReadDone(int bytes_to_read) {
        if (bytes_to_read > 0) {
            if (STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
                SLOGD("netstack: STAT_HUB - Fetch read: %s (%d)", request_info_->url.spec().c_str(), bytes_to_read);
            }
            stat_hub::StatHub::GetInstance()->FetchReadComplete(cookie_, buf_->data(), bytes_to_read);
        }
    }

    //=========================================================================
    void StartReadFromRequest()
    {
        int bytes_read = 0;

        if (!request_->Read(buf_, READ_BUF_SIZE, &bytes_read)) {
            if (request_->status().is_io_pending()) {
                // Wait for OnReadCompleted()
                return;
            }
            SLOGE("netstack: STAT_HUB - Fetch read from request ERROR: %s", request_info_->url.spec().c_str());
            Finish(net::ERR_UNEXPECTED);
            return;
        }
        if (bytes_read) {
            ReadDone(bytes_read);
            StartReadFromRequest();
        }
        else {
            //Done: bytes_read == 0 indicates finished
            Finish(net::OK);
        }
    }

    //=========================================================================
    void StartRead() {
        int rv = trans_->Read(buf_, READ_BUF_SIZE, &read_callback_);
        if (rv >= 0) {
            ReadDone(rv);
            Finish(net::OK);
        }
        else {
            if (rv == net::ERR_IO_PENDING) {
                read_in_progress_ = true;
            }
            else {
                SLOGE("netstack: STAT_HUB - Fetch read ERROR: %d : %s",
                    rv, request_info_->url.spec().c_str());
                Finish(rv);
            }
        }
    }

    //=========================================================================
    void OnStartComplete(int error_code) {
        // If the transaction was destroyed, then the job was cancelled, and
        // we can just ignore this notification.
        if (!trans_.get()) {
            Finish(net::ERR_UNEXPECTED);
        }
        OnStartCompleteHelper(error_code, trans_->GetResponseInfo());
        if (error_code == net::OK) {
            StartRead();
        }
    }

    //=========================================================================
    void OnReadComplete(int error_code) {
        read_in_progress_ = false;
        if (error_code <= net::OK) {
            Finish(error_code);
        }
        else {
            ReadDone(error_code);
            StartRead();
        }
    }


    void* cookie_;
    bool read_in_progress_;
    bool no_context_;

    scoped_refptr<net::IOBuffer>        buf_;
    scoped_ptr<net::HttpRequestInfo>    request_info_;
    scoped_ptr<net::HttpTransaction>    trans_;

    net::CompletionCallbackImpl<FetchRequest> start_callback_;
    net::CompletionCallbackImpl<FetchRequest> read_callback_;

    scoped_ptr<net::URLRequest> request_;

    DISALLOW_COPY_AND_ASSIGN(FetchRequest);
};

static void DoFetch(void* cookie, net::URLRequestContext* context) {
    FetchRequest* fetch = new FetchRequest(cookie);
    if (context) {
        fetch->StartFetch(context);
    }
    else {
        fetch->StartFetch();
    }
}

void StatHubURLRequestContextCreated(unsigned int context) {
    if(STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
        SLOGD("netstack: STAT_HUB - URL request context created: %08X ", context);
    }
    stat_hub_context_set_.insert(context);
}

void StatHubURLRequestContextDestroyed(unsigned int context) {
    if(STAT_HUB_IS_VERBOSE_LEVEL_DEBUG) {
        SLOGD("netstack: STAT_HUB - URL request context destroyed: %08X ", context);
    }
    stat_hub_context_set_.erase(context);
}

bool StatHubIsInDC(const char* url) {
    return stat_hub::StatHub::GetInstance()->IsInDC(url);
}

bool StatHubIsPreloaded(const char* url, std::string& headers, char*& data, int& size) {
    return stat_hub::StatHub::GetInstance()->IsPreloaded(url, headers, data, size);
}

bool StatHubReleasePreloaded(const char* url) {
    return stat_hub::StatHub::GetInstance()->ReleasePreloaded(url);
}

bool StatHubIsPreloaderEnabled() {
    return stat_hub::StatHub::GetInstance()->IsPreloaderEnabled();
}

// ======================================= Commands ==============================================
StatHubCmd* StatHubCmd::Create(StatHubCmdType cmd, StatHubActionType action, unsigned int cookie) {
    unsigned int cmd_mask = stat_hub::StatHub::GetInstance()->GetCmdMask();

    if ((cmd>SH_CMD_USER_DEFINED || (cmd_mask&(1<<cmd))) && stat_hub::StatHub::GetInstance()->IsReady()) {
        return new StatHubCmd(cmd, action, cookie);
    }
    return NULL;
}

void StatHubCmd::Release(StatHubCmd* cmd) {
    if (NULL!=cmd) {
        cmd->referenced_--;
        if(0==cmd->referenced_) {
            delete cmd;
        }
    }
}

StatHubCmd::StatHubCmdParam::StatHubCmdParam(const char* param): param_size_(0) {
    if(NULL!=param) {
        param_size_= strlen(param)+1;
        if(0!=param_size_) {
            param_ = new char[param_size_];
            memcpy(param_, param, param_size_);
        }
    }
}

StatHubCmd::StatHubCmdParam::StatHubCmdParam(const void* param, unsigned int param_size): param_size_(0) {
    if(NULL!=param && 0!=param_size) {
        param_size_ = param_size;
        param_ = new char[param_size_];
        memcpy(param_, param, param_size_);
    }
}

// ======================================= Exports ==============================================

bool StatHubIsVerboseEnabled() {
    return stat_hub::StatHub::GetInstance()->IsVerboseEnabled();
}

bool StatHubIsPerfEnabled() {
    return stat_hub::StatHub::GetInstance()->IsPerfEnabled();
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

const char* StatHubGetHostFromUrl(const std::string& url, std::string& host) {
    GURL dest(url);
    host = dest.GetOrigin().spec();
    return host.c_str();
}

const char* StatHubGetHostPortFromUrl(const std::string& url, std::string& host_port) {
    GURL dest(url);

    net::HostPortPair origin_host_port =
        net::HostPortPair(dest.HostNoBrackets(),dest.EffectiveIntPort());
    host_port = origin_host_port.ToString();
    return host_port.c_str();
}

int StatHubGetMaxSocketsPerGroup() {
    return net::ClientSocketPoolManager::max_sockets_per_group();
}

unsigned int StatHubHash(const char* str) {
    return disk_cache::Hash(str, strlen(str));
}

bool StatHubFetch(void* cookie, net::URLRequestContext* context) {
    MessageLoop* message_loop = StatHubGetIoMessageLoop();
    if (NULL!=message_loop) {
        message_loop->PostTask(FROM_HERE, NewRunnableFunction(&DoFetch, cookie, context));
        return true;
    }
    return false;
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

MessageLoop* StatHubGetIoMessageLoop() {
    return stat_hub::StatHub::GetInstance()->GetIoMessageLoop();
}

void StatHubSetIoMessageLoop(MessageLoop* message_loop) {
    stat_hub::StatHub::GetInstance()->SetIoMessageLoop(message_loop);
    if (NULL!=StatHubGetHttpCache()) {
        StatHubCmd* cmd = StatHubCmdCreate(SH_CMD_CH_URL_REQUEST, SH_ACTION_FETCH_DELAYED);
        if (NULL!=cmd) {
            StatHubCmdCommit(cmd);
        }
    }
}

void StatHubSetHttpCache(net::HttpCache* cache) {
    stat_hub::StatHub::GetInstance()->SetHttpCache(cache);
    if (NULL!=StatHubGetIoMessageLoop()) {
        StatHubCmd* cmd = StatHubCmdCreate(SH_CMD_CH_URL_REQUEST, SH_ACTION_FETCH_DELAYED);
        if (NULL!=cmd) {
            StatHubCmdCommit(cmd);
        }
    }
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

void CmdCommitProxy(StatHubCmd* cmd) {
    stat_hub::StatHub::GetInstance()->Cmd(cmd);
    StatHubCmd::Release(cmd);
}

// ================================ StatHub Interface ====================================
void StatHubDebugLog(const char* str) {
    LOG(INFO) << str;
}

bool StatHubIsReady() {
    return stat_hub::StatHub::GetInstance()->IsReady();
}

bool StatHubIsProcReady(const char* name) {
    return stat_hub::StatHub::GetInstance()->IsProcReady(name);
}

// ================================ StatHub CMD Interface ====================================
StatHubCmd* StatHubCmdCreate(StatHubCmdType cmd_id, StatHubActionType action, unsigned int cookie) {
    return StatHubCmd::Create(cmd_id, action, cookie);
}

void StatHubCmdAddParamAsUint32(StatHubCmd* cmd, unsigned int param) {
    if(NULL!=cmd) {
        cmd->AddParamAsUint32(param);
    }
}

void StatHubCmdAddParamAsString(StatHubCmd* cmd, const char* param) {
    if(NULL!=cmd) {
        cmd->AddParamAsString(param);
    }
}

void StatHubCmdAddParamAsBuf(StatHubCmd* cmd, const void* param, unsigned int size) {
    if(NULL!=cmd) {
        cmd->AddParamAsBuf(param, size);
    }
}

void StatHubCmdTimeStamp(StatHubCmd* cmd) {
    if(NULL!=cmd) {
        cmd->SetStartTimeStamp(StatHubTimeStamp::NowFromSystemTime());
    }
}

void StatHubCmdAddParamAsBool(StatHubCmd* cmd, bool param) {
    if(NULL!=cmd) {
        cmd->AddParamAsBool(param);
    }
}

void StatHubCmdCommit(StatHubCmd* cmd) {
    if(NULL!=cmd) {
        if (STAT_HUB_IS_VERBOSE_LEVEL_DEBUG && STAT_HUB_DEV_LOG_ENABLED) {
            SLOGD("netstack: STAT_HUB - StatHubCmdCommit CMD:%d Action:%d", cmd->GetCmd(), cmd->GetAction());
        }
        cmd->SetCommitTimeStamp(StatHubTimeStamp::NowFromSystemTime());
        stat_hub::StatHub::GetInstance()->GetThread()->message_loop()->PostTask( FROM_HERE, NewRunnableFunction(
            &CmdCommitProxy, cmd));
    }
}

void StatHubCmdCommitDelayed(StatHubCmd* cmd, unsigned int delay_ms) {
    if(NULL!=cmd) {
        cmd->SetCommitTimeStamp(StatHubTimeStamp::NowFromSystemTime());
        stat_hub::StatHub::GetInstance()->GetThread()->message_loop()->PostDelayedTask( FROM_HERE, NewRunnableFunction(
            &CmdCommitProxy, cmd), (int64) delay_ms);
    }
}

void StatHubCmdPush(StatHubCmd* cmd) {
    if(NULL!=cmd) {
        stat_hub_cmd_map_.insert(std::pair<unsigned int, StatHubCmd*>(cmd->GetCookie(), cmd));
    }
}

StatHubCmd* StatHubCmdPop(unsigned int cookie, StatHubCmdType cmd_id, StatHubActionType action) {
    unsigned int cmd_mask = stat_hub::StatHub::GetInstance()->GetCmdMask();

    if ((cmd_id>SH_CMD_USER_DEFINED || (cmd_mask&(1<<cmd_id))) && stat_hub::StatHub::GetInstance()->IsReady()) {
        StatHubCmdMapType::iterator iter;
        std::pair<StatHubCmdMapType::iterator,StatHubCmdMapType::iterator> ret = stat_hub_cmd_map_.equal_range(cookie);

        for (iter=ret.first; iter!=ret.second; ++iter) {
            StatHubCmd* cmd = (*iter).second;
            if (NULL!=cmd && cmd->GetCmd()==cmd_id && cmd->GetAction()==action) {
                stat_hub_cmd_map_.erase(iter);
                return cmd;
            }
        }
    }
    return NULL;
}
