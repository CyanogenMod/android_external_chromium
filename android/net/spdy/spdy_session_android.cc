// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_session.h"

#include "base/basictypes.h"
#include "base/linked_ptr.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/metrics/stats_counters.h"
#include "base/stl_util-inl.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "net/base/connection_type_histograms.h"
#include "net/base/net_log.h"
#include "net/base/net_util.h"
#include "net/http/http_network_session.h"
#include "net/socket/ssl_client_socket.h"
#include "net/spdy/spdy_frame_builder.h"
#include "net/spdy/spdy_protocol.h"
#include "net/spdy/spdy_settings_storage.h"
#include "net/spdy/spdy_stream.h"

namespace net {

namespace {

class NetLogSpdySynParameter : public NetLog::EventParameters {
 public:
  NetLogSpdySynParameter(const linked_ptr<spdy::SpdyHeaderBlock>& headers,
                         spdy::SpdyControlFlags flags,
                         spdy::SpdyStreamId id)
      : headers_(headers), flags_(flags), id_(id) {}

  Value* ToValue() const {
    DictionaryValue* dict = new DictionaryValue();
    ListValue* headers_list = new ListValue();
    for (spdy::SpdyHeaderBlock::const_iterator it = headers_->begin();
         it != headers_->end(); ++it) {
      headers_list->Append(new StringValue(base::StringPrintf(
          "%s: %s", it->first.c_str(), it->second.c_str())));
    }
    dict->SetInteger("flags", flags_);
    dict->Set("headers", headers_list);
    dict->SetInteger("id", id_);
    return dict;
  }

 private:
  ~NetLogSpdySynParameter() {}

  const linked_ptr<spdy::SpdyHeaderBlock> headers_;
  const spdy::SpdyControlFlags flags_;
  const spdy::SpdyStreamId id_;

  DISALLOW_COPY_AND_ASSIGN(NetLogSpdySynParameter);
};

}

void SpdySession::OnSyn(const spdy::SpdySynStreamControlFrame& frame,
                        const linked_ptr<spdy::SpdyHeaderBlock>& headers) {
  spdy::SpdyStreamId stream_id = frame.stream_id();
  spdy::SpdyStreamId associated_stream_id = frame.associated_stream_id();

  if (net_log_.IsLoggingAllEvents()) {
    net_log_.AddEvent(
        NetLog::TYPE_SPDY_SESSION_PUSHED_SYN_STREAM,
        new NetLogSpdySynParameter(
            headers, static_cast<spdy::SpdyControlFlags>(frame.flags()),
            stream_id));
  }

  // Server-initiated streams should have even sequence numbers.
  if ((stream_id & 0x1) != 0) {
    LOG(ERROR) << "Received invalid OnSyn stream id " << stream_id;
    return;
  }

  if (IsStreamActive(stream_id)) {
    LOG(ERROR) << "Received OnSyn for active stream " << stream_id;
    return;
  }

  if (associated_stream_id == 0) {
    LOG(ERROR) << "Received invalid OnSyn associated stream id "
               << associated_stream_id
               << " for stream " << stream_id;
    ResetStream(stream_id, spdy::INVALID_STREAM);
    return;
  }

  streams_pushed_count_++;

  // TODO(mbelshe): DCHECK that this is a GET method?

  const std::string& path = ContainsKey(*headers, "path") ?
      headers->find("path")->second : "";

  // Verify that the response had a URL for us.
  if (path.empty()) {
    ResetStream(stream_id, spdy::PROTOCOL_ERROR);
    LOG(WARNING) << "Pushed stream did not contain a path.";
    return;
  }

  if (!IsStreamActive(associated_stream_id)) {
    LOG(ERROR) << "Received OnSyn with inactive associated stream "
               << associated_stream_id;
    ResetStream(stream_id, spdy::INVALID_ASSOCIATED_STREAM);
    return;
  }

  // TODO(erikchen): Actually do something with the associated id.

  // There should not be an existing pushed stream with the same path.
  PushedStreamMap::iterator it = unclaimed_pushed_streams_.find(path);
  if (it != unclaimed_pushed_streams_.end()) {
    LOG(ERROR) << "Received duplicate pushed stream with path: " << path;
    ResetStream(stream_id, spdy::PROTOCOL_ERROR);
    return;
  }

  scoped_refptr<SpdyStream> stream =
      new SpdyStream(this, stream_id, true, net_log_);

  stream->set_path(path);

  unclaimed_pushed_streams_[path] = stream;

  ActivateStream(stream);
  stream->set_response_received();

  // Parse the headers.
  if (!Respond(*headers, stream))
    return;

  static base::StatsCounter push_requests("spdy.pushed_streams");
  push_requests.Increment();
}

}  // namespace net
