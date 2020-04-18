// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/google/core/browser/google_url_tracker.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "components/google/core/browser/google_pref_names.h"
#include "components/google/core/browser/google_switches.h"
#include "components/google/core/browser/google_util.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

const char GoogleURLTracker::kDefaultGoogleHomepage[] =
    "https://www.google.com/";
const char GoogleURLTracker::kSearchDomainCheckURL[] =
    "https://www.google.com/searchdomaincheck?format=domain&type=chrome";
const base::Feature GoogleURLTracker::kNoSearchDomainCheck{
    "NoSearchDomainCheck", base::FEATURE_DISABLED_BY_DEFAULT};

GoogleURLTracker::GoogleURLTracker(
    std::unique_ptr<GoogleURLTrackerClient> client,
    Mode mode)
    : client_(std::move(client)),
      google_url_(
          mode == ALWAYS_DOT_COM_MODE
              ? kDefaultGoogleHomepage
              : client_->GetPrefs()->GetString(prefs::kLastKnownGoogleURL)),
      in_startup_sleep_(true),
      already_loaded_(false),
      need_to_load_(false),
      weak_ptr_factory_(this) {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  client_->set_google_url_tracker(this);

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
  if (mode == NORMAL_MODE) {
    static const int kStartLoadDelayMS = 5000;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&GoogleURLTracker::FinishSleep,
                       weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(kStartLoadDelayMS));
  }
}

GoogleURLTracker::~GoogleURLTracker() {
}

// static
void GoogleURLTracker::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(prefs::kLastKnownGoogleURL,
                               GoogleURLTracker::kDefaultGoogleHomepage);
  registry->RegisterStringPref(prefs::kLastPromptedGoogleURL, std::string());
}

void GoogleURLTracker::RequestServerCheck() {
  if (!simple_loader_)
    SetNeedToLoad();
}

std::unique_ptr<GoogleURLTracker::Subscription>
GoogleURLTracker::RegisterCallback(const OnGoogleURLUpdatedCallback& cb) {
  return callback_list_.Add(cb);
}

void GoogleURLTracker::OnURLLoaderComplete(
    std::unique_ptr<std::string> response_body) {
  // Delete the loader.
  simple_loader_.reset();

  // Don't update the URL if the request didn't succeed.
  if (!response_body) {
    already_loaded_ = false;
    return;
  }

  // See if the response data was valid. It should be ".google.<TLD>".
  base::TrimWhitespaceASCII(*response_body, base::TRIM_ALL,
                            response_body.get());
  if (!base::StartsWith(*response_body, ".google.",
                        base::CompareCase::INSENSITIVE_ASCII))
    return;
  GURL url("https://www" + *response_body);
  if (!url.is_valid() || (url.path().length() > 1) || url.has_query() ||
      url.has_ref() ||
      !google_util::IsGoogleDomainUrl(url, google_util::DISALLOW_SUBDOMAIN,
                                      google_util::DISALLOW_NON_STANDARD_PORTS))
    return;

  if (url != google_url_) {
    google_url_ = url;
    client_->GetPrefs()->SetString(prefs::kLastKnownGoogleURL,
                                   google_url_.spec());
    callback_list_.Notify();
  }
}

void GoogleURLTracker::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  // Ignore destructive signals.
  if (type == net::NetworkChangeNotifier::CONNECTION_NONE)
    return;
  already_loaded_ = false;
  StartLoadIfDesirable();
}

void GoogleURLTracker::Shutdown() {
  client_.reset();
  simple_loader_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void GoogleURLTracker::SetNeedToLoad() {
  need_to_load_ = true;
  StartLoadIfDesirable();
}

void GoogleURLTracker::FinishSleep() {
  in_startup_sleep_ = false;
  StartLoadIfDesirable();
}

void GoogleURLTracker::StartLoadIfDesirable() {
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
      net::DefineNetworkTrafficAnnotation("google_url_tracker", R"(
        semantics {
          sender: "Google URL Tracker"
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
      base::BindOnce(&GoogleURLTracker::OnURLLoaderComplete,
                     base::Unretained(this)),
      2 * 1024 /* max_body_size */);
}
