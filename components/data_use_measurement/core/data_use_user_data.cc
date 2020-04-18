// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_use_measurement/core/data_use_user_data.h"

#include <memory>

#if defined(OS_ANDROID)
#include "base/android/application_status_listener.h"
#endif

#include "net/url_request/url_fetcher.h"

namespace data_use_measurement {

namespace {

DataUseUserData::AppState GetCurrentAppState() {
#if defined(OS_ANDROID)
  return base::android::ApplicationStatusListener::GetState() ==
                 base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES
             ? DataUseUserData::FOREGROUND
             : DataUseUserData::BACKGROUND;
#else
  // If the OS is not Android, all the requests are considered Foreground.
  return DataUseUserData::FOREGROUND;
#endif
}

}  // namespace

DataUseUserData::DataUseUserData(ServiceName service_name, AppState app_state)
    : service_name_(service_name),
      app_state_(app_state),
      content_type_(DataUseContentType::OTHER) {}

DataUseUserData::~DataUseUserData() {}

// static
const void* const DataUseUserData::kUserDataKey =
    &DataUseUserData::kUserDataKey;

// static
std::unique_ptr<base::SupportsUserData::Data> DataUseUserData::Create(
    ServiceName service_name) {
  return std::make_unique<DataUseUserData>(service_name, GetCurrentAppState());
}

// static
std::string DataUseUserData::GetServiceNameAsString(ServiceName service_name) {
  switch (service_name) {
    case SUGGESTIONS:
      return "Suggestions";
    case NOT_TAGGED:
      return "NotTagged";
    case TRANSLATE:
      return "Translate";
    case SYNC:
      return "Sync";
    case OMNIBOX:
      return "Omnibox";
    case INVALIDATION:
      return "Invalidation";
    case RAPPOR:
      return "Rappor";
    case VARIATIONS:
      return "Variations";
    case UMA:
      return "UMA";
    case DOMAIN_RELIABILITY:
      return "DomainReliability";
    case PROFILE_DOWNLOADER:
      return "ProfileDownloader";
    case GOOGLE_URL_TRACKER:
      return "GoogleURLTracker";
    case AUTOFILL:
      return "Autofill";
    case POLICY:
      return "Policy";
    case SPELL_CHECKER:
      return "SpellChecker";
    case NTP_SNIPPETS_OBSOLETE:
      return "NTPSnippetsObsolete";
    case SAFE_BROWSING:
      return "SafeBrowsing";
    case DATA_REDUCTION_PROXY:
      return "DataReductionProxy";
    case PRECACHE:
      return "Precache";
    case NTP_TILES:
      return "NTPTiles";
    case FEEDBACK_UPLOADER:
      return "FeedbackUploader";
    case TRACING_UPLOADER:
      return "TracingUploader";
    case DOM_DISTILLER:
      return "DOMDistiller";
    case CLOUD_PRINT:
      return "CloudPrint";
    case SEARCH_PROVIDER_LOGOS:
      return "SearchProviderLogos";
    case UPDATE_CLIENT:
      return "UpdateClient";
    case GCM_DRIVER:
      return "GCMDriver";
    case WEB_HISTORY_SERVICE:
      return "WebHistoryService";
    case NETWORK_TIME_TRACKER:
      return "NetworkTimeTracker";
    case SUPERVISED_USER:
      return "SupervisedUser";
    case IMAGE_FETCHER_UNTAGGED:
      return "ImageFetcherUntagged";
    case GAIA:
      return "GAIA";
    case CAPTIVE_PORTAL:
      return "CaptivePortal";
    case WEB_RESOURCE_SERVICE:
      return "WebResourceService";
    case SIGNIN:
      return "Signin";
    case NTP_SNIPPETS_SUGGESTIONS:
      return "NTPSnippetsSuggestions";
    case NTP_SNIPPETS_THUMBNAILS:
      return "NTPSnippetsThumbnails";
    case DOODLE:
      return "Doodle";
    case UKM:
      return "UKM";
    case PAYMENTS:
      return "Payments";
    case LARGE_ICON_SERVICE:
      return "LargeIconService";
    case MACHINE_INTELLIGENCE:
      return "MachineIntelligence";
  }
  return "INVALID";
}

// static
void DataUseUserData::AttachToFetcher(net::URLFetcher* fetcher,
                                      ServiceName service_name) {
  fetcher->SetURLRequestUserData(kUserDataKey,
                                 base::Bind(&Create, service_name));
}

}  // namespace data_use_measurement
