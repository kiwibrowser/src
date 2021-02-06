// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/google/core/browser/search_url_tracker.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/json/json_string_value_serializer.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_value_map.h"
#include "components/search_engines/search_engines_pref_names.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/google/core/browser/google_pref_names.h"
#include "components/google/core/browser/google_switches.h"
#include "components/google/core/browser/google_util.h"
#include "components/version_info/version_info_values.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

#include "base/android/callback_android.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "components/feature_engagement/public/feature_list.h"
#include "components/feature_engagement/public/tracker.h"
#include "base/android/sys_utils.h"
#include "net/base/url_util.h"
#include "base/strings/string_number_conversions.h"

const char SearchURLTracker::kSearchDomainCheckURL[] =
    "https://settings.kiwibrowser.com/search/getrecommendedsearch?format=domain&type=chrome&version=" PRODUCT_VERSION "&release_name=" RELEASE_NAME "&release_version=" RELEASE_VERSION;

SearchURLTracker::SearchURLTracker(
    std::unique_ptr<SearchURLTrackerClient> client,
    Mode mode)
    : client_(std::move(client)),
      search_version_(
          mode == ALWAYS_DOT_COM_MODE
              ? -1
              : client_->GetPrefs()->GetInteger(prefs::kLastKnownSearchVersion)),
      in_startup_sleep_(true),
      already_loaded_(false),
      need_to_load_(false),
      weak_ptr_factory_(this) {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  client_->set_search_url_tracker(this);

  // Because this function can be called during startup, when kicking off a URL
  // load can eat up 20 ms of time, we delay five seconds, which is hopefully
  // long enough to be after startup, but still get results back quickly.
  // Ideally, instead of this timer, we'd do something like "check if the
  // browser is starting up, and if so, come back later", but there is currently
  // no function to do this.
  //
  // In ALWAYS_DOT_COM_MODE we do not nothing at all (but in unit tests
  // /searchdomaincheck lookups might still be issued by calling FinishSleep
  // manually).
  LOG(INFO) << "[Kiwi] List of search engines is initializing";
  if (mode == NORMAL_MODE) {
    LOG(INFO) << "[Kiwi] List of search engines is starting in 7 seconds";
    static const int kStartLoadDelayMS = 7000;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&SearchURLTracker::FinishSleep,
                       weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(kStartLoadDelayMS));
  }
}

SearchURLTracker::~SearchURLTracker() {
}

// static
void SearchURLTracker::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(prefs::kLastKnownSearchVersion, -1);
  registry->RegisterIntegerPref(prefs::kEnableServerSuggestions, -1);
}

void SearchURLTracker::RequestServerCheck() {
  if (!simple_loader_)
    SetNeedToLoad();
}

std::unique_ptr<SearchURLTracker::Subscription>
SearchURLTracker::RegisterCallback(const OnSearchURLUpdatedCallback& cb) {
  return callback_list_.Add(cb);
}

void SearchURLTracker::OnURLLoaderComplete(
    std::unique_ptr<std::string> response_body) {
  int version_code = -1;
  int enable_server_suggestions = -1;

  if (response_body)
    LOG(INFO) << "[Kiwi] List of search engines returned with body";
  else
    LOG(INFO) << "[Kiwi] List of search engines returned without body";
  // Don't update the URL if the request didn't succeed.
  if (!response_body) {
    // Delete the loader.
    simple_loader_.reset();
    already_loaded_ = false;
    return;
  }
  if (simple_loader_->ResponseInfo() && simple_loader_->ResponseInfo()->headers && simple_loader_->ResponseInfo()->headers->HasHeader("se-version-code")) {
    version_code = simple_loader_->ResponseInfo()->headers->GetInt64HeaderValue("se-version-code");
  } else {
    // Delete the loader.
    simple_loader_.reset();
    already_loaded_ = false;
    return;
  }
  if (simple_loader_->ResponseInfo() && simple_loader_->ResponseInfo()->headers && simple_loader_->ResponseInfo()->headers->HasHeader("se-enable-server-suggestions")) {
    enable_server_suggestions = simple_loader_->ResponseInfo()->headers->GetInt64HeaderValue("se-enable-server-suggestions");
    client_->GetPrefs()->SetInteger(prefs::kEnableServerSuggestions, enable_server_suggestions);
  }
  std::string body = *response_body;
  LOG(INFO) << "[Kiwi] version_code: [" << version_code << "], response_body: [" << body.length() << "]";
  if (!base::StartsWith(body, "{",
                        base::CompareCase::INSENSITIVE_ASCII)) {
    LOG(INFO) << "[Kiwi] Received invalid search-engines info with [" << body.length() << "]";
    return;
  }

  if (version_code != -1 && search_version_ != version_code && version_code > 0 && body.length() > 10) {
    search_version_ = version_code;
    LOG(INFO) << "[Kiwi] Received search-engines version: [" << version_code << "] settings from server-side: " << body.length() << " chars";

    std::unique_ptr<base::DictionaryValue> master_dictionary_;

    JSONStringValueDeserializer json(body);
    std::string error;
    std::unique_ptr<base::Value> root(json.Deserialize(NULL, &error));
    if (!root.get()) {
      LOG(ERROR) << "[Kiwi] Failed to parse brandcode prefs file: " << error;
      return;
    }
    if (!root->is_dict()) {
      LOG(ERROR) << "[Kiwi] Failed to parse brandcode prefs file: "
                 << "Root item must be a dictionary.";
      return;
    }
    master_dictionary_.reset(
        static_cast<base::DictionaryValue*>(root.release()));

    const base::ListValue* value = NULL;
    if (master_dictionary_ &&
        master_dictionary_->GetList(prefs::kSearchProviderOverrides, &value) &&
        !value->empty() && value->GetSize() >= 2) {
      LOG(INFO) << "[Kiwi] Search engine list contains " << value->GetSize() << " elements";

      client_->GetPrefs()->ClearPref(prefs::kSearchProviderOverrides);
      client_->GetPrefs()->SetInteger(prefs::kSearchProviderOverridesVersion,
                                     -1);
      client_->GetPrefs()->SetInteger(prefs::kLastKnownSearchVersion,
                                     -1);
      ListPrefUpdate update(client_->GetPrefs(), prefs::kSearchProviderOverrides);
      base::ListValue* list = update.Get();
      bool success = false;
      for (auto it = value->begin(); it != value->end(); ++it) {
        success = true;
        LOG(INFO) << "[Kiwi] Adding to the list one search engine";
        list->Append(it->CreateDeepCopy());
      }

      if (success) {
        LOG(INFO) << "[Kiwi] Search engines processing is a success";
        client_->GetPrefs()->SetInteger(prefs::kSearchProviderOverridesVersion,
                                       version_code);
        client_->GetPrefs()->SetInteger(prefs::kLastKnownSearchVersion,
                                       version_code);
        callback_list_.Notify();
      } else {
        LOG(ERROR) << "[Kiwi] Failure, no search engine found";
        // Delete the loader.
        simple_loader_.reset();
        already_loaded_ = false;
      }
      return ;
    }
    LOG(ERROR) << "[Kiwi] Failed to parse search-engines JSON";
  } else {
    LOG(INFO) << "[Kiwi] Received search-engines [" << version_code << "] settings from server-side: " << body.length() << " chars but we already have it";
  }
}

void SearchURLTracker::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  // Ignore destructive signals.
  if (type == net::NetworkChangeNotifier::CONNECTION_NONE)
    return;
  already_loaded_ = false;
  StartLoadIfDesirable();
}

void SearchURLTracker::Shutdown() {
  client_.reset();
  simple_loader_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void SearchURLTracker::SetNeedToLoad() {
  need_to_load_ = true;
  StartLoadIfDesirable();
}

void SearchURLTracker::FinishSleep() {
  in_startup_sleep_ = false;
  StartLoadIfDesirable();
}

void SearchURLTracker::StartLoadIfDesirable() {
  // Bail if a load isn't appropriate right now.  This function will be called
  // again each time one of the preconditions changes, so we'll load
  // immediately once all of them are met.
  //
  // See comments in header on the class, on RequestServerCheck(), and on the
  // various members here for more detail on exactly what the conditions are.
  if (in_startup_sleep_ || already_loaded_ || !need_to_load_)
    return;

  // Some switches should disable the Google URL tracker entirely.  If we can't
  // do background networking, we can't do the necessary load, and if the user
  // specified a Google base URL manually, we shouldn't bother to look up any
  // alternatives or offer to switch to them.
  if (!client_->IsBackgroundNetworkingEnabled() ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kGoogleBaseURL))
    return;

  already_loaded_ = true;
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("search_url_tracker", R"(
        semantics {
          sender: "Search URL Tracker"
          description:
            "When the user's default search engine is Google, or Google "
            "services are used to resolve navigation errors, the browser needs "
            "to know the ideal origin for requests to Google services. In "
            "these cases the browser makes a request to a global Google "
            "service that returns this origin, potentially taking into account "
            "the user's cookies or IP address."
          trigger: "Browser startup or network change."
          data: "None."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting:
            "To disable this check, users can change the default search engine "
            "to something other than Google, and disable 'Use a web service to "
            "help resolve navigation errors' in Chromium's settings under "
            "Privacy.\nAlternately, running Chromium with "
            "--google-base-url=\"https://www.google.com/\" will disable this, "
            "and force Chromium to use the specified URL for Google service "
            "requests.\nFinally, running Chromium with "
            "--disable-background-networking will disable this, as well as "
            "various other features that make network requests automatically."
           policy_exception_justification:
            "Setting DefaultSearchProviderEnabled Chrome settings policy to "
            "false suffices as a way of setting the default search engine to "
            "not be Google. But there is no policy that controls navigation "
            "error resolution."
        })");
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(kSearchDomainCheckURL);
  long firstInstallDate = base::android::SysUtils::FirstInstallDateFromJni();
  resource_request->url = net::AppendOrReplaceQueryParameter(resource_request->url, "install_date", base::NumberToString(firstInstallDate));
  int searchVersion = client_->GetPrefs()->GetInteger(prefs::kSearchProviderOverridesVersion);
  resource_request->url = net::AppendOrReplaceQueryParameter(resource_request->url, "settings_version", std::to_string(searchVersion));
  std::string referrerString = base::android::SysUtils::ReferrerStringFromJni();
  resource_request->url = net::AppendOrReplaceQueryParameter(resource_request->url, "ref", referrerString);
  LOG(INFO) << "[Kiwi] List of search engines is requesting";
  // country_codes::GetCountryIDFromPrefs(prefs)
  // We don't want this load to set new entries in the cache or cookies, lest
  // we alarm the user.
  resource_request->load_flags =
      (net::LOAD_DISABLE_CACHE | net::LOAD_DO_NOT_SAVE_COOKIES);
  simple_loader_ = network::SimpleURLLoader::Create(std::move(resource_request),
                                                    traffic_annotation);
  // Configure to retry at most kMaxRetries times for 5xx errors and network
  // changes.
  // A network change can propagate through Chrome in various stages, so it's
  // possible for this code to be reached via OnNetworkChanged(), and then have
  // the load we kick off be canceled due to e.g. the DNS server changing at a
  // later time. In general it's not possible to ensure that by the time we
  // reach here any requests we start won't be canceled in this fashion, so
  // retrying is the best we can do.
  static const int kMaxRetries = 5;
  simple_loader_->SetRetryOptions(
      kMaxRetries,
      network::SimpleURLLoader::RetryMode::RETRY_ON_5XX |
          network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  simple_loader_->DownloadToString(
      client_->GetURLLoaderFactory(),
      base::BindOnce(&SearchURLTracker::OnURLLoaderComplete,
                     base::Unretained(this)),
      64 * 1024 /* max_body_size */);
}
