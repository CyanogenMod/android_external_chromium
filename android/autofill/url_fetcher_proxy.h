/*
 * Copyright 2010, The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef URL_FETCHER_PROXY_H_
#define URL_FETCHER_PROXY_H_

#include "android/autofill/android_url_request_context_getter.h"
#include "android/autofill/profile_android.h"
#include "base/thread.h"
#include "common/net/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include <WebCoreSupport/autofill/MainThreadProxy.h>

#define AUTOFILL_FETCHER_ID 1

class URLFetcherProxy;

// The URLFetcherProxy uses RunnableMethod to call functions on it in
// another thread, but (since it's trying to behave like a URLFetcher)
// isn't reference counted. This specialisation makes RunnableMethod
// work with a non-reference-counted object by not manipulating the
// reference counts.
// TODO: Investigate alternatives to using RunnableMethod that don't
// expect a ref counted object so we can remove this if possible.
template <>
struct RunnableMethodTraits<class URLFetcherProxy> {
  void RetainCallee(URLFetcherProxy* obj) {
  }

  void ReleaseCallee(URLFetcherProxy* obj) {
  }
};

// A class that implements the same API as URLFetcher but instead of
// assuming that the calling thread is a chrome thread with a message
// loop, it assumes the calling thread is WebKit's main
// thread. Implemented as a wrapper round URLFetcher.
// TODO: I think this can be improved as follows, which should reduce
// our diff here and in autofill_download.cc as we'll be able to use
// a URLFetcher base class pointer.
//  * Have URLFetcherProxy extend URLFetcher and URLFetcherDelegate
//  * Delete the Delegate inner class here
//  * Update the URLFetcher::Create factory method to return
//    a URLFetcherProxy on Android.
class URLFetcherProxy : public URLFetcher::Delegate {
public:
  class Delegate {
   public:
    virtual ~Delegate() { };
    // This will be called when the URL has been fetched, successfully or not.
    // |response_code| is the HTTP response code (200, 404, etc.) if
    // applicable.  |url|, |status| and |data| are all valid until the
    // URLFetcherProxy instance is destroyed.
    virtual void OnURLFetchComplete(const URLFetcherProxy* source,
                                    const GURL& url,
                                    const URLRequestStatus& status,
                                    int response_code,
                                    const ResponseCookies& cookies,
                                    const std::string& data) = 0;
  };

  URLFetcherProxy(const GURL& url, Delegate* d,
                  const std::string& upload_content)
    : url_(url), d_(d), upload_content_(upload_content)
    {
      URLRequestContextGetter* con =  Profile::GetDefaultRequestContext();
      scoped_refptr<base::MessageLoopProxy> mlp = con->GetIOMessageLoopProxy();
      // TODO: See the template specialisation at the top of the file. Can we use
      // an alternative to RunnableMethod that doesn't expect a ref counted object?
      mlp->PostTask(FROM_HERE, NewRunnableMethod(this, &URLFetcherProxy::DoStart));
    };


  virtual ~URLFetcherProxy()
  {
    delete real_fetcher_;
  }

  virtual void OnURLFetchComplete(const URLFetcher* source,
                                  const GURL& url,
                                  const URLRequestStatus& status,
                                  int response_code,
                                  const ResponseCookies& cookies,
                                  const std::string& data)
  {
    url_ = url;
    status_ = status;
    response_code_ = response_code;
    cookies_ = cookies;
    data_ = data;
    MainThreadProxy::CallOnMainThread(DoComplete, this);
  }

  net::HttpResponseHeaders* response_headers() const {return real_fetcher_->response_headers();};

  // Returns the back-off delay before the request will be retried,
  // when a 5xx response was received.
  base::TimeDelta backoff_delay() const { return real_fetcher_->backoff_delay();};

private:
  void DoStart()
  {
    real_fetcher_ = URLFetcher::Create(AUTOFILL_FETCHER_ID, url_, URLFetcher::POST, this);
    real_fetcher_->set_automatcally_retry_on_5xx(false);
    real_fetcher_->set_request_context(ProfileImplAndroid::GetDefaultRequestContext());
    real_fetcher_->set_upload_data("text/plain", upload_content_);
    real_fetcher_->Start();
  };

  static void DoComplete(void* context)
  {
    URLFetcherProxy* that = (URLFetcherProxy*)context;
    that->d_->OnURLFetchComplete(that, that->url_, that->status_,
                                 that->response_code_, that->cookies_,
                                 that->data_);
  }

  URLFetcher* real_fetcher_;
  GURL url_;
  Delegate* d_;
  std::string upload_content_;

  URLRequestStatus status_;
  int response_code_;
  ResponseCookies cookies_;
  std::string data_;

  DISALLOW_EVIL_CONSTRUCTORS(URLFetcherProxy);
};

#endif  // URL_FETCHER_PROXY_H_
