// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/metrics/uma_session_stats.h"

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_service.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/prefs/pref_service.h"
#include "components/ukm/ukm_service.h"
#include "components/variations/hashing.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/browser_thread.h"
#include "jni/UmaSessionStats_jni.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::JavaParamRef;
using base::UserMetricsAction;

namespace {
UmaSessionStats* g_uma_session_stats = NULL;

// Used to keep the state of whether we should consider metric consent enabled.
// This is used/read only within the ChromeMetricsServiceAccessor methods.
bool g_metrics_consent_for_testing = false;
}  // namespace

UmaSessionStats::UmaSessionStats()
    : active_session_count_(0) {
}

UmaSessionStats::~UmaSessionStats() {
}

void UmaSessionStats::UmaResumeSession(JNIEnv* env,
                                       const JavaParamRef<jobject>& obj) {
  DCHECK(g_browser_process);

  if (active_session_count_ == 0) {
    session_start_time_ = base::TimeTicks::Now();

    // Tell the metrics services that the application resumes.
    metrics::MetricsService* metrics = g_browser_process->metrics_service();
    if (metrics)
      metrics->OnAppEnterForeground();
    ukm::UkmService* ukm_service =
        g_browser_process->GetMetricsServicesManager()->GetUkmService();
    if (ukm_service)
      ukm_service->OnAppEnterForeground();
  }
  ++active_session_count_;
}

void UmaSessionStats::UmaEndSession(JNIEnv* env,
                                    const JavaParamRef<jobject>& obj) {
  --active_session_count_;
  DCHECK_GE(active_session_count_, 0);

  if (active_session_count_ == 0) {
    base::TimeDelta duration = base::TimeTicks::Now() - session_start_time_;

    // Note: This metric is recorded separately on desktop in
    // DesktopSessionDurationTracker::EndSession.
    UMA_HISTOGRAM_LONG_TIMES("Session.TotalDuration", duration);

    DCHECK(g_browser_process);
    // Tell the metrics services they were cleanly shutdown.
    metrics::MetricsService* metrics = g_browser_process->metrics_service();
    if (metrics)
      metrics->OnAppEnterBackground();
    ukm::UkmService* ukm_service =
        g_browser_process->GetMetricsServicesManager()->GetUkmService();
    if (ukm_service)
      ukm_service->OnAppEnterBackground();
  }
}

// static
void UmaSessionStats::RegisterSyntheticFieldTrial(
    const std::string& trial_name,
    const std::string& group_name) {
  ChromeMetricsServiceAccessor::RegisterSyntheticFieldTrial(trial_name,
                                                            group_name);
}

// static
void UmaSessionStats::RegisterSyntheticMultiGroupFieldTrial(
    const std::string& trial_name,
    const std::vector<uint32_t>& group_name_hashes) {
  ChromeMetricsServiceAccessor::RegisterSyntheticMultiGroupFieldTrial(
      trial_name, group_name_hashes);
}

// Updates metrics reporting state managed by native code. This should only be
// called when consent is changing, and UpdateMetricsServiceState() should be
// called immediately after for metrics services to be started or stopped as
// needed. This is enforced by UmaSessionStats.changeMetricsReportingConsent on
// the Java side.
static void JNI_UmaSessionStats_ChangeMetricsReportingConsent(
    JNIEnv*,
    const JavaParamRef<jclass>&,
    jboolean consent) {
  UpdateMetricsPrefsOnPermissionChange(consent);

  // This function ensures a consent file in the data directory is either
  // created, or deleted, depending on consent. Starting up metrics services
  // will ensure that the consent file contains the ClientID. The ID is passed
  // to the renderer for crash reporting when things go wrong.
  GoogleUpdateSettings::CollectStatsConsentTaskRunner()->PostTask(
      FROM_HERE, base::Bind(base::IgnoreResult(
                                GoogleUpdateSettings::SetCollectStatsConsent),
                            consent));
}

// Initialize the local consent bool variable to false. Used only for testing.
static void JNI_UmaSessionStats_InitMetricsAndCrashReportingForTesting(
    JNIEnv*,
    const JavaParamRef<jclass>&) {
  DCHECK(g_browser_process);

  g_metrics_consent_for_testing = false;
  ChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(
      &g_metrics_consent_for_testing);
}

// Clears the boolean consent pointer for ChromeMetricsServiceAccessor to
// original setting. Used only for testing.
static void JNI_UmaSessionStats_UnsetMetricsAndCrashReportingForTesting(
    JNIEnv*,
    const JavaParamRef<jclass>&) {
  DCHECK(g_browser_process);

  g_metrics_consent_for_testing = false;
  ChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(nullptr);
}

// Updates the metrics consent bit to |consent|. This is separate from
// InitMetricsAndCrashReportingForTesting as the Set isn't meant to be used
// repeatedly. Used only for testing.
static void JNI_UmaSessionStats_UpdateMetricsAndCrashReportingForTesting(
    JNIEnv*,
    const JavaParamRef<jclass>&,
    jboolean consent) {
  DCHECK(g_browser_process);

  g_metrics_consent_for_testing = consent;
  g_browser_process->GetMetricsServicesManager()->UpdateUploadPermissions(true);
}

// Starts/stops the MetricsService based on existing consent and upload
// preferences.
// There are three possible states:
// * Logs are being recorded and being uploaded to the server.
// * Logs are being recorded, but not being uploaded to the server.
//   This happens when we've got permission to upload on Wi-Fi but we're on a
//   mobile connection (for example).
// * Logs are neither being recorded or uploaded.
// If logs aren't being recorded, then |may_upload| is ignored.
//
// This can be called at any time when consent hasn't changed, such as
// connection type change, or start up. If consent has changed, then
// ChangeMetricsReportingConsent() should be called first.
static void JNI_UmaSessionStats_UpdateMetricsServiceState(
    JNIEnv*,
    const JavaParamRef<jclass>&,
    jboolean may_upload) {
  // This will also apply the consent state, taken from Chrome Local State
  // prefs.
  g_browser_process->GetMetricsServicesManager()->UpdateUploadPermissions(
      may_upload);
}

static void JNI_UmaSessionStats_RegisterExternalExperiment(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& jtrial_name,
    const JavaParamRef<jintArray>& jexperiment_ids) {
  const std::string trial_name_utf8(ConvertJavaStringToUTF8(env, jtrial_name));
  std::vector<int> experiment_ids;
  // A null |jexperiment_ids| is the same as an empty list.
  if (jexperiment_ids) {
    base::android::JavaIntArrayToIntVector(env, jexperiment_ids,
                                           &experiment_ids);
  }

  UMA_HISTOGRAM_COUNTS_100("UMA.ExternalExperiment.GroupCount",
                           experiment_ids.size());

  std::vector<uint32_t> group_name_hashes;
  group_name_hashes.reserve(experiment_ids.size());

  variations::ActiveGroupId active_group;
  active_group.name = variations::HashName(trial_name_utf8);
  for (int experiment_id : experiment_ids) {
    active_group.group = variations::HashName(base::IntToString(experiment_id));
    // Since external experiments are not based on Chrome's low entropy source,
    // they are only sent to Google web properties for signed in users to make
    // sure that this couldn't be used to identify a user that's not signed in.
    variations::AssociateGoogleVariationIDForceHashes(
        variations::GOOGLE_WEB_PROPERTIES_SIGNED_IN, active_group,
        static_cast<variations::VariationID>(experiment_id));
    group_name_hashes.push_back(active_group.group);
  }

  UmaSessionStats::RegisterSyntheticMultiGroupFieldTrial(trial_name_utf8,
                                                         group_name_hashes);
}

static void JNI_UmaSessionStats_RegisterSyntheticFieldTrial(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& jtrial_name,
    const JavaParamRef<jstring>& jgroup_name) {
  std::string trial_name(ConvertJavaStringToUTF8(env, jtrial_name));
  std::string group_name(ConvertJavaStringToUTF8(env, jgroup_name));
  UmaSessionStats::RegisterSyntheticFieldTrial(trial_name, group_name);
}

static void JNI_UmaSessionStats_RecordMultiWindowSession(
    JNIEnv*,
    const JavaParamRef<jclass>&,
    jint area_percent,
    jint instance_count) {
  UMA_HISTOGRAM_PERCENTAGE("MobileStartup.MobileMultiWindowSession",
                           area_percent);
  // Make sure the bucket count is the same as the range.  This currently
  // expects no more than 10 simultaneous multi window instances.
  UMA_HISTOGRAM_CUSTOM_COUNTS("MobileStartup.MobileMultiWindowInstances",
                              instance_count,
                              1 /* min */,
                              10 /* max */,
                              10 /* bucket count */);
}

static void JNI_UmaSessionStats_RecordTabCountPerLoad(
    JNIEnv*,
    const JavaParamRef<jclass>&,
    jint num_tabs) {
  // Record how many tabs total are open.
  UMA_HISTOGRAM_CUSTOM_COUNTS("Tabs.TabCountPerLoad", num_tabs, 1, 200, 50);
}

static void JNI_UmaSessionStats_RecordPageLoaded(
    JNIEnv*,
    const JavaParamRef<jclass>&,
    jboolean is_desktop_user_agent) {
  // Should be called whenever a page has been loaded.
  base::RecordAction(UserMetricsAction("MobilePageLoaded"));
  if (is_desktop_user_agent) {
    base::RecordAction(UserMetricsAction("MobilePageLoadedDesktopUserAgent"));
  }
}

static void JNI_UmaSessionStats_RecordPageLoadedWithKeyboard(
    JNIEnv*,
    const JavaParamRef<jclass>&) {
  base::RecordAction(UserMetricsAction("MobilePageLoadedWithKeyboard"));
}

static jlong JNI_UmaSessionStats_Init(JNIEnv* env,
                                      const JavaParamRef<jclass>& obj) {
  // We should have only one UmaSessionStats instance.
  DCHECK(!g_uma_session_stats);
  g_uma_session_stats = new UmaSessionStats();
  return reinterpret_cast<intptr_t>(g_uma_session_stats);
}
