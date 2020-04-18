// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_prefs.h"

#include <memory>

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace data_reduction_proxy {

// Make sure any changes here that have the potential to impact android_webview
// are reflected in RegisterSimpleProfilePrefs.
void RegisterSyncableProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kDataReductionProxyWasEnabledBefore,
                                false);

  registry->RegisterInt64Pref(prefs::kDataReductionProxyLastEnabledTime, 0L);
  registry->RegisterInt64Pref(
      prefs::kDataReductionProxySavingsClearedNegativeSystemClock, 0);

  registry->RegisterBooleanPref(prefs::kDataUsageReportingEnabled, false);

  registry->RegisterInt64Pref(prefs::kHttpReceivedContentLength, 0);
  registry->RegisterInt64Pref(prefs::kHttpOriginalContentLength, 0);

  registry->RegisterListPref(prefs::kDailyHttpOriginalContentLength);
  registry->RegisterInt64Pref(prefs::kDailyHttpOriginalContentLengthApplication,
                              0L);
  registry->RegisterInt64Pref(prefs::kDailyHttpOriginalContentLengthVideo, 0L);
  registry->RegisterInt64Pref(prefs::kDailyHttpOriginalContentLengthUnknown,
                              0L);

  registry->RegisterListPref(prefs::kDailyHttpReceivedContentLength);
  registry->RegisterInt64Pref(prefs::kDailyHttpReceivedContentLengthApplication,
                              0L);
  registry->RegisterInt64Pref(prefs::kDailyHttpReceivedContentLengthVideo, 0L);
  registry->RegisterInt64Pref(prefs::kDailyHttpReceivedContentLengthUnknown,
                              0L);

  registry->RegisterListPref(
      prefs::kDailyOriginalContentLengthWithDataReductionProxyEnabled);
  registry->RegisterInt64Pref(
      prefs::
          kDailyOriginalContentLengthWithDataReductionProxyEnabledApplication,
      0L);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthWithDataReductionProxyEnabledVideo, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthWithDataReductionProxyEnabledUnknown,
      0L);
  registry->RegisterListPref(
      prefs::kDailyContentLengthWithDataReductionProxyEnabled);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthWithDataReductionProxyEnabledApplication, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthWithDataReductionProxyEnabledVideo, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthWithDataReductionProxyEnabledUnknown, 0L);
  registry->RegisterListPref(
      prefs::kDailyContentLengthHttpsWithDataReductionProxyEnabled);
  registry->RegisterListPref(
      prefs::kDailyContentLengthShortBypassWithDataReductionProxyEnabled);
  registry->RegisterListPref(
      prefs::kDailyContentLengthLongBypassWithDataReductionProxyEnabled);
  registry->RegisterListPref(
      prefs::kDailyContentLengthUnknownWithDataReductionProxyEnabled);
  registry->RegisterListPref(
      prefs::kDailyOriginalContentLengthViaDataReductionProxy);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthViaDataReductionProxyApplication, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthViaDataReductionProxyVideo, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthViaDataReductionProxyUnknown, 0L);
  registry->RegisterListPref(prefs::kDailyContentLengthViaDataReductionProxy);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthViaDataReductionProxyApplication, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthViaDataReductionProxyVideo, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthViaDataReductionProxyUnknown, 0L);

  registry->RegisterInt64Pref(prefs::kDailyHttpContentLengthLastUpdateDate, 0L);
  registry->RegisterStringPref(prefs::kDataReductionProxyConfig, std::string());
  registry->RegisterInt64Pref(prefs::kDataReductionProxyLastConfigRetrievalTime,
                              0L);
  registry->RegisterDictionaryPref(prefs::kNetworkProperties);
}

void RegisterSimpleProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(
      prefs::kDataReductionProxyWasEnabledBefore, false);

  registry->RegisterBooleanPref(prefs::kDataUsageReportingEnabled, false);
  RegisterPrefs(registry);
}

// Add any new data reduction proxy prefs to the |pref_map_| or the
// |list_pref_map_| in Init() of DataReductionProxyCompressionStats.
void RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kDataReductionProxy, std::string());
  registry->RegisterInt64Pref(prefs::kDataReductionProxyLastEnabledTime, 0L);
  registry->RegisterInt64Pref(
      prefs::kDataReductionProxySavingsClearedNegativeSystemClock, 0);
  registry->RegisterInt64Pref(prefs::kHttpReceivedContentLength, 0);
  registry->RegisterInt64Pref(
      prefs::kHttpOriginalContentLength, 0);
  registry->RegisterListPref(
      prefs::kDailyHttpOriginalContentLength);
  registry->RegisterInt64Pref(prefs::kDailyHttpOriginalContentLengthApplication,
                              0L);
  registry->RegisterInt64Pref(prefs::kDailyHttpOriginalContentLengthVideo, 0L);
  registry->RegisterInt64Pref(prefs::kDailyHttpOriginalContentLengthUnknown,
                              0L);
  registry->RegisterListPref(prefs::kDailyHttpReceivedContentLength);
  registry->RegisterInt64Pref(prefs::kDailyHttpReceivedContentLengthApplication,
                              0L);
  registry->RegisterInt64Pref(prefs::kDailyHttpReceivedContentLengthVideo, 0L);
  registry->RegisterInt64Pref(prefs::kDailyHttpReceivedContentLengthUnknown,
                              0L);
  registry->RegisterListPref(
      prefs::kDailyOriginalContentLengthWithDataReductionProxyEnabled);
  registry->RegisterInt64Pref(
      prefs::
          kDailyOriginalContentLengthWithDataReductionProxyEnabledApplication,
      0L);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthWithDataReductionProxyEnabledVideo, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthWithDataReductionProxyEnabledUnknown,
      0L);
  registry->RegisterListPref(
      prefs::kDailyContentLengthWithDataReductionProxyEnabled);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthWithDataReductionProxyEnabledApplication, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthWithDataReductionProxyEnabledVideo, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthWithDataReductionProxyEnabledUnknown, 0L);
  registry->RegisterListPref(
      prefs::kDailyContentLengthHttpsWithDataReductionProxyEnabled);
  registry->RegisterListPref(
      prefs::kDailyContentLengthShortBypassWithDataReductionProxyEnabled);
  registry->RegisterListPref(
      prefs::kDailyContentLengthLongBypassWithDataReductionProxyEnabled);
  registry->RegisterListPref(
      prefs::kDailyContentLengthUnknownWithDataReductionProxyEnabled);
  registry->RegisterListPref(
      prefs::kDailyOriginalContentLengthViaDataReductionProxy);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthViaDataReductionProxyApplication, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthViaDataReductionProxyVideo, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyOriginalContentLengthViaDataReductionProxyUnknown, 0L);
  registry->RegisterListPref(prefs::kDailyContentLengthViaDataReductionProxy);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthViaDataReductionProxyApplication, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthViaDataReductionProxyVideo, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyContentLengthViaDataReductionProxyUnknown, 0L);
  registry->RegisterInt64Pref(
      prefs::kDailyHttpContentLengthLastUpdateDate, 0L);
  registry->RegisterStringPref(prefs::kDataReductionProxyConfig, std::string());
  registry->RegisterInt64Pref(prefs::kDataReductionProxyLastConfigRetrievalTime,
                              0L);
  registry->RegisterDictionaryPref(prefs::kNetworkProperties);
}

}  // namespace data_reduction_proxy
