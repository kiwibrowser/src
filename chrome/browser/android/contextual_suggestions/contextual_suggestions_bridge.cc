// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/contextual_suggestions/contextual_suggestions_bridge.h"

#include "base/android/callback_android.h"
#include "base/android/jni_string.h"
#include "base/callback.h"
#include "chrome/browser/android/chrome_feature_list.h"
#include "chrome/browser/ntp_snippets/contextual_content_suggestions_service_factory.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/ntp_snippets/category.h"
#include "components/ntp_snippets/content_suggestions_service.h"
#include "components/ntp_snippets/contextual/contextual_content_suggestions_service.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_features.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_metrics_reporter.h"
#include "components/ukm/content/source_url_recorder.h"
#include "content/public/browser/web_contents.h"
#include "jni/ContextualSuggestionsBridge_jni.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/image/image.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace contextual_suggestions {

static jlong JNI_ContextualSuggestionsBridge_Init(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  ContextualContentSuggestionsService* contextual_suggestions_service =
      ContextualContentSuggestionsServiceFactory::GetForProfile(profile);

  std::unique_ptr<ContextualContentSuggestionsServiceProxy> service_proxy =
      contextual_suggestions_service->CreateProxy();

  ContextualSuggestionsBridge* contextual_suggestions_bridge =
      new ContextualSuggestionsBridge(env, std::move(service_proxy));
  return reinterpret_cast<intptr_t>(contextual_suggestions_bridge);
}

static jboolean JNI_ContextualSuggestionsBridge_IsEnterprisePolicyManaged(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  // Bypass policy check, if corresponding feature is enabled.
  if (base::FeatureList::IsEnabled(
          contextual_suggestions::
              kContextualSuggestionsEnterprisePolicyBypass)) {
    return false;
  }

  // TODO(fgorski): This is simply checking whether the profile is managed by
  // an enterprise policy.
  // http://crbug.com/829460 covers implementation of policy controller for
  // contextual content suggestions.
  Profile* profile = ProfileManager::GetLastUsedProfile()->GetOriginalProfile();
  if (!profile)
    return false;

  policy::ProfilePolicyConnector* policy_connector =
      policy::ProfilePolicyConnectorFactory::GetForBrowserContext(profile);

  return (policy_connector != nullptr) && policy_connector->IsManaged();
}

ContextualSuggestionsBridge::ContextualSuggestionsBridge(
    JNIEnv* env,
    std::unique_ptr<ContextualContentSuggestionsServiceProxy> service_proxy)
    : service_proxy_(std::move(service_proxy)), weak_ptr_factory_(this) {}

ContextualSuggestionsBridge::~ContextualSuggestionsBridge() {}

void ContextualSuggestionsBridge::Destroy(JNIEnv* env,
                                          const JavaParamRef<jobject>& obj) {
  service_proxy_->FlushMetrics();
  delete this;
}

void ContextualSuggestionsBridge::FetchSuggestions(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& j_url,
    const JavaParamRef<jobject>& j_callback) {
  GURL url(ConvertJavaStringToUTF8(env, j_url));
  service_proxy_->FetchContextualSuggestions(
      url, base::BindOnce(&ContextualSuggestionsBridge::OnSuggestionsAvailable,
                          weak_ptr_factory_.GetWeakPtr(),
                          ScopedJavaGlobalRef<jobject>(j_callback)));
}

void ContextualSuggestionsBridge::FetchSuggestionImage(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& j_suggestion_id,
    const JavaParamRef<jobject>& j_callback) {
  std::string suggestion_id(ConvertJavaStringToUTF8(env, j_suggestion_id));
  service_proxy_->FetchContextualSuggestionImage(
      suggestion_id,
      base::BindOnce(&ContextualSuggestionsBridge::OnImageFetched,
                     weak_ptr_factory_.GetWeakPtr(),
                     ScopedJavaGlobalRef<jobject>(j_callback)));
}

void ContextualSuggestionsBridge::FetchSuggestionFavicon(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& j_suggestion_id,
    const JavaParamRef<jobject>& j_callback) {
  std::string suggestion_id(ConvertJavaStringToUTF8(env, j_suggestion_id));
  service_proxy_->FetchContextualSuggestionFavicon(
      suggestion_id,
      base::BindOnce(&ContextualSuggestionsBridge::OnImageFetched,
                     weak_ptr_factory_.GetWeakPtr(),
                     ScopedJavaGlobalRef<jobject>(j_callback)));
}

void ContextualSuggestionsBridge::ClearState(JNIEnv* env,
                                             const JavaParamRef<jobject>& obj) {
  service_proxy_->ClearState();
}

void ContextualSuggestionsBridge::ReportEvent(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& j_web_contents,
    jint j_event_id) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(j_web_contents);

  ukm::SourceId ukm_source_id =
      ukm::GetSourceIdForWebContentsDocument(web_contents);

  contextual_suggestions::ContextualSuggestionsEvent event =
      static_cast<contextual_suggestions::ContextualSuggestionsEvent>(
          j_event_id);

  service_proxy_->ReportEvent(
      ukm_source_id, web_contents->GetLastCommittedURL().spec(), event);
}

void ContextualSuggestionsBridge::OnSuggestionsAvailable(
    ScopedJavaGlobalRef<jobject> j_callback,
    ContextualSuggestionsResult result) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_result =
      Java_ContextualSuggestionsBridge_createContextualSuggestionsResult(
          env, ConvertUTF8ToJavaString(env, result.peek_text));
  Java_ContextualSuggestionsBridge_setPeekConditionsOnResult(
      env, j_result, result.peek_conditions.page_scroll_percentage,
      result.peek_conditions.minimum_seconds_on_page,
      result.peek_conditions.maximum_number_of_peeks);
  for (auto& cluster : result.clusters) {
    Java_ContextualSuggestionsBridge_addNewClusterToResult(
        env, j_result, ConvertUTF8ToJavaString(env, cluster.title));
    for (auto& suggestion : cluster.suggestions) {
      Java_ContextualSuggestionsBridge_addSuggestionToLastCluster(
          env, j_result, ConvertUTF8ToJavaString(env, suggestion.id),
          ConvertUTF8ToJavaString(env, suggestion.title),
          ConvertUTF8ToJavaString(env, suggestion.snippet),
          ConvertUTF8ToJavaString(env, suggestion.publisher_name),
          ConvertUTF8ToJavaString(env, suggestion.url.spec()),
          !suggestion.image_id.empty());
    }
  }
  RunCallbackAndroid(j_callback, j_result);
}

void ContextualSuggestionsBridge::OnImageFetched(
    ScopedJavaGlobalRef<jobject> j_callback,
    const gfx::Image& image) {
  ScopedJavaLocalRef<jobject> j_bitmap;
  if (!image.IsEmpty())
    j_bitmap = gfx::ConvertToJavaBitmap(image.ToSkBitmap());

  RunCallbackAndroid(j_callback, j_bitmap);
}

}  // namespace contextual_suggestions
