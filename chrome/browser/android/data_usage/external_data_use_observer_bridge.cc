// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/data_usage/external_data_use_observer_bridge.h"

#include <memory>
#include <vector>

#include "base/android/jni_string.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/android/data_usage/data_use_tab_model.h"
#include "chrome/browser/android/data_usage/external_data_use_observer.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/browser_thread.h"
#include "jni/ExternalDataUseObserver_jni.h"

using base::android::ConvertUTF8ToJavaString;

namespace {

// Name of the external data use observer synthetic field trial, enabled and
// disabled groups.
const char kSyntheticFieldTrial[] = "SyntheticExternalDataUseObserver";
const char kSyntheticFieldTrialEnabledGroup[] = "Enabled";
const char kSyntheticFieldTrialDisabledGroup[] = "Disabled";

// Returns the package name of the control app from the field trial.
const std::string GetControlAppPackageName() {
  return variations::GetVariationParamValue(
      android::ExternalDataUseObserver::kExternalDataUseObserverFieldTrial,
      "control_app_package_name");
}

// Returns the google variation ID from the field trial.
variations::VariationID GetGoogleVariationID() {
  variations::VariationID variation_id;
  std::string variation_value = variations::GetVariationParamValue(
      android::ExternalDataUseObserver::kExternalDataUseObserverFieldTrial,
      "variation_id");
  if (!variation_value.empty() &&
      base::StringToInt(variation_value, &variation_id)) {
    return variation_id;
  }
  return variations::EMPTY_ID;
}

}  // namespace

namespace android {

ExternalDataUseObserverBridge::ExternalDataUseObserverBridge()
    : construct_time_(base::TimeTicks::Now()),
      is_first_matching_rule_fetch_(true) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

  // Detach from IO thread since rest of ExternalDataUseObserverBridge lives on
  // the UI thread.
  thread_checker_.DetachFromThread();
}

ExternalDataUseObserverBridge::~ExternalDataUseObserverBridge() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (j_external_data_use_observer_.is_null())
    return;
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_ExternalDataUseObserver_onDestroy(env, j_external_data_use_observer_);
}

void ExternalDataUseObserverBridge::Init(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    base::WeakPtr<ExternalDataUseObserver> external_data_use_observer,
    DataUseTabModel* data_use_tab_model) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  DCHECK(io_task_runner);
  DCHECK(j_external_data_use_observer_.is_null());
  // |data_use_tab_model| is guaranteed to be non-null because this method is
  // called in the constructor of ExternalDataUseObserver.
  DCHECK(data_use_tab_model);

  external_data_use_observer_ = external_data_use_observer;
  data_use_tab_model_ = data_use_tab_model->GetWeakPtr();
  io_task_runner_ = io_task_runner;

  JNIEnv* env = base::android::AttachCurrentThread();
  j_external_data_use_observer_.Reset(Java_ExternalDataUseObserver_create(
      env, reinterpret_cast<intptr_t>(this)));
  DCHECK(!j_external_data_use_observer_.is_null());

  Java_ExternalDataUseObserver_initControlAppManager(
      env, j_external_data_use_observer_,
      ConvertUTF8ToJavaString(env, GetControlAppPackageName()));
}

void ExternalDataUseObserverBridge::FetchMatchingRules() const {
  DCHECK(thread_checker_.CalledOnValidThread());

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_ExternalDataUseObserver_fetchMatchingRules(
      env, j_external_data_use_observer_);
}

void ExternalDataUseObserverBridge::FetchMatchingRulesDone(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    const base::android::JavaParamRef<jobjectArray>& app_package_name,
    const base::android::JavaParamRef<jobjectArray>& domain_path_regex,
    const base::android::JavaParamRef<jobjectArray>& label) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!construct_time_.is_null());

  // Convert to native objects.
  std::vector<std::string> app_package_name_native;
  std::vector<std::string> domain_path_regex_native;
  std::vector<std::string> label_native;

  if (app_package_name && domain_path_regex && label) {
    base::android::AppendJavaStringArrayToStringVector(
        env, app_package_name, &app_package_name_native);
    base::android::AppendJavaStringArrayToStringVector(
        env, domain_path_regex, &domain_path_regex_native);
    base::android::AppendJavaStringArrayToStringVector(env, label,
                                                       &label_native);
  }

  DCHECK_EQ(app_package_name_native.size(), domain_path_regex_native.size());
  DCHECK_EQ(app_package_name_native.size(), label_native.size());

  if (!data_use_tab_model_)
    return;

  if (is_first_matching_rule_fetch_) {
    is_first_matching_rule_fetch_ = false;
    UMA_HISTOGRAM_TIMES("DataUsage.Perf.MatchingRuleFirstFetchDuration",
                        base::TimeTicks::Now() - construct_time_);
  }

  data_use_tab_model_->RegisterURLRegexes(
      app_package_name_native, domain_path_regex_native, label_native);
}

void ExternalDataUseObserverBridge::ReportDataUse(
    const std::string& label,
    const std::string& tag,
    net::NetworkChangeNotifier::ConnectionType connection_type,
    const std::string& mcc_mnc,
    base::Time start_time,
    base::Time end_time,
    int64_t bytes_downloaded,
    int64_t bytes_uploaded) const {
  DCHECK(thread_checker_.CalledOnValidThread());

  JNIEnv* env = base::android::AttachCurrentThread();
  DCHECK(!j_external_data_use_observer_.is_null());

  // End time should be greater than start time.
  int64_t start_time_milliseconds = start_time.ToJavaTime();
  int64_t end_time_milliseconds = end_time.ToJavaTime();
  if (start_time_milliseconds >= end_time_milliseconds)
    start_time_milliseconds = end_time_milliseconds - 1;

  Java_ExternalDataUseObserver_reportDataUse(
      env, j_external_data_use_observer_, ConvertUTF8ToJavaString(env, label),
      ConvertUTF8ToJavaString(env, tag), connection_type,
      ConvertUTF8ToJavaString(env, mcc_mnc), start_time_milliseconds,
      end_time_milliseconds, bytes_downloaded, bytes_uploaded);
}

void ExternalDataUseObserverBridge::OnReportDataUseDone(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj,
    bool success) {
  DCHECK(thread_checker_.CalledOnValidThread());

  io_task_runner_->PostTask(
      FROM_HERE, base::Bind(&ExternalDataUseObserver::OnReportDataUseDone,
                            external_data_use_observer_, success));
}

void ExternalDataUseObserverBridge::OnControlAppInstallStateChange(
    JNIEnv* env,
    jobject obj,
    bool is_control_app_installed) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (data_use_tab_model_) {
    data_use_tab_model_->OnControlAppInstallStateChange(
        is_control_app_installed);
  }
}

void ExternalDataUseObserverBridge::ShouldRegisterAsDataUseObserver(
    bool should_register) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!j_external_data_use_observer_.is_null());

  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&ExternalDataUseObserver::ShouldRegisterAsDataUseObserver,
                 external_data_use_observer_, should_register));
}

void ExternalDataUseObserverBridge::RegisterGoogleVariationID(
    bool should_register) {
  DCHECK(thread_checker_.CalledOnValidThread());
  variations::VariationID variation_id = GetGoogleVariationID();

  if (variation_id != variations::EMPTY_ID) {
    // Set variation id for the enabled group if |should_register| is true.
    // Otherwise clear the variation id for the enabled group by setting to
    // EMPTY_ID.
    if (should_register) {
      variations::AssociateGoogleVariationID(
          variations::GOOGLE_WEB_PROPERTIES, kSyntheticFieldTrial,
          kSyntheticFieldTrialEnabledGroup, variation_id);
    }
    ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(
        kSyntheticFieldTrial, should_register
                                  ? kSyntheticFieldTrialEnabledGroup
                                  : kSyntheticFieldTrialDisabledGroup);
  }
}

}  // namespace android
