// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/logo_bridge.h"

#include <jni.h>
#include <stdint.h>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/search_provider_logos/logo_service_factory.h"
#include "components/search_provider_logos/logo_service.h"
#include "components/search_provider_logos/logo_tracker.h"
#include "jni/LogoBridge_jni.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/android/java_bitmap.h"
#include "url/gurl.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaByteArray;

namespace {

ScopedJavaLocalRef<jobject> JNI_LogoBridge_MakeJavaLogo(
    JNIEnv* env,
    const SkBitmap* bitmap,
    const GURL& on_click_url,
    const std::string& alt_text,
    const GURL& animated_url) {
  ScopedJavaLocalRef<jobject> j_bitmap = gfx::ConvertToJavaBitmap(bitmap);

  ScopedJavaLocalRef<jstring> j_on_click_url;
  if (on_click_url.is_valid())
    j_on_click_url = ConvertUTF8ToJavaString(env, on_click_url.spec());

  ScopedJavaLocalRef<jstring> j_alt_text;
  if (!alt_text.empty())
    j_alt_text = ConvertUTF8ToJavaString(env, alt_text);

  ScopedJavaLocalRef<jstring> j_animated_url;
  if (animated_url.is_valid())
    j_animated_url = ConvertUTF8ToJavaString(env, animated_url.spec());

  return Java_LogoBridge_createLogo(env, j_bitmap, j_on_click_url, j_alt_text,
                                    j_animated_url);
}

// Converts a C++ Logo to a Java Logo.
ScopedJavaLocalRef<jobject> JNI_LogoBridge_ConvertLogoToJavaObject(
    JNIEnv* env,
    const search_provider_logos::Logo* logo) {
  if (!logo)
    return ScopedJavaLocalRef<jobject>();

  return JNI_LogoBridge_MakeJavaLogo(
      env, &logo->image, GURL(logo->metadata.on_click_url),
      logo->metadata.alt_text, GURL(logo->metadata.animated_url));
}

class LogoObserverAndroid : public search_provider_logos::LogoObserver {
 public:
  LogoObserverAndroid(base::WeakPtr<LogoBridge> logo_bridge,
                      JNIEnv* env,
                      jobject j_logo_observer)
      : logo_bridge_(logo_bridge) {
    j_logo_observer_.Reset(env, j_logo_observer);
  }

  ~LogoObserverAndroid() override {}

  // seach_provider_logos::LogoObserver:
  void OnLogoAvailable(const search_provider_logos::Logo* logo,
                       bool from_cache) override {
    if (!logo_bridge_)
      return;

    JNIEnv* env = base::android::AttachCurrentThread();
    ScopedJavaLocalRef<jobject> j_logo =
        JNI_LogoBridge_ConvertLogoToJavaObject(env, logo);
    Java_LogoObserver_onLogoAvailable(env, j_logo_observer_, j_logo,
                                      from_cache);
  }

  void OnObserverRemoved() override { delete this; }

 private:
  // The associated LogoBridge. We won't call back to Java if the LogoBridge has
  // been destroyed.
  base::WeakPtr<LogoBridge> logo_bridge_;

  base::android::ScopedJavaGlobalRef<jobject> j_logo_observer_;

  DISALLOW_COPY_AND_ASSIGN(LogoObserverAndroid);
};

}  // namespace

class LogoBridge::AnimatedLogoFetcher : public net::URLFetcherDelegate {
 public:
  AnimatedLogoFetcher(
      const scoped_refptr<net::URLRequestContextGetter>& request_context)
      : request_context_(request_context) {}

  ~AnimatedLogoFetcher() override {}

  void Start(JNIEnv* env,
             const GURL& url,
             const JavaParamRef<jobject>& j_callback) {
    DCHECK(j_callback);

    if (fetcher_ && fetcher_->GetOriginalURL() == url)
      return;

    j_callback_.Reset(env, j_callback);
    fetcher_ = net::URLFetcher::Create(url, net::URLFetcher::GET, this);
    fetcher_->SetRequestContext(request_context_.get());
    fetcher_->Start();
    start_time_ = base::TimeTicks::Now();
  }

 private:
  void OnURLFetchComplete(const net::URLFetcher* source) override {
    DCHECK_EQ(source, fetcher_.get());
    DCHECK(!j_callback_.is_null());
    if (!source->GetStatus().is_success() ||
        (source->GetResponseCode() != 200)) {
      ClearFetcher();
      return;
    }

    UMA_HISTOGRAM_TIMES("NewTabPage.AnimatedLogoDownloadTime",
                        base::TimeTicks::Now() - start_time_);

    std::string response;
    source->GetResponseAsString(&response);
    JNIEnv* env = base::android::AttachCurrentThread();

    ScopedJavaLocalRef<jbyteArray> j_bytes =
        ToJavaByteArray(env, reinterpret_cast<const uint8_t*>(response.data()),
                        response.length());
    ScopedJavaLocalRef<jobject> j_gif_image =
        Java_LogoBridge_createGifImage(env, j_bytes);
    Java_AnimatedLogoCallback_onAnimatedLogoAvailable(env, j_callback_,
                                                      j_gif_image);
    ClearFetcher();
  }

  void ClearFetcher() {
    fetcher_.reset();
    j_callback_.Reset();
  }

  scoped_refptr<net::URLRequestContextGetter> request_context_;

  base::android::ScopedJavaGlobalRef<jobject> j_callback_;

  // The URLFetcher currently fetching the animated logo, or nullptr when not
  // fetching.
  std::unique_ptr<net::URLFetcher> fetcher_;

  // The time when the current fetch was started.
  base::TimeTicks start_time_;

  DISALLOW_COPY_AND_ASSIGN(AnimatedLogoFetcher);
};

static jlong JNI_LogoBridge_Init(JNIEnv* env,
                                 const JavaParamRef<jobject>& obj,
                                 const JavaParamRef<jobject>& j_profile) {
  LogoBridge* logo_bridge = new LogoBridge(j_profile);
  return reinterpret_cast<intptr_t>(logo_bridge);
}

LogoBridge::LogoBridge(const JavaRef<jobject>& j_profile)
    : logo_service_(nullptr), weak_ptr_factory_(this) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  DCHECK(profile);

  logo_service_ = LogoServiceFactory::GetForProfile(profile);

  animated_logo_fetcher_ =
      std::make_unique<AnimatedLogoFetcher>(profile->GetRequestContext());
}

LogoBridge::~LogoBridge() {}

void LogoBridge::Destroy(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  delete this;
}

void LogoBridge::GetCurrentLogo(JNIEnv* env,
                                const JavaParamRef<jobject>& obj,
                                const JavaParamRef<jobject>& j_logo_observer) {
  // |observer| is deleted in LogoObserverAndroid::OnObserverRemoved().
  LogoObserverAndroid* observer = new LogoObserverAndroid(
      weak_ptr_factory_.GetWeakPtr(), env, j_logo_observer);
  logo_service_->GetLogo(observer);
}

void LogoBridge::GetAnimatedLogo(JNIEnv* env,
                                 const JavaParamRef<jobject>& obj,
                                 const JavaParamRef<jobject>& j_callback,
                                 const JavaParamRef<jstring>& j_url) {
  GURL url = GURL(ConvertJavaStringToUTF8(env, j_url));
  animated_logo_fetcher_->Start(env, url, j_callback);
}
