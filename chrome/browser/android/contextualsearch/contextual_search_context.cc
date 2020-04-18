// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <chrome/browser/android/contextualsearch/contextual_search_context.h>

#include "base/android/jni_string.h"
#include "components/translate/core/common/translate_constants.h"
#include "components/translate/core/language_detection/language_detection_util.h"
#include "content/public/browser/browser_thread.h"
#include "jni/ContextualSearchContext_jni.h"

ContextualSearchContext::ContextualSearchContext(JNIEnv* env, jobject obj)
    : can_resolve(false),
      can_send_base_page_url(false),
      home_country(std::string()),
      base_page_url(GURL()),
      surrounding_text(base::string16()),
      start_offset(0),
      end_offset(0),
      weak_factory_(this) {
  java_object_.Reset(env, obj);
}

ContextualSearchContext::ContextualSearchContext(
    const std::string& home_country,
    const GURL& page_url,
    const std::string& encoding)
    : home_country(home_country),
      base_page_url(page_url),
      base_page_encoding(encoding),
      weak_factory_(this) {
  java_object_ = nullptr;
}

ContextualSearchContext::~ContextualSearchContext() {
}

// static
base::WeakPtr<ContextualSearchContext>
ContextualSearchContext::FromJavaContextualSearchContext(
    const base::android::JavaRef<jobject>& j_contextual_search_context) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (j_contextual_search_context.is_null())
    return NULL;

  ContextualSearchContext* contextual_search_context =
      reinterpret_cast<ContextualSearchContext*>(
          Java_ContextualSearchContext_getNativePointer(
              base::android::AttachCurrentThread(),
              j_contextual_search_context));
  return contextual_search_context->GetWeakPtr();
}

void ContextualSearchContext::SetResolveProperties(
    JNIEnv* env,
    jobject obj,
    const base::android::JavaParamRef<jstring>& j_home_country,
    jboolean j_may_send_base_page_url) {
  can_resolve = true;
  home_country = base::android::ConvertJavaStringToUTF8(env, j_home_country);
  can_send_base_page_url = j_may_send_base_page_url;
}

void ContextualSearchContext::AdjustSelection(JNIEnv* env,
                                              jobject obj,
                                              jint j_start_adjust,
                                              jint j_end_adjust) {
  DCHECK(start_offset + j_start_adjust >= 0);
  DCHECK(start_offset + j_start_adjust <= (int)surrounding_text.length());
  DCHECK(end_offset + j_end_adjust >= 0);
  DCHECK(end_offset + j_end_adjust <= (int)surrounding_text.length());
  start_offset += j_start_adjust;
  end_offset += j_end_adjust;
}

// Accessors

bool ContextualSearchContext::CanResolve() const {
  return can_resolve;
}

bool ContextualSearchContext::CanSendBasePageUrl() const {
  return can_send_base_page_url;
}

const GURL ContextualSearchContext::GetBasePageUrl() const {
  return base_page_url;
}

void ContextualSearchContext::SetBasePageUrl(const GURL& base_page_url) {
  this->base_page_url = base_page_url;
}

const std::string ContextualSearchContext::GetBasePageEncoding() const {
  return base_page_encoding;
}

void ContextualSearchContext::SetBasePageEncoding(
    const std::string& base_page_encoding) {
  this->base_page_encoding = base_page_encoding;
}

const std::string ContextualSearchContext::GetHomeCountry() const {
  return home_country;
}

void ContextualSearchContext::SetSelectionSurroundings(
    int start_offset,
    int end_offset,
    const base::string16& surrounding_text) {
  this->start_offset = start_offset;
  this->end_offset = end_offset;
  this->surrounding_text = surrounding_text;
}

const base::string16 ContextualSearchContext::GetSurroundingText() const {
  return surrounding_text;
}

int ContextualSearchContext::GetStartOffset() const {
  return start_offset;
}

int ContextualSearchContext::GetEndOffset() const {
  return end_offset;
}

base::android::ScopedJavaLocalRef<jstring>
ContextualSearchContext::DetectLanguage(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  std::string content_language;
  std::string html_lang;
  std::string cld_language;
  bool is_cld_reliable;
  std::string language = translate::DeterminePageLanguage(
      content_language, html_lang, this->surrounding_text, &cld_language,
      &is_cld_reliable);
  // Make sure we return an empty string when unreliable or an unknown result.
  if (!is_cld_reliable || language == translate::kUnknownLanguageCode)
    language = "";
  base::android::ScopedJavaLocalRef<jstring> j_language =
      base::android::ConvertUTF8ToJavaString(env, language);
  return j_language;
}

base::WeakPtr<ContextualSearchContext> ContextualSearchContext::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

// Java wrapper boilerplate

void ContextualSearchContext::Destroy(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  delete this;
}

jlong JNI_ContextualSearchContext_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  ContextualSearchContext* context = new ContextualSearchContext(env, obj);
  return reinterpret_cast<intptr_t>(context);
}
