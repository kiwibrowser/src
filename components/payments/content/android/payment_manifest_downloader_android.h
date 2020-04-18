// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_ANDROID_PAYMENT_MANIFEST_DOWNLOADER_ANDROID_H_
#define COMPONENTS_PAYMENTS_CONTENT_ANDROID_PAYMENT_MANIFEST_DOWNLOADER_ANDROID_H_

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/payments/core/payment_manifest_downloader.h"

namespace net {
class URLRequestContextGetter;
}

namespace payments {

// Android wrapper for the payment manifest downloader.
class PaymentManifestDownloaderAndroid {
 public:
  explicit PaymentManifestDownloaderAndroid(
      const scoped_refptr<net::URLRequestContextGetter>& context);
  ~PaymentManifestDownloaderAndroid();

  void DownloadPaymentMethodManifest(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller,
      const base::android::JavaParamRef<jobject>& juri,
      const base::android::JavaParamRef<jobject>& jcallback);

  void DownloadWebAppManifest(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller,
      const base::android::JavaParamRef<jobject>& juri,
      const base::android::JavaParamRef<jobject>& jcallback);

  // Deletes this object.
  void Destroy(JNIEnv* env,
               const base::android::JavaParamRef<jobject>& jcaller);

 private:
  PaymentManifestDownloader downloader_;

  DISALLOW_COPY_AND_ASSIGN(PaymentManifestDownloaderAndroid);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_ANDROID_PAYMENT_MANIFEST_DOWNLOADER_ANDROID_H_
