// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "jni/ConnectivityChecker_jni.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

using base::android::JavaParamRef;

namespace chrome {
namespace android {

namespace {
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.feedback
// GENERATED_JAVA_PREFIX_TO_STRIP: CONNECTIVITY_CHECK_RESULT_
enum ConnectivityCheckResult {
  CONNECTIVITY_CHECK_RESULT_UNKNOWN = 0,
  CONNECTIVITY_CHECK_RESULT_CONNECTED = 1,
  CONNECTIVITY_CHECK_RESULT_NOT_CONNECTED = 2,
  CONNECTIVITY_CHECK_RESULT_TIMEOUT = 3,
  CONNECTIVITY_CHECK_RESULT_ERROR = 4,
  CONNECTIVITY_CHECK_RESULT_END = 5
};

void ExecuteCallback(const base::android::JavaRef<jobject>& callback,
                     ConnectivityCheckResult result) {
  CHECK(result >= CONNECTIVITY_CHECK_RESULT_UNKNOWN);
  CHECK(result < CONNECTIVITY_CHECK_RESULT_END);
  Java_ConnectivityChecker_executeCallback(base::android::AttachCurrentThread(),
                                           callback, result);
}

void JNI_ConnectivityChecker_PostCallback(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& j_callback,
    ConnectivityCheckResult result) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&ExecuteCallback,
                 base::android::ScopedJavaGlobalRef<jobject>(j_callback),
                 result));
}

// A utility class for checking if the device is currently connected to the
// Internet.
class ConnectivityChecker : public net::URLFetcherDelegate {
 public:
  ConnectivityChecker(Profile* profile,
                      const GURL& url,
                      const base::TimeDelta& timeout,
                      const base::android::JavaRef<jobject>& java_callback);

  // Kicks off the asynchronous connectivity check. When the request has
  // completed, |this| is deleted.
  void StartAsyncCheck();

  // net::URLFetcherDelegate implementation:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Cancels the URLFetcher, and triggers the callback with a negative result
  // and the timeout flag set.
  void OnTimeout();

 private:
  // The context in which the connectivity check is performed.
  net::URLRequestContextGetter* request_context_;

  // The URL to connect to.
  const GURL& url_;

  // How long to wait for a response before giving up.
  const base::TimeDelta timeout_;

  // Holds the Java object which will get the callback with the result.
  base::android::ScopedJavaGlobalRef<jobject> java_callback_;

  // The URLFetcher that executes the connectivity check.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // Whether |this| is already being destroyed, at which point the callback
  // has already happened, and no further action should be taken.
  bool is_being_destroyed_;

  std::unique_ptr<base::OneShotTimer> expiration_timer_;
};

void ConnectivityChecker::OnURLFetchComplete(const net::URLFetcher* source) {
  if (is_being_destroyed_)
    return;
  is_being_destroyed_ = true;

  DCHECK_EQ(url_fetcher_.get(), source);
  net::URLRequestStatus status = source->GetStatus();
  int response_code = source->GetResponseCode();

  bool connected = status.is_success() && response_code == net::HTTP_NO_CONTENT;
  if (connected) {
    ExecuteCallback(java_callback_, CONNECTIVITY_CHECK_RESULT_CONNECTED);
  } else {
    ExecuteCallback(java_callback_, CONNECTIVITY_CHECK_RESULT_NOT_CONNECTED);
  }

  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

ConnectivityChecker::ConnectivityChecker(
    Profile* profile,
    const GURL& url,
    const base::TimeDelta& timeout,
    const base::android::JavaRef<jobject>& java_callback)
    : request_context_(profile->GetRequestContext()),
      url_(url),
      timeout_(timeout),
      java_callback_(java_callback),
      is_being_destroyed_(false) {
}

void ConnectivityChecker::StartAsyncCheck() {
  url_fetcher_ = net::URLFetcher::Create(url_, net::URLFetcher::GET, this);
  data_use_measurement::DataUseUserData::AttachToFetcher(
      url_fetcher_.get(),
      data_use_measurement::DataUseUserData::FEEDBACK_UPLOADER);
  url_fetcher_->SetRequestContext(request_context_);
  url_fetcher_->SetStopOnRedirect(true);
  url_fetcher_->SetAutomaticallyRetryOn5xx(false);
  url_fetcher_->SetAutomaticallyRetryOnNetworkChanges(0);
  url_fetcher_->SetLoadFlags(net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                             net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SEND_AUTH_DATA);
  url_fetcher_->Start();
  expiration_timer_.reset(new base::OneShotTimer());
  expiration_timer_->Start(FROM_HERE, timeout_, this,
                           &ConnectivityChecker::OnTimeout);
}

void ConnectivityChecker::OnTimeout() {
  if (is_being_destroyed_)
    return;
  is_being_destroyed_ = true;
  url_fetcher_.reset();
  ExecuteCallback(java_callback_, CONNECTIVITY_CHECK_RESULT_TIMEOUT);
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

}  // namespace

void JNI_ConnectivityChecker_CheckConnectivity(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jobject>& j_profile,
    const JavaParamRef<jstring>& j_url,
    jlong j_timeout_ms,
    const JavaParamRef<jobject>& j_callback) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  if (!profile) {
    JNI_ConnectivityChecker_PostCallback(env, j_callback,
                                         CONNECTIVITY_CHECK_RESULT_ERROR);
    return;
  }
  GURL url(base::android::ConvertJavaStringToUTF8(env, j_url));
  if (!url.is_valid()) {
    JNI_ConnectivityChecker_PostCallback(env, j_callback,
                                         CONNECTIVITY_CHECK_RESULT_ERROR);
    return;
  }

  // This object will be deleted when the connectivity check has completed.
  ConnectivityChecker* connectivity_checker = new ConnectivityChecker(
      profile, url, base::TimeDelta::FromMilliseconds(j_timeout_ms),
      j_callback);
  connectivity_checker->StartAsyncCheck();
}

jboolean JNI_ConnectivityChecker_IsUrlValid(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& j_url) {
  GURL url(base::android::ConvertJavaStringToUTF8(env, j_url));
  return url.is_valid();
}

}  // namespace android
}  // namespace chrome
