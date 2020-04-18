// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CONTEXTUAL_SUGGESTIONS_CONTEXTUAL_SUGGESTIONS_BRIDGE_H_
#define CHROME_BROWSER_ANDROID_CONTEXTUAL_SUGGESTIONS_CONTEXTUAL_SUGGESTIONS_BRIDGE_H_

#include <memory>

#include "base/android/scoped_java_ref.h"
#include "base/memory/weak_ptr.h"
#include "components/ntp_snippets/contextual/contextual_content_suggestions_service_proxy.h"

namespace gfx {
class Image;
}  // namespace gfx

namespace contextual_suggestions {

class ContextualContentSuggestionsServiceProxy;

// Class implementing native side of ContextualSuggestionsBrigde.java.
class ContextualSuggestionsBridge {
 public:
  ContextualSuggestionsBridge(
      JNIEnv* env,
      std::unique_ptr<ContextualContentSuggestionsServiceProxy> service_proxy);

  // Deletes the bridge.
  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  // Fetches suggestions for a gives |j_url| and passes results to Java side
  // using |j_callback|.
  void FetchSuggestions(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj,
                        const base::android::JavaParamRef<jstring>& j_url,
                        const base::android::JavaParamRef<jobject>& j_callback);

  // Fetches an image corresponding to suggestion with |j_suggestion_id| and
  // passes results to Java side using |j_callback|.
  void FetchSuggestionImage(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& j_suggestion_id,
      const base::android::JavaParamRef<jobject>& j_callback);

  // Fetches a favicon corresponding to suggestion with |j_suggestion_id| and
  // passes results to Java side using |j_callback|.
  void FetchSuggestionFavicon(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      const base::android::JavaParamRef<jstring>& j_suggestion_id,
      const base::android::JavaParamRef<jobject>& j_callback);

  // Requests the backend to clear state related to this bridge.
  void ClearState(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  // Reports an event happening in UI (for the purpose of metrics collection).
  void ReportEvent(JNIEnv* env,
                   const base::android::JavaParamRef<jobject>& obj,
                   const base::android::JavaParamRef<jobject>& j_web_contents,
                   jint j_event_id);

 private:
  ~ContextualSuggestionsBridge();

  void OnSuggestionsAvailable(
      base::android::ScopedJavaGlobalRef<jobject> j_callback,
      ContextualSuggestionsResult result);

  void OnImageFetched(base::android::ScopedJavaGlobalRef<jobject> j_callback,
                      const gfx::Image& image);

  // Proxy to contextual content suggestions service used for fetching
  // suggestions and images.
  std::unique_ptr<ContextualContentSuggestionsServiceProxy> service_proxy_;

  base::WeakPtrFactory<ContextualSuggestionsBridge> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ContextualSuggestionsBridge);
};

}  // namespace contextual_suggestions

#endif  // CHROME_BROWSER_ANDROID_CONTEXTUAL_SUGGESTIONS_CONTEXTUAL_SUGGESTIONS_BRIDGE_H_
