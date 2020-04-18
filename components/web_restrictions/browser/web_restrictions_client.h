// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEB_RESTRICTION_WEB_RESTRICTIONS_CLIENT_H_
#define COMPONENTS_WEB_RESTRICTION_WEB_RESTRICTIONS_CLIENT_H_

#include <jni.h>

#include <list>
#include <string>
#include <unordered_map>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "components/web_restrictions/browser/web_restrictions_client_result.h"

namespace web_restrictions {

enum UrlAccess { ALLOW, DISALLOW, PENDING };

class WebRestrictionsClient {
 public:
  // An instance of this class is expected to live through the lifetime of a
  // browser and uses raw pointers in callbacks.
  // Any changes to the class, enable/disable/change should be done through the
  // SetAuthority(...) method.
  WebRestrictionsClient();
  ~WebRestrictionsClient();

  // Verify the content provider and query it for basic information like does it
  // support handling requests. This should be called everytime the provider
  // changes. An empty authority can be used to disable this class.
  void SetAuthority(const std::string& content_provider_authority);

  // WebRestrictionsProvider:
  UrlAccess ShouldProceed(bool is_main_frame,
                          const std::string& url,
                          const base::Callback<void(bool)>& callback);

  bool SupportsRequest() const;

  void RequestPermission(const std::string& url,
                         const base::Callback<void(bool)>& callback);

  void OnWebRestrictionsChanged(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  // Get a cached WebRestrictionsResult synchronously, for use when building
  // error pages etc.. May be called on any thread, and will return a fresh copy
  // of the result (hence thread safe).
  std::unique_ptr<WebRestrictionsClientResult> GetCachedWebRestrictionsResult(
      const std::string& url);

 private:
  // Make the test classes friends, so that they can set the authority
  // synchronously
  friend class WebRestrictionsResourceThrottleTest;
  friend class WebRestrictionsClientTest;

  class Cache {
   public:
    Cache();
    ~Cache();
    std::unique_ptr<WebRestrictionsClientResult> GetCacheEntry(
        const std::string& url);
    void SetCacheEntry(const std::string& url,
                       const WebRestrictionsClientResult& entry);
    void RemoveCacheEntry(const std::string& url);
    void Clear();

   private:
    base::Lock lock_;
    std::unordered_map<std::string, WebRestrictionsClientResult> cache_data_;
    DISALLOW_COPY_AND_ASSIGN(Cache);
  };

  void SetAuthorityTask(const std::string& content_provider_authority);

  void RecordURLAccess(const std::string& url);

  void UpdateCache(const std::string& provider_authority,
                   const std::string& url,
                   base::android::ScopedJavaGlobalRef<jobject> result);

  void RequestSupportKnown(const std::string& provider_authority,
                           bool supports_request);

  void ClearCache();

  static base::android::ScopedJavaGlobalRef<jobject> ShouldProceedTask(
      const std::string& url,
      const base::android::JavaRef<jobject>& java_provider);

  void OnShouldProceedComplete(
      std::string provider_authority,
      const std::string& url,
      const base::Callback<void(bool)>& callback,
      const base::android::ScopedJavaGlobalRef<jobject>& result);

  // Set up after SetAuthority().
  bool initialized_;
  bool supports_request_;
  base::android::ScopedJavaGlobalRef<jobject> java_provider_;
  std::string provider_authority_;
  Cache cache_;

  scoped_refptr<base::SequencedTaskRunner> background_task_runner_;
  std::list<std::string> recent_urls_;

  DISALLOW_COPY_AND_ASSIGN(WebRestrictionsClient);
};

}  // namespace web_restrictions

#endif  // COMPONENTS_WEB_RESTRICTION_WEB_RESTRICTIONS_CLIENT_H_
