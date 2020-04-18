// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/android_protocol_handler.h"

#include <memory>
#include <utility>

#include "android_webview/browser/input_stream.h"
#include "android_webview/browser/net/android_stream_reader_url_request_job.h"
#include "android_webview/browser/net/aw_url_request_job_factory.h"
#include "android_webview/common/url_constants.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "content/public/common/url_constants.h"
#include "jni/AndroidProtocolHandler_jni.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_interceptor.h"
#include "url/gurl.h"
#include "url/url_constants.h"

using android_webview::AndroidStreamReaderURLRequestJob;
using android_webview::InputStream;
using android_webview::InputStream;
using base::android::AttachCurrentThread;
using base::android::ClearException;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace {

const void* const kPreviouslyFailedKey = &kPreviouslyFailedKey;

void MarkRequestAsFailed(net::URLRequest* request) {
  request->SetUserData(kPreviouslyFailedKey,
                       std::make_unique<base::SupportsUserData::Data>());
}

bool HasRequestPreviouslyFailed(net::URLRequest* request) {
  return request->GetUserData(kPreviouslyFailedKey) != NULL;
}

class AndroidStreamReaderURLRequestJobDelegateImpl
    : public AndroidStreamReaderURLRequestJob::Delegate {
 public:
  AndroidStreamReaderURLRequestJobDelegateImpl();

  std::unique_ptr<InputStream> OpenInputStream(JNIEnv* env,
                                               const GURL& url) override;

  void OnInputStreamOpenFailed(net::URLRequest* request,
                               bool* restart) override;

  bool GetMimeType(JNIEnv* env,
                   net::URLRequest* request,
                   InputStream* stream,
                   std::string* mime_type) override;

  bool GetCharset(JNIEnv* env,
                  net::URLRequest* request,
                  InputStream* stream,
                  std::string* charset) override;

  void AppendResponseHeaders(JNIEnv* env,
                             net::HttpResponseHeaders* headers) override;

  ~AndroidStreamReaderURLRequestJobDelegateImpl() override;
};

class AndroidRequestInterceptorBase : public net::URLRequestInterceptor {
 public:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  virtual bool ShouldHandleRequest(const net::URLRequest* request) const = 0;
};

class AssetFileRequestInterceptor : public AndroidRequestInterceptorBase {
 public:
  AssetFileRequestInterceptor();
  bool ShouldHandleRequest(const net::URLRequest* request) const override;
};

// Protocol handler for content:// scheme requests.
class ContentSchemeRequestInterceptor : public AndroidRequestInterceptorBase {
 public:
  ContentSchemeRequestInterceptor();
  bool ShouldHandleRequest(const net::URLRequest* request) const override;
};

// AndroidStreamReaderURLRequestJobDelegateImpl -------------------------------

AndroidStreamReaderURLRequestJobDelegateImpl::
    AndroidStreamReaderURLRequestJobDelegateImpl() {}

AndroidStreamReaderURLRequestJobDelegateImpl::
    ~AndroidStreamReaderURLRequestJobDelegateImpl() {}

std::unique_ptr<InputStream>
AndroidStreamReaderURLRequestJobDelegateImpl::OpenInputStream(JNIEnv* env,
                                                              const GURL& url) {
  DCHECK(url.is_valid());
  DCHECK(env);

  // Open the input stream.
  ScopedJavaLocalRef<jstring> jurl = ConvertUTF8ToJavaString(env, url.spec());
  ScopedJavaLocalRef<jobject> stream =
      android_webview::Java_AndroidProtocolHandler_open(env, jurl);

  if (stream.is_null()) {
    DLOG(ERROR) << "Unable to open input stream for Android URL";
    return std::unique_ptr<InputStream>();
  }
  return std::make_unique<InputStream>(stream);
}

void AndroidStreamReaderURLRequestJobDelegateImpl::OnInputStreamOpenFailed(
    net::URLRequest* request,
    bool* restart) {
  DCHECK(!HasRequestPreviouslyFailed(request));
  MarkRequestAsFailed(request);
  *restart = true;
}

bool AndroidStreamReaderURLRequestJobDelegateImpl::GetMimeType(
    JNIEnv* env,
    net::URLRequest* request,
    android_webview::InputStream* stream,
    std::string* mime_type) {
  DCHECK(env);
  DCHECK(request);
  DCHECK(mime_type);

  // Query the mime type from the Java side. It is possible for the query to
  // fail, as the mime type cannot be determined for all supported schemes.
  ScopedJavaLocalRef<jstring> url =
      ConvertUTF8ToJavaString(env, request->url().spec());
  ScopedJavaLocalRef<jstring> returned_type =
      android_webview::Java_AndroidProtocolHandler_getMimeType(
          env, stream->jobj(), url);
  if (returned_type.is_null())
    return false;

  *mime_type = base::android::ConvertJavaStringToUTF8(returned_type);
  return true;
}

bool AndroidStreamReaderURLRequestJobDelegateImpl::GetCharset(
    JNIEnv* env,
    net::URLRequest* request,
    android_webview::InputStream* stream,
    std::string* charset) {
  // TODO: We should probably be getting this from the managed side.
  return false;
}

void AndroidStreamReaderURLRequestJobDelegateImpl::AppendResponseHeaders(
    JNIEnv* env,
    net::HttpResponseHeaders* headers) {
  // no-op
}

// AndroidRequestInterceptorBase ----------------------------------------------

net::URLRequestJob* AndroidRequestInterceptorBase::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  if (!ShouldHandleRequest(request))
    return NULL;

  // For WebViewClassic compatibility this job can only accept URLs that can be
  // opened. URLs that cannot be opened should be resolved by the next handler.
  //
  // If a request is initially handled here but the job fails due to it being
  // unable to open the InputStream for that request the request is marked as
  // previously failed and restarted.
  // Restarting a request involves creating a new job for that request. This
  // handler will ignore requests know to have previously failed to 1) prevent
  // an infinite loop, 2) ensure that the next handler in line gets the
  // opportunity to create a job for the request.
  if (HasRequestPreviouslyFailed(request))
    return NULL;

  std::unique_ptr<AndroidStreamReaderURLRequestJobDelegateImpl> reader_delegate(
      new AndroidStreamReaderURLRequestJobDelegateImpl());

  return new AndroidStreamReaderURLRequestJob(request, network_delegate,
                                              std::move(reader_delegate));
}

// AssetFileRequestInterceptor ------------------------------------------------

AssetFileRequestInterceptor::AssetFileRequestInterceptor() {}

bool AssetFileRequestInterceptor::ShouldHandleRequest(
    const net::URLRequest* request) const {
  return android_webview::IsAndroidSpecialFileUrl(request->url());
}

// ContentSchemeRequestInterceptor --------------------------------------------

ContentSchemeRequestInterceptor::ContentSchemeRequestInterceptor() {}

bool ContentSchemeRequestInterceptor::ShouldHandleRequest(
    const net::URLRequest* request) const {
  return request->url().SchemeIs(url::kContentScheme);
}

}  // namespace

namespace android_webview {

// static
std::unique_ptr<net::URLRequestInterceptor>
CreateContentSchemeRequestInterceptor() {
  return std::make_unique<ContentSchemeRequestInterceptor>();
}

// static
std::unique_ptr<net::URLRequestInterceptor>
CreateAssetFileRequestInterceptor() {
  return std::unique_ptr<net::URLRequestInterceptor>(
      new AssetFileRequestInterceptor());
}

static ScopedJavaLocalRef<jstring>
JNI_AndroidProtocolHandler_GetAndroidAssetPath(
    JNIEnv* env,
    const JavaParamRef<jclass>& /*clazz*/) {
  return ConvertUTF8ToJavaString(env, android_webview::kAndroidAssetPath);
}

static ScopedJavaLocalRef<jstring>
JNI_AndroidProtocolHandler_GetAndroidResourcePath(
    JNIEnv* env,
    const JavaParamRef<jclass>& /*clazz*/) {
  return ConvertUTF8ToJavaString(env, android_webview::kAndroidResourcePath);
}

}  // namespace android_webview
