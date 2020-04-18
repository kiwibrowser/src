// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_NET_ANDROID_STREAM_READER_URL_REQUEST_JOB_H_
#define ANDROID_WEBVIEW_BROWSER_NET_ANDROID_STREAM_READER_URL_REQUEST_JOB_H_

#include <memory>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "net/base/net_errors.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request_job.h"

namespace net {
class HttpResponseHeaders;
class HttpResponseInfo;
class URLRequest;
}

namespace android_webview {

class InputStream;
class InputStreamReader;
class InputStreamReaderWrapper;

// A request job that reads data from a Java InputStream.
class AndroidStreamReaderURLRequestJob : public net::URLRequestJob {
 public:
  /*
   * We use a delegate so that we can share code for this job in slightly
   * different contexts.
   */
  class Delegate {
   public:
    virtual ~Delegate() {}

    // This method is called from a worker thread, not from the IO thread.
    virtual std::unique_ptr<android_webview::InputStream> OpenInputStream(
        JNIEnv* env,
        const GURL& url) = 0;

    // This method is called on the Job's thread if the result of calling
    // OpenInputStream was null.
    // Setting the |restart| parameter to true will cause the request to be
    // restarted with a new job.
    virtual void OnInputStreamOpenFailed(
        net::URLRequest* request,
        bool* restart) = 0;

    virtual bool GetMimeType(
        JNIEnv* env,
        net::URLRequest* request,
        android_webview::InputStream* stream,
        std::string* mime_type) = 0;

    virtual bool GetCharset(
        JNIEnv* env,
        net::URLRequest* request,
        android_webview::InputStream* stream,
        std::string* charset) = 0;

    virtual void AppendResponseHeaders(JNIEnv* env,
                                       net::HttpResponseHeaders* headers) = 0;
  };

  class DelegateObtainer {
   public:
    virtual ~DelegateObtainer() {}

    using Callback = base::OnceCallback<void(std::unique_ptr<Delegate>)>;
    virtual void ObtainDelegate(net::URLRequest* request,
                                Callback callback) = 0;
  };

  AndroidStreamReaderURLRequestJob(net::URLRequest* request,
                                   net::NetworkDelegate* network_delegate,
                                   std::unique_ptr<Delegate> delegate);
  AndroidStreamReaderURLRequestJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      std::unique_ptr<DelegateObtainer> delegate_obtainer,
      bool);  // resolve ambiguity

  // URLRequestJob:
  void Start() override;
  void Kill() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  void SetExtraRequestHeaders(const net::HttpRequestHeaders& headers) override;
  bool GetMimeType(std::string* mime_type) const override;
  bool GetCharset(std::string* charset) override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;

 protected:
  ~AndroidStreamReaderURLRequestJob() override;

  // Creates an InputStreamReader instance.
  // Overridden in unittests to return a mock.
  virtual std::unique_ptr<android_webview::InputStreamReader>
  CreateStreamReader(android_webview::InputStream* stream);

 private:
  // Used as a callback when obtaining the delegate asynchronously,
  // see DelegateObtainer.
  void DelegateObtained(std::unique_ptr<Delegate> delegate);
  // Actual URLRequestJob::Start implementation.
  void DoStart();

  void HeadersComplete(int status_code, const std::string& status_text);

  void OnInputStreamOpened(
      std::unique_ptr<Delegate> delegate,
      std::unique_ptr<android_webview::InputStream> input_stream);
  void OnReaderSeekCompleted(int content_size);
  void OnReaderReadCompleted(int bytes_read);

  net::HttpByteRange byte_range_;
  net::Error range_parse_result_;
  std::unique_ptr<net::HttpResponseInfo> response_info_;
  std::unique_ptr<Delegate> delegate_;
  std::unique_ptr<DelegateObtainer> delegate_obtainer_;
  scoped_refptr<InputStreamReaderWrapper> input_stream_reader_wrapper_;
  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<AndroidStreamReaderURLRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AndroidStreamReaderURLRequestJob);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_NET_ANDROID_STREAM_READER_URL_REQUEST_JOB_H_
