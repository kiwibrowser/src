// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search_engines/template_url_service_android.h"

#include <stddef.h>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/format_macros.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/search_engines/template_url_android.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "components/google/core/browser/google_util.h"
#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/util.h"
#include "components/search_provider_logos/features.h"
#include "components/search_provider_logos/switches.h"
#include "jni/TemplateUrlService_jni.h"
#include "net/base/url_util.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace {

Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}

}  // namespace

TemplateUrlServiceAndroid::TemplateUrlServiceAndroid(JNIEnv* env,
                                                     jobject obj)
    : weak_java_obj_(env, obj),
      template_url_service_(
          TemplateURLServiceFactory::GetForProfile(GetOriginalProfile())) {
  template_url_subscription_ =
      template_url_service_->RegisterOnLoadedCallback(
          base::Bind(&TemplateUrlServiceAndroid::OnTemplateURLServiceLoaded,
                     base::Unretained(this)));
  template_url_service_->AddObserver(this);
}

TemplateUrlServiceAndroid::~TemplateUrlServiceAndroid() {
  template_url_service_->RemoveObserver(this);
}

void TemplateUrlServiceAndroid::Load(JNIEnv* env,
                                     const JavaParamRef<jobject>& obj) {
  template_url_service_->Load();
}

void TemplateUrlServiceAndroid::SetUserSelectedDefaultSearchProvider(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jkeyword) {
  base::string16 keyword(
      base::android::ConvertJavaStringToUTF16(env, jkeyword));
  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForKeyword(keyword);
  template_url_service_->SetUserSelectedDefaultSearchProvider(template_url);
}

jboolean TemplateUrlServiceAndroid::IsLoaded(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) const {
  return template_url_service_->loaded();
}

jboolean TemplateUrlServiceAndroid::IsDefaultSearchManaged(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  return template_url_service_->is_default_search_managed();
}

jboolean TemplateUrlServiceAndroid::IsSearchByImageAvailable(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  const TemplateURL* default_search_provider =
      template_url_service_->GetDefaultSearchProvider();
  return default_search_provider &&
      !default_search_provider->image_url().empty() &&
      default_search_provider->image_url_ref().IsValid(
          template_url_service_->search_terms_data());
}

jboolean TemplateUrlServiceAndroid::IsDefaultSearchEngineGoogle(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  const TemplateURL* default_search_provider =
      template_url_service_->GetDefaultSearchProvider();
  return default_search_provider &&
      default_search_provider->url_ref().HasGoogleBaseURLs(
          template_url_service_->search_terms_data());
}

jboolean TemplateUrlServiceAndroid::DoesDefaultSearchEngineHaveLogo(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  // |kSearchProviderLogoURL| applies to all search engines (Google or
  // third-party).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          search_provider_logos::switches::kSearchProviderLogoURL)) {
    return true;
  }

  // Google always has a logo.
  if (IsDefaultSearchEngineGoogle(env, obj))
    return true;

  // Third-party search engines can have a doodle specified via the command
  // line, or a static logo or doodle from the TemplateURLService.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          search_provider_logos::switches::kThirdPartyDoodleURL)) {
    return true;
  }
  const TemplateURL* default_search_provider =
      template_url_service_->GetDefaultSearchProvider();
  return default_search_provider &&
         (default_search_provider->doodle_url().is_valid() ||
          default_search_provider->logo_url().is_valid());
}

jboolean
TemplateUrlServiceAndroid::IsSearchResultsPageFromDefaultSearchProvider(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& jurl) {
  GURL url(base::android::ConvertJavaStringToUTF8(env, jurl));
  return template_url_service_->IsSearchResultsPageFromDefaultSearchProvider(
      url);
}

void TemplateUrlServiceAndroid::OnTemplateURLServiceLoaded() {
  template_url_subscription_.reset();
  JNIEnv* env = base::android::AttachCurrentThread();
  auto java_obj = weak_java_obj_.get(env);
  if (java_obj.is_null())
    return;

  Java_TemplateUrlService_templateUrlServiceLoaded(env, java_obj);
}

void TemplateUrlServiceAndroid::OnTemplateURLServiceChanged() {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto java_obj = weak_java_obj_.get(env);
  if (java_obj.is_null())
    return;

  Java_TemplateUrlService_onTemplateURLServiceChanged(env, java_obj);
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::GetUrlForSearchQuery(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jquery) {
  const TemplateURL* default_provider =
      template_url_service_->GetDefaultSearchProvider();

  base::string16 query(base::android::ConvertJavaStringToUTF16(env, jquery));

  std::string url;
  if (default_provider &&
      default_provider->url_ref().SupportsReplacement(
          template_url_service_->search_terms_data()) &&
      !query.empty()) {
    url = default_provider->url_ref().ReplaceSearchTerms(
        TemplateURLRef::SearchTermsArgs(query),
        template_url_service_->search_terms_data());
  }

  return base::android::ConvertUTF8ToJavaString(env, url);
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::GetUrlForVoiceSearchQuery(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jquery) {
  base::string16 query(base::android::ConvertJavaStringToUTF16(env, jquery));
  std::string url;

  if (!query.empty()) {
    GURL gurl(GetDefaultSearchURLForSearchTerms(template_url_service_, query));
    if (google_util::IsGoogleSearchUrl(gurl))
      gurl = net::AppendQueryParameter(gurl, "inm", "vs");
    url = gurl.spec();
  }

  return base::android::ConvertUTF8ToJavaString(env, url);
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::ReplaceSearchTermsInUrl(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jquery,
    const JavaParamRef<jstring>& jcurrent_url) {
  const TemplateURL* default_provider =
      template_url_service_->GetDefaultSearchProvider();

  base::string16 query(base::android::ConvertJavaStringToUTF16(env, jquery));
  GURL current_url(base::android::ConvertJavaStringToUTF16(env, jcurrent_url));
  GURL destination_url(current_url);
  if (default_provider && !query.empty()) {
    bool refined_query = default_provider->ReplaceSearchTermsInURL(
        current_url, TemplateURLRef::SearchTermsArgs(query),
        template_url_service_->search_terms_data(), &destination_url);
    if (refined_query)
      return base::android::ConvertUTF8ToJavaString(
          env, destination_url.spec());
  }
  return base::android::ScopedJavaLocalRef<jstring>(env, NULL);
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::GetUrlForContextualSearchQuery(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jquery,
    const JavaParamRef<jstring>& jalternate_term,
    jboolean jshould_prefetch,
    const JavaParamRef<jstring>& jprotocol_version) {
  base::string16 query(base::android::ConvertJavaStringToUTF16(env, jquery));
  std::string url;

  if (!query.empty()) {
    GURL gurl(GetDefaultSearchURLForSearchTerms(template_url_service_, query));
    if (google_util::IsGoogleSearchUrl(gurl)) {
      std::string protocol_version(
          base::android::ConvertJavaStringToUTF8(env, jprotocol_version));
      gurl = net::AppendQueryParameter(gurl, "ctxs", protocol_version);
      if (jshould_prefetch) {
        // Indicate that the search page is being prefetched.
        gurl = net::AppendQueryParameter(gurl, "pf", "c");
      }

      if (jalternate_term) {
        std::string alternate_term(
            base::android::ConvertJavaStringToUTF8(env, jalternate_term));
        if (!alternate_term.empty()) {
          gurl = net::AppendQueryParameter(
              gurl, "ctxsl_alternate_term", alternate_term);
        }
      }
    }
    url = gurl.spec();
  }

  return base::android::ConvertUTF8ToJavaString(env, url);
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::GetSearchEngineUrlFromTemplateUrl(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jkeyword) {
  base::string16 keyword =
      base::android::ConvertJavaStringToUTF16(env, jkeyword);
  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForKeyword(keyword);
  if (!template_url)
    return base::android::ScopedJavaLocalRef<jstring>(env, nullptr);
  std::string url(template_url->url_ref().ReplaceSearchTerms(
      TemplateURLRef::SearchTermsArgs(base::ASCIIToUTF16("query")),
      template_url_service_->search_terms_data()));
  return base::android::ConvertUTF8ToJavaString(env, url);
}

int TemplateUrlServiceAndroid::GetSearchEngineTypeFromTemplateUrl(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& jkeyword) {
  base::string16 keyword =
      base::android::ConvertJavaStringToUTF16(env, jkeyword);
  TemplateURL* template_url =
      template_url_service_->GetTemplateURLForKeyword(keyword);
  if (!template_url)
    return -1;
  const SearchTermsData& search_terms_data =
      template_url_service_->search_terms_data();
  return template_url->GetEngineType(search_terms_data);
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::AddSearchEngineForTesting(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& jkeyword,
    jint age_in_days) {
  TemplateURLData data;
  base::string16 keyword =
      base::android::ConvertJavaStringToUTF16(env, jkeyword);
  data.SetShortName(keyword);
  data.SetKeyword(keyword);
  data.SetURL("https://testurl.com/?searchstuff={searchTerms}");
  data.favicon_url = GURL("http://favicon.url");
  data.safe_for_autoreplace = true;
  data.input_encodings.push_back("UTF-8");
  data.prepopulate_id = 0;
  data.date_created =
      base::Time::Now() - base::TimeDelta::FromDays((int) age_in_days);
  data.last_modified =
      base::Time::Now() - base::TimeDelta::FromDays((int) age_in_days);
  data.last_visited =
      base::Time::Now() - base::TimeDelta::FromDays((int) age_in_days);
  TemplateURL* t_url =
      template_url_service_->Add(std::make_unique<TemplateURL>(data));
  CHECK(t_url) << "Failed adding template url for: " << keyword;
  return base::android::ConvertUTF16ToJavaString(env, t_url->data().keyword());
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::UpdateLastVisitedForTesting(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& jkeyword) {
  base::string16 keyword =
      base::android::ConvertJavaStringToUTF16(env, jkeyword);
  TemplateURL* t_url = template_url_service_->GetTemplateURLForKeyword(keyword);
  template_url_service_->UpdateTemplateURLVisitTime(t_url);
  return base::android::ConvertUTF16ToJavaString(env, t_url->data().keyword());
}

base::android::ScopedJavaLocalRef<jstring>
TemplateUrlServiceAndroid::ExtractSearchTermsFromUrl(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jstring>& jurl) {
  base::string16 search_terms;
  const TemplateURL* default_search_provider =
      template_url_service_->GetDefaultSearchProvider();
  bool has_search_terms = default_search_provider->ExtractSearchTermsFromURL(
      GURL(base::android::ConvertJavaStringToUTF8(jurl)),
      template_url_service_->search_terms_data(), &search_terms);
  return base::android::ConvertUTF16ToJavaString(
      env, (has_search_terms ? search_terms : base::string16()));
}

void TemplateUrlServiceAndroid::GetTemplateUrls(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jobject>& template_url_list_obj) {
  std::vector<TemplateURL*> template_urls =
      template_url_service_->GetTemplateURLs();
  for (TemplateURL* template_url : template_urls) {
    base::android::ScopedJavaLocalRef<jobject> j_template_url =
        CreateTemplateUrlAndroid(env, template_url);
    Java_TemplateUrlService_addTemplateUrlToList(env, template_url_list_obj,
                                                 j_template_url);
  }
}

base::android::ScopedJavaLocalRef<jobject>
TemplateUrlServiceAndroid::GetDefaultSearchEngine(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  const TemplateURL* default_search_provider =
      template_url_service_->GetDefaultSearchProvider();
  if (default_search_provider == nullptr) {
    return base::android::ScopedJavaLocalRef<jobject>(env, nullptr);
  }
  return CreateTemplateUrlAndroid(env, default_search_provider);
}

static jlong JNI_TemplateUrlService_Init(JNIEnv* env,
                                         const JavaParamRef<jobject>& obj) {
  TemplateUrlServiceAndroid* template_url_service_android =
      new TemplateUrlServiceAndroid(env, obj);
  return reinterpret_cast<intptr_t>(template_url_service_android);
}
