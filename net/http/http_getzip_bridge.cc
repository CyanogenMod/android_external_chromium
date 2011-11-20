/**
 * Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other *materials provided
 *     with the distribution.
 *   * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#include "net/http/http_getzip_bridge.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"

namespace net
{

void GetNextHttpRequestHeader( void*& it)
{
  if (NULL == it)return;

  HttpRequestHeaders::Iterator* myIt =
      (static_cast<HttpRequestHeaders::Iterator*> (it));

  if (!myIt->GetNext())
  {
    delete myIt;
    it = NULL;
  }
}

void GetFirstHttpRequestHeader(
    net::HttpRequestHeaders& req, void*& it)
{
  //create a new Iterator for the req
  it = new net::HttpRequestHeaders::Iterator(req);
  return GetNextHttpRequestHeader( it );
}

const std::string& GetHttpRequestHeaderName( void*& it )
{
  net::HttpRequestHeaders::Iterator* myIt =
      (static_cast<net::HttpRequestHeaders::Iterator*> (it));
  return myIt->name();
}

const std::string& GetHttpRequestHeaderValue( void*& it )
{
  net::HttpRequestHeaders::Iterator* myIt =
    (static_cast<net::HttpRequestHeaders::Iterator*> (it));
  return myIt->value();
}

bool GetHttpRequestHeaderByValue(
  net::HttpRequestHeaders& req,
  const std::string& headerName,
  std::string* headerValue)
{
  return req.GetHeader(headerName, headerValue);
}

void SetHttpRequestHeader(net::HttpRequestHeaders& req,
    const std::string& headerName, const std::string& value)
{
  req.SetHeader(headerName, value);
}

void RemoveHttpRequestHeader(net::HttpRequestHeaders& req,
    const std::string& headerName)
{
  req.RemoveHeader(headerName);
}

void RemoveHttpResponseHeader(net::HttpResponseHeaders* res,
    const std::string& headerName)
{
  if (NULL == res)
    return;

  res->RemoveHeader(headerName);
}

std::string GetHttpResponseHeaderValue(net::HttpResponseHeaders* res,
    const std::string& headerName)
{
  if (NULL == res)
    return "";
  std::string value;

  void* iter = NULL;
  std::string temp;
  while (res->EnumerateHeader(&iter, headerName, &temp)) {

        value.append(temp);
        value.append(", ");
  }
  if(value.size() > 2)
  {
    value.erase(value.size() - 2);
  }

  return value;
}

int GetHttpResponseCode(net::HttpResponseHeaders* res)
{
  if(NULL == res) return -1;

  return res->response_code();
}

bool HasHttpResponseHeader(net::HttpResponseHeaders* res,
    const std::string& headerName)
{
  if(NULL == res) return false;

  return res->HasHeader(headerName);
}

void initBridge(){ }

}; //end net

