// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/android/payment_manifest_downloader_android.h"

#include <memory>

#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "jni/PaymentManifestDownloader_jni.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

namespace payments {
namespace {

class DownloadCallback {
 public:
  explicit DownloadCallback(
      const base::android::JavaParamRef<jobject>& jcallback)
      : jcallback_(jcallback) {}

  ~DownloadCallback() {}

  void OnPaymentMethodManifestDownload(const std::string& content) {
    JNIEnv* env = base::android::AttachCurrentThread();

    if (content.empty()) {
      Java_ManifestDownloadCallback_onManifestDownloadFailure(env, jcallback_);
    } else {
      Java_ManifestDownloadCallback_onPaymentMethodManifestDownloadSuccess(
          env, jcallback_,
          base::android::ConvertUTF8ToJavaString(env, content));
    }
  }

  void OnWebAppManifestDownload(const std::string& content) {
    JNIEnv* env = base::android::AttachCurrentThread();

    if (content.empty()) {
      Java_ManifestDownloadCallback_onManifestDownloadFailure(env, jcallback_);
    } else {
      Java_ManifestDownloadCallback_onWebAppManifestDownloadSuccess(
          env, jcallback_,
          base::android::ConvertUTF8ToJavaString(env, content));
    }
  }

 private:
  base::android::ScopedJavaGlobalRef<jobject> jcallback_;

  DISALLOW_COPY_AND_ASSIGN(DownloadCallback);
};

}  // namespace

PaymentManifestDownloaderAndroid::PaymentManifestDownloaderAndroid(
    const scoped_refptr<net::URLRequestContextGetter>& context)
    : downloader_(context) {}

PaymentManifestDownloaderAndroid::~PaymentManifestDownloaderAndroid() {}

void PaymentManifestDownloaderAndroid::DownloadPaymentMethodManifest(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jobject>& juri,
    const base::android::JavaParamRef<jobject>& jcallback) {
  downloader_.DownloadPaymentMethodManifest(
      GURL(base::android::ConvertJavaStringToUTF8(
          env, Java_PaymentManifestDownloader_getUriString(env, juri))),
      base::BindOnce(&DownloadCallback::OnPaymentMethodManifestDownload,
                     std::make_unique<DownloadCallback>(jcallback)));
}

void PaymentManifestDownloaderAndroid::DownloadWebAppManifest(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jobject>& juri,
    const base::android::JavaParamRef<jobject>& jcallback) {
  downloader_.DownloadWebAppManifest(
      GURL(base::android::ConvertJavaStringToUTF8(
          env, Java_PaymentManifestDownloader_getUriString(env, juri))),
      base::BindOnce(&DownloadCallback::OnWebAppManifestDownload,
                     std::make_unique<DownloadCallback>(jcallback)));
}

void PaymentManifestDownloaderAndroid::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  delete this;
}

// Static free function declared and called directly from java.
// Caller owns the result. Returns 0 on error.
static jlong JNI_PaymentManifestDownloader_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  if (!web_contents)
    return 0;

  return reinterpret_cast<jlong>(new PaymentManifestDownloaderAndroid(
      content::BrowserContext::GetDefaultStoragePartition(
          web_contents->GetBrowserContext())
          ->GetURLRequestContext()));
}

}  // namespace payments
