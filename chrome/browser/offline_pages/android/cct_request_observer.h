// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_ANDROID_CCT_REQUEST_OBSERVER_H_
#define CHROME_BROWSER_OFFLINE_PAGES_ANDROID_CCT_REQUEST_OBSERVER_H_

#include <stdint.h>

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/macros.h"
#include "base/supports_user_data.h"
#include "components/offline_pages/core/downloads/download_ui_adapter.h"

namespace offline_pages {

/**
 * Bridge between C++ and Java for exposing when CCT offlining requests
 * complete.
 */
class CCTRequestObserver : public RequestCoordinator::Observer,
                           public base::SupportsUserData::Data {
 public:
  static void AttachToRequestCoordinator(RequestCoordinator* coordinator);
  ~CCTRequestObserver() override;

  // RequestCoordinator::Observer implementation.
  void OnAdded(const SavePageRequest& request) override;
  void OnCompleted(const SavePageRequest& request,
                   RequestNotifier::BackgroundSavePageResult status) override;
  void OnChanged(const SavePageRequest& request) override;
  void OnNetworkProgress(const SavePageRequest& request,
                         int64_t received_bytes) override;

 private:
  explicit CCTRequestObserver(
      base::android::ScopedJavaLocalRef<jobject> callback);

  base::android::ScopedJavaGlobalRef<jobject> j_callback_;

  DISALLOW_COPY_AND_ASSIGN(CCTRequestObserver);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_ANDROID_CCT_REQUEST_OBSERVER_H_
