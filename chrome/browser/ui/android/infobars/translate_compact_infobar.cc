// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/translate_compact_infobar.h"

#include <stddef.h>

#include <memory>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "chrome/browser/translate/android/translate_utils.h"
#include "components/translate/core/browser/translate_infobar_delegate.h"
#include "components/variations/variations_associated_data.h"
#include "jni/TranslateCompactInfoBar_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

// Default values for the finch parameters. (used when the corresponding finch
// parameter does not exist.)
const int kDefaultAutoAlwaysThreshold = 5;
const int kDefaultAutoNeverThreshold = 10;
const int kDefaultMaxNumberOfAutoAlways = 2;
const int kDefaultMaxNumberOfAutoNever = 2;

// Finch parameter names:
const char kTranslateAutoAlwaysThreshold[] = "translate_auto_always_threshold";
const char kTranslateAutoNeverThreshold[] = "translate_auto_never_threshold";
const char kTranslateMaxNumberOfAutoAlways[] =
    "translate_max_number_of_auto_always";
const char kTranslateMaxNumberOfAutoNever[] =
    "translate_max_number_of_auto_never";
const char kTranslateTabDefaultTextColor[] = "translate_tab_default_text_color";

// ChromeTranslateClient
// ----------------------------------------------------------

std::unique_ptr<infobars::InfoBar> ChromeTranslateClient::CreateInfoBar(
    std::unique_ptr<translate::TranslateInfoBarDelegate> delegate) const {
  return std::make_unique<TranslateCompactInfoBar>(std::move(delegate));
}

// TranslateInfoBar -----------------------------------------------------------

TranslateCompactInfoBar::TranslateCompactInfoBar(
    std::unique_ptr<translate::TranslateInfoBarDelegate> delegate)
    : InfoBarAndroid(std::move(delegate)), action_flags_(FLAG_NONE) {
  GetDelegate()->SetObserver(this);

  // Flip the translate bit if auto translate is enabled.
  if (GetDelegate()->translate_step() == translate::TRANSLATE_STEP_TRANSLATING)
    action_flags_ |= FLAG_TRANSLATE;
}

TranslateCompactInfoBar::~TranslateCompactInfoBar() {
  GetDelegate()->SetObserver(nullptr);
}

ScopedJavaLocalRef<jobject> TranslateCompactInfoBar::CreateRenderInfoBar(
    JNIEnv* env) {
  translate::TranslateInfoBarDelegate* delegate = GetDelegate();

  base::android::ScopedJavaLocalRef<jobjectArray> java_languages =
      TranslateUtils::GetJavaLanguages(env, delegate);
  base::android::ScopedJavaLocalRef<jobjectArray> java_codes =
      TranslateUtils::GetJavaLanguageCodes(env, delegate);
  base::android::ScopedJavaLocalRef<jintArray> java_hash_codes =
      TranslateUtils::GetJavaLanguageHashCodes(env, delegate);

  ScopedJavaLocalRef<jstring> source_language_code =
      base::android::ConvertUTF8ToJavaString(
          env, delegate->original_language_code());

  ScopedJavaLocalRef<jstring> target_language_code =
      base::android::ConvertUTF8ToJavaString(env,
                                             delegate->target_language_code());
  return Java_TranslateCompactInfoBar_create(
      env, delegate->translate_step(), source_language_code,
      target_language_code, delegate->ShouldAlwaysTranslate(),
      delegate->triggered_from_menu(), java_languages, java_codes,
      java_hash_codes, TabDefaultTextColor());
}

void TranslateCompactInfoBar::ProcessButton(int action) {
  if (!owner())
    return;  // We're closing; don't call anything, it might access the owner.

  translate::TranslateInfoBarDelegate* delegate = GetDelegate();
  if (action == InfoBarAndroid::ACTION_TRANSLATE) {
    action_flags_ |= FLAG_TRANSLATE;
    delegate->Translate();
    if (!delegate->ShouldAlwaysTranslate() && ShouldAutoAlwaysTranslate()) {
      JNIEnv* env = base::android::AttachCurrentThread();
      Java_TranslateCompactInfoBar_setAutoAlwaysTranslate(env,
                                                          GetJavaInfoBar());

      // Auto-always is triggered by the line above.  Need to increment the
      // auto-always counter.
      delegate->IncrementTranslationAutoAlwaysCount();
      // Reset translateAcceptedCount so that auto-always could be triggered
      // again.
      delegate->ResetTranslationAcceptedCount();
    }
  } else if (action == InfoBarAndroid::ACTION_TRANSLATE_SHOW_ORIGINAL) {
    action_flags_ |= FLAG_REVERT;
    delegate->RevertWithoutClosingInfobar();
  } else {
    DCHECK_EQ(InfoBarAndroid::ACTION_NONE, action);
  }
}

void TranslateCompactInfoBar::SetJavaInfoBar(
    const base::android::JavaRef<jobject>& java_info_bar) {
  InfoBarAndroid::SetJavaInfoBar(java_info_bar);
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_TranslateCompactInfoBar_setNativePtr(env, java_info_bar,
                                            reinterpret_cast<intptr_t>(this));
}

void TranslateCompactInfoBar::ApplyStringTranslateOption(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    int option,
    const JavaParamRef<jstring>& value) {
  translate::TranslateInfoBarDelegate* delegate = GetDelegate();
  if (option == TranslateUtils::OPTION_SOURCE_CODE) {
    std::string source_code =
        base::android::ConvertJavaStringToUTF8(env, value);
    if (delegate->original_language_code().compare(source_code) != 0)
      delegate->UpdateOriginalLanguage(source_code);
  } else if (option == TranslateUtils::OPTION_TARGET_CODE) {
    std::string target_code =
        base::android::ConvertJavaStringToUTF8(env, value);
    if (delegate->target_language_code().compare(target_code) != 0)
      delegate->UpdateTargetLanguage(target_code);
  } else {
    DCHECK(false);
  }
}

void TranslateCompactInfoBar::ApplyBoolTranslateOption(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    int option,
    jboolean value) {
  translate::TranslateInfoBarDelegate* delegate = GetDelegate();
  if (option == TranslateUtils::OPTION_ALWAYS_TRANSLATE) {
    if (delegate->ShouldAlwaysTranslate() != value) {
      action_flags_ |= FLAG_ALWAYS_TRANSLATE;
      delegate->ToggleAlwaysTranslate();
    }
  } else if (option == TranslateUtils::OPTION_NEVER_TRANSLATE) {
    if (value && delegate->IsTranslatableLanguageByPrefs()) {
      action_flags_ |= FLAG_NEVER_LANGUAGE;
      delegate->ToggleTranslatableLanguageByPrefs();
    }
  } else if (option == TranslateUtils::OPTION_NEVER_TRANSLATE_SITE) {
    if (value && !delegate->IsSiteBlacklisted()) {
      action_flags_ |= FLAG_NEVER_SITE;
      delegate->ToggleSiteBlacklist();
    }
  } else {
    DCHECK(false);
  }
}

bool TranslateCompactInfoBar::ShouldAutoAlwaysTranslate() {
  translate::TranslateInfoBarDelegate* delegate = GetDelegate();
  return (delegate->GetTranslationAcceptedCount() >= AutoAlwaysThreshold() &&
          delegate->GetTranslationAutoAlwaysCount() < MaxNumberOfAutoAlways());
}

jboolean TranslateCompactInfoBar::ShouldAutoNeverTranslate(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    jboolean menu_expanded) {
  // Flip menu expanded bit.
  if (menu_expanded)
    action_flags_ |= FLAG_EXPAND_MENU;

  if (!IsDeclinedByUser())
    return false;

  translate::TranslateInfoBarDelegate* delegate = GetDelegate();
  // Don't trigger if it's off the record or already blocked.
  if (delegate->is_off_the_record() ||
      !delegate->IsTranslatableLanguageByPrefs())
    return false;

  int auto_never_count = delegate->GetTranslationAutoNeverCount();

  // At the beginning (auto_never_count == 0), deniedCount starts at 0 and is
  // off-by-one (because this checking is done before increment). However, after
  // auto-never is triggered once (auto_never_count > 0), deniedCount starts at
  // 1.  So there is no off-by-one by then.
  int off_by_one = auto_never_count == 0 ? 1 : 0;

  bool never_translate = (delegate->GetTranslationDeniedCount() + off_by_one >=
                              AutoNeverThreshold() &&
                          auto_never_count < MaxNumberOfAutoNever());
  if (never_translate) {
    // Auto-never will be triggered.  Need to increment the auto-never counter.
    delegate->IncrementTranslationAutoNeverCount();
    // Reset translateDeniedCount so that auto-never could be triggered again.
    delegate->ResetTranslationDeniedCount();
  }
  return never_translate;
}

int TranslateCompactInfoBar::GetParam(const std::string& paramName,
                                      int default_value) {
  std::map<std::string, std::string> params;
  if (!variations::GetVariationParams(translate::kTranslateCompactUI.name,
                                      &params))
    return default_value;
  int value = 0;
  base::StringToInt(params[paramName], &value);
  return value <= 0 ? default_value : value;
}

int TranslateCompactInfoBar::AutoAlwaysThreshold() {
  return GetParam(kTranslateAutoAlwaysThreshold, kDefaultAutoAlwaysThreshold);
}

int TranslateCompactInfoBar::AutoNeverThreshold() {
  return GetParam(kTranslateAutoNeverThreshold, kDefaultAutoNeverThreshold);
}

int TranslateCompactInfoBar::MaxNumberOfAutoAlways() {
  return GetParam(kTranslateMaxNumberOfAutoAlways,
                  kDefaultMaxNumberOfAutoAlways);
}

int TranslateCompactInfoBar::MaxNumberOfAutoNever() {
  return GetParam(kTranslateMaxNumberOfAutoNever, kDefaultMaxNumberOfAutoNever);
}

int TranslateCompactInfoBar::TabDefaultTextColor() {
  return GetParam(kTranslateTabDefaultTextColor, 0);
}

translate::TranslateInfoBarDelegate* TranslateCompactInfoBar::GetDelegate() {
  return delegate()->AsTranslateInfoBarDelegate();
}

void TranslateCompactInfoBar::OnTranslateStepChanged(
    translate::TranslateStep step,
    translate::TranslateErrors::Type error_type) {
  if (!owner())
    return;  // We're closing; don't call anything.

  if ((step == translate::TRANSLATE_STEP_AFTER_TRANSLATE) ||
      (step == translate::TRANSLATE_STEP_TRANSLATE_ERROR)) {
    JNIEnv* env = base::android::AttachCurrentThread();
    Java_TranslateCompactInfoBar_onPageTranslated(env, GetJavaInfoBar(),
                                                  error_type);
  }
}

bool TranslateCompactInfoBar::IsDeclinedByUser() {
  // Whether there is any affirmative action bit.
  return action_flags_ == FLAG_NONE;
}
