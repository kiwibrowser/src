// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/aw_request_interceptor.h"

#include <memory>
#include <utility>

#include "android_webview/browser/aw_contents_io_thread_client.h"
#include "android_webview/browser/input_stream.h"
#include "android_webview/browser/net/android_stream_reader_url_request_job.h"
#include "android_webview/browser/net/aw_web_resource_response.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/supports_user_data.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_job.h"
#include "url/url_constants.h"

namespace android_webview {

namespace {

const void* const kRequestAlreadyHasJobDataKey = &kRequestAlreadyHasJobDataKey;

class StreamReaderJobDelegateImpl
    : public AndroidStreamReaderURLRequestJob::Delegate {
 public:
  StreamReaderJobDelegateImpl(
      std::unique_ptr<AwWebResourceResponse> aw_web_resource_response)
      : aw_web_resource_response_(std::move(aw_web_resource_response)) {
    DCHECK(aw_web_resource_response_);
  }

  std::unique_ptr<InputStream> OpenInputStream(JNIEnv* env,
                                               const GURL& url) override {
    return aw_web_resource_response_->GetInputStream(env);
  }

  void OnInputStreamOpenFailed(net::URLRequest* request,
                               bool* restart) override {
    *restart = false;
  }

  bool GetMimeType(JNIEnv* env,
                   net::URLRequest* request,
                   android_webview::InputStream* stream,
                   std::string* mime_type) override {
    return aw_web_resource_response_->GetMimeType(env, mime_type);
  }

  bool GetCharset(JNIEnv* env,
                  net::URLRequest* request,
                  android_webview::InputStream* stream,
                  std::string* charset) override {
    return aw_web_resource_response_->GetCharset(env, charset);
  }

  void AppendResponseHeaders(JNIEnv* env,
                             net::HttpResponseHeaders* headers) override {
    int status_code;
    std::string reason_phrase;
    if (aw_web_resource_response_->GetStatusInfo(
            env, &status_code, &reason_phrase)) {
      std::string status_line("HTTP/1.1 ");
      status_line.append(base::IntToString(status_code));
      status_line.append(" ");
      status_line.append(reason_phrase);
      headers->ReplaceStatusLine(status_line);
    }
    aw_web_resource_response_->GetResponseHeaders(env, headers);
  }

 private:
  std::unique_ptr<AwWebResourceResponse> aw_web_resource_response_;
};

class ShouldInterceptRequestAdaptor
    : public AndroidStreamReaderURLRequestJob::DelegateObtainer {
 public:
  explicit ShouldInterceptRequestAdaptor(
      std::unique_ptr<AwContentsIoThreadClient> io_thread_client)
      : io_thread_client_(std::move(io_thread_client)), weak_factory_(this) {}
  ~ShouldInterceptRequestAdaptor() override {}

  void ObtainDelegate(net::URLRequest* request, Callback callback) override {
    callback_ = std::move(callback);
    io_thread_client_->ShouldInterceptRequestAsync(
        // The request is only used while preparing the call, not retained.
        request,
        base::BindOnce(
            &ShouldInterceptRequestAdaptor::WebResourceResponseObtained,
            // The lifetime of the DelegateObtainer is managed by
            // AndroidStreamReaderURLRequestJob, it might get deleted.
            weak_factory_.GetWeakPtr()));
  }

 private:
  void WebResourceResponseObtained(
      std::unique_ptr<AwWebResourceResponse> response) {
    if (response) {
      std::move(callback_).Run(
          std::make_unique<StreamReaderJobDelegateImpl>(std::move(response)));
    } else {
      std::move(callback_).Run(nullptr);
    }
  }

  std::unique_ptr<AwContentsIoThreadClient> io_thread_client_;
  Callback callback_;
  base::WeakPtrFactory<ShouldInterceptRequestAdaptor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShouldInterceptRequestAdaptor);
};

std::unique_ptr<AwContentsIoThreadClient> GetCorrespondingIoThreadClient(
    net::URLRequest* request) {
  if (content::ResourceRequestInfo::OriginatedFromServiceWorker(request))
    return AwContentsIoThreadClient::GetServiceWorkerIoThreadClient();
  int render_process_id, render_frame_id;
  if (!content::ResourceRequestInfo::GetRenderFrameForRequest(
      request, &render_process_id, &render_frame_id)) {
    return nullptr;
  }

  if (render_process_id == -1 || render_frame_id == -1) {
    const content::ResourceRequestInfo* resourceRequestInfo =
        content::ResourceRequestInfo::ForRequest(request);
    if (resourceRequestInfo == nullptr) {
      return nullptr;
    }
    return AwContentsIoThreadClient::FromID(
        resourceRequestInfo->GetFrameTreeNodeId());
  }

  return AwContentsIoThreadClient::FromID(render_process_id, render_frame_id);
}

}  // namespace

AwRequestInterceptor::AwRequestInterceptor() {}

AwRequestInterceptor::~AwRequestInterceptor() {}

net::URLRequestJob* AwRequestInterceptor::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // MaybeInterceptRequest can be called multiple times for the same request.
  if (request->GetUserData(kRequestAlreadyHasJobDataKey))
    return nullptr;

  // It's not useful to emit shouldInterceptRequest for blob URLs, so we
  // intentionally skip the callback for all such URLs. See
  // http://crbug.com/822983.
  if (request->url().SchemeIs(url::kBlobScheme)) {
    return nullptr;
  }

  std::unique_ptr<AwContentsIoThreadClient> io_thread_client =
      GetCorrespondingIoThreadClient(request);

  if (!io_thread_client)
    return nullptr;

  GURL referrer(request->referrer());
  if (referrer.is_valid() &&
      (!request->is_pending() || request->is_redirecting())) {
    request->SetExtraRequestHeaderByName(net::HttpRequestHeaders::kReferer,
                                         referrer.spec(), true);
  }
  request->SetUserData(kRequestAlreadyHasJobDataKey,
                       std::make_unique<base::SupportsUserData::Data>());
  return new AndroidStreamReaderURLRequestJob(
      request, network_delegate,
      std::make_unique<ShouldInterceptRequestAdaptor>(
          std::move(io_thread_client)),
      true);
}

}  // namespace android_webview
