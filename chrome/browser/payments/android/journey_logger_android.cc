// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/payments/android/journey_logger_android.h"

#include "base/android/jni_string.h"
#include "jni/JourneyLogger_jni.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "url/gurl.h"

namespace payments {
namespace {

using ::base::android::JavaParamRef;
using ::base::android::ConvertJavaStringToUTF8;

}  // namespace

JourneyLoggerAndroid::JourneyLoggerAndroid(bool is_incognito,
                                           const std::string& url)
    : journey_logger_(is_incognito, GURL(url), ukm::UkmRecorder::Get()) {}

JourneyLoggerAndroid::~JourneyLoggerAndroid() {}

void JourneyLoggerAndroid::Destroy(JNIEnv* env,
                                   const JavaParamRef<jobject>& jcaller) {
  delete this;
}

void JourneyLoggerAndroid::SetNumberOfSuggestionsShown(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jint jsection,
    jint jnumber,
    jboolean jhas_complete_suggestion) {
  DCHECK_GE(jsection, 0);
  DCHECK_LT(jsection, JourneyLogger::Section::SECTION_MAX);
  journey_logger_.SetNumberOfSuggestionsShown(
      static_cast<JourneyLogger::Section>(jsection), jnumber,
      jhas_complete_suggestion);
}

void JourneyLoggerAndroid::IncrementSelectionChanges(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jint jsection) {
  DCHECK_GE(jsection, 0);
  DCHECK_LT(jsection, JourneyLogger::Section::SECTION_MAX);
  journey_logger_.IncrementSelectionChanges(
      static_cast<JourneyLogger::Section>(jsection));
}

void JourneyLoggerAndroid::IncrementSelectionEdits(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jint jsection) {
  DCHECK_GE(jsection, 0);
  DCHECK_LT(jsection, JourneyLogger::Section::SECTION_MAX);
  journey_logger_.IncrementSelectionEdits(
      static_cast<JourneyLogger::Section>(jsection));
}

void JourneyLoggerAndroid::IncrementSelectionAdds(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jint jsection) {
  DCHECK_GE(jsection, 0);
  DCHECK_LT(jsection, JourneyLogger::Section::SECTION_MAX);
  journey_logger_.IncrementSelectionAdds(
      static_cast<JourneyLogger::Section>(jsection));
}

void JourneyLoggerAndroid::SetCanMakePaymentValue(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jboolean jvalue) {
  journey_logger_.SetCanMakePaymentValue(jvalue);
}

void JourneyLoggerAndroid::SetEventOccurred(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jint jevent) {
  DCHECK_GE(jevent, 0);
  DCHECK_LT(jevent, JourneyLogger::Event::EVENT_ENUM_MAX);
  journey_logger_.SetEventOccurred(static_cast<JourneyLogger::Event>(jevent));
}

void JourneyLoggerAndroid::SetRequestedInformation(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jboolean requested_shipping,
    jboolean requested_email,
    jboolean requested_phone,
    jboolean requested_name) {
  journey_logger_.SetRequestedInformation(requested_shipping, requested_email,
                                          requested_phone, requested_name);
}

void JourneyLoggerAndroid::SetRequestedPaymentMethodTypes(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jboolean requested_basic_card,
    jboolean requested_method_google,
    jboolean requested_method_other) {
  journey_logger_.SetRequestedPaymentMethodTypes(
      requested_basic_card, requested_method_google, requested_method_other);
}

void JourneyLoggerAndroid::SetCompleted(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  journey_logger_.SetCompleted();
}

void JourneyLoggerAndroid::SetAborted(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jint jreason) {
  DCHECK_GE(jreason, 0);
  DCHECK_LT(jreason, JourneyLogger::AbortReason::ABORT_REASON_MAX);
  journey_logger_.SetAborted(static_cast<JourneyLogger::AbortReason>(jreason));
}

void JourneyLoggerAndroid::SetNotShown(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jint jreason) {
  DCHECK_GE(jreason, 0);
  DCHECK_LT(jreason, JourneyLogger::NotShownReason::NOT_SHOWN_REASON_MAX);
  journey_logger_.SetNotShown(
      static_cast<JourneyLogger::NotShownReason>(jreason));
}

static jlong JNI_JourneyLogger_InitJourneyLoggerAndroid(
    JNIEnv* env,
    const JavaParamRef<jobject>& jcaller,
    jboolean jis_incognito,
    const base::android::JavaParamRef<jstring>& jurl) {
  return reinterpret_cast<jlong>(new JourneyLoggerAndroid(
      jis_incognito, ConvertJavaStringToUTF8(env, jurl)));
}

}  // namespace payments
