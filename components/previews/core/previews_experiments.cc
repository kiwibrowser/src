// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/previews/core/previews_experiments.h"

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/optional.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "components/previews/core/previews_features.h"

namespace previews {

namespace {

// The group of client-side previews experiments. This controls paramters of the
// client side blacklist.
const char kClientSidePreviewsFieldTrial[] = "ClientSidePreviews";

// Name for the version parameter of a field trial. Version changes will
// result in older blacklist entries being removed.
const char kVersion[] = "version";

// The threshold of EffectiveConnectionType above which previews will not be
// served.
// See net/nqe/effective_connection_type.h for mapping from string to value.
const char kEffectiveConnectionTypeThreshold[] =
    "max_allowed_effective_connection_type";

// Inflation parameters for estimating NoScript data savings.
const char kNoScriptInflationPercent[] = "NoScriptInflationPercent";
const char kNoScriptInflationBytes[] = "NoScriptInflationBytes";

size_t GetParamValueAsSizeT(const std::string& trial_name,
                            const std::string& param_name,
                            size_t default_value) {
  size_t value;
  if (!base::StringToSizeT(
          base::GetFieldTrialParamValue(trial_name, param_name), &value)) {
    return default_value;
  }
  return value;
}

int GetParamValueAsInt(const std::string& trial_name,
                       const std::string& param_name,
                       int default_value) {
  int value;
  if (!base::StringToInt(base::GetFieldTrialParamValue(trial_name, param_name),
                         &value)) {
    return default_value;
  }
  return value;
}

net::EffectiveConnectionType GetParamValueAsECT(
    const std::string& trial_name,
    const std::string& param_name,
    net::EffectiveConnectionType default_value) {
  return net::GetEffectiveConnectionTypeForName(
             base::GetFieldTrialParamValue(trial_name, param_name))
      .value_or(default_value);
}

net::EffectiveConnectionType GetParamValueAsECTByFeature(
    const base::Feature& feature,
    const std::string& param_name,
    net::EffectiveConnectionType default_value) {
  return net::GetEffectiveConnectionTypeForName(
             base::GetFieldTrialParamValueByFeature(feature, param_name))
      .value_or(default_value);
}

}  // namespace

namespace params {

size_t MaxStoredHistoryLengthForPerHostBlackList() {
  return GetParamValueAsSizeT(kClientSidePreviewsFieldTrial,
                              "per_host_max_stored_history_length", 4);
}

size_t MaxStoredHistoryLengthForHostIndifferentBlackList() {
  return GetParamValueAsSizeT(kClientSidePreviewsFieldTrial,
                              "host_indifferent_max_stored_history_length", 10);
}

size_t MaxInMemoryHostsInBlackList() {
  return GetParamValueAsSizeT(kClientSidePreviewsFieldTrial,
                              "max_hosts_in_blacklist", 100);
}

int PerHostBlackListOptOutThreshold() {
  return GetParamValueAsInt(kClientSidePreviewsFieldTrial,
                            "per_host_opt_out_threshold", 2);
}

int HostIndifferentBlackListOptOutThreshold() {
  return GetParamValueAsInt(kClientSidePreviewsFieldTrial,
                            "host_indifferent_opt_out_threshold", 6);
}

base::TimeDelta PerHostBlackListDuration() {
  return base::TimeDelta::FromDays(
      GetParamValueAsInt(kClientSidePreviewsFieldTrial,
                         "per_host_black_list_duration_in_days", 30));
}

base::TimeDelta HostIndifferentBlackListPerHostDuration() {
  return base::TimeDelta::FromDays(
      GetParamValueAsInt(kClientSidePreviewsFieldTrial,
                         "host_indifferent_black_list_duration_in_days", 30));
}

base::TimeDelta SingleOptOutDuration() {
  return base::TimeDelta::FromSeconds(
      GetParamValueAsInt(kClientSidePreviewsFieldTrial,
                         "single_opt_out_duration_in_seconds", 60 * 5));
}

base::TimeDelta OfflinePreviewFreshnessDuration() {
  return base::TimeDelta::FromDays(
      GetParamValueAsInt(kClientSidePreviewsFieldTrial,
                         "offline_preview_freshness_duration_in_days", 7));
}

net::EffectiveConnectionType GetECTThresholdForPreview(
    previews::PreviewsType type) {
  switch (type) {
    case PreviewsType::OFFLINE:
    case PreviewsType::NOSCRIPT:
      return GetParamValueAsECT(kClientSidePreviewsFieldTrial,
                                kEffectiveConnectionTypeThreshold,
                                net::EFFECTIVE_CONNECTION_TYPE_2G);
    case PreviewsType::LOFI:
      return GetParamValueAsECTByFeature(features::kClientLoFi,
                                         kEffectiveConnectionTypeThreshold,
                                         net::EFFECTIVE_CONNECTION_TYPE_2G);
    case PreviewsType::LITE_PAGE:
      NOTREACHED();
      break;
    case PreviewsType::AMP_REDIRECTION:
      return net::EFFECTIVE_CONNECTION_TYPE_LAST;  // Trigger irrespective of
                                                   // ECT.
    case PreviewsType::NONE:
    case PreviewsType::UNSPECIFIED:
    case PreviewsType::LAST:
      break;
  }
  NOTREACHED();
  return net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN;
}

bool ArePreviewsAllowed() {
  return base::FeatureList::IsEnabled(features::kPreviews);
}

bool IsOfflinePreviewsEnabled() {
  return base::FeatureList::IsEnabled(features::kOfflinePreviews);
}

bool IsClientLoFiEnabled() {
  return base::FeatureList::IsEnabled(features::kClientLoFi);
}

bool IsAMPRedirectionPreviewEnabled() {
  return base::FeatureList::IsEnabled(features::kAMPRedirection);
}

bool IsNoScriptPreviewsEnabled() {
  return base::FeatureList::IsEnabled(features::kNoScriptPreviews);
}

int OfflinePreviewsVersion() {
  return GetParamValueAsInt(kClientSidePreviewsFieldTrial, kVersion, 0);
}

int ClientLoFiVersion() {
  return base::GetFieldTrialParamByFeatureAsInt(features::kClientLoFi, kVersion,
                                                0);
}

int AMPRedirectionPreviewsVersion() {
  return GetFieldTrialParamByFeatureAsInt(features::kAMPRedirection, kVersion,
                                          0);
}

int NoScriptPreviewsVersion() {
  return GetFieldTrialParamByFeatureAsInt(features::kNoScriptPreviews, kVersion,
                                          0);
}

bool IsOptimizationHintsEnabled() {
  return base::FeatureList::IsEnabled(features::kOptimizationHints);
}

net::EffectiveConnectionType EffectiveConnectionTypeThresholdForClientLoFi() {
  return GetParamValueAsECTByFeature(features::kClientLoFi,
                                     kEffectiveConnectionTypeThreshold,
                                     net::EFFECTIVE_CONNECTION_TYPE_2G);
}

std::vector<std::string> GetBlackListedHostsForClientLoFiFieldTrial() {
  return base::SplitString(base::GetFieldTrialParamValueByFeature(
                               features::kClientLoFi, "short_host_blacklist"),
                           ",", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

int NoScriptPreviewsInflationPercent() {
  // The default value was determined from lab experiment data of whitelisted
  // URLs. It may be improved once there is enough UKM live experiment data
  // via the field trial param.
  return GetFieldTrialParamByFeatureAsInt(features::kNoScriptPreviews,
                                          kNoScriptInflationPercent, 80);
}

int NoScriptPreviewsInflationBytes() {
  return GetFieldTrialParamByFeatureAsInt(features::kNoScriptPreviews,
                                          kNoScriptInflationBytes, 0);
}

}  // namespace params

std::string GetStringNameForType(PreviewsType type) {
  // The returned string is used to record histograms for the new preview type.
  // Also add the string to Previews.Types histogram suffix in histograms.xml.
  switch (type) {
    case PreviewsType::NONE:
      return "None";
    case PreviewsType::OFFLINE:
      return "Offline";
    case PreviewsType::LOFI:
      return "LoFi";
    case PreviewsType::LITE_PAGE:
      return "LitePage";
    case PreviewsType::AMP_REDIRECTION:
      return "AMPRedirection";
    case PreviewsType::NOSCRIPT:
      return "NoScript";
    case PreviewsType::UNSPECIFIED:
      return "Unspecified";
    case PreviewsType::LAST:
      break;
  }
  NOTREACHED();
  return std::string();
}

}  // namespace previews
