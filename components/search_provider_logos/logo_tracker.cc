// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_provider_logos/logo_tracker.h"

#include <algorithm>
#include <utility>

#include "base/bind_helpers.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/clock.h"
#include "components/data_use_measurement/core/data_use_user_data.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

namespace search_provider_logos {

namespace {

const int64_t kMaxDownloadBytes = 1024 * 1024;

// Returns whether the metadata for the cached logo indicates that the logo is
// OK to show, i.e. it's not expired or it's allowed to be shown temporarily
// after expiration.
bool IsLogoOkToShow(const LogoMetadata& metadata, base::Time now) {
  base::TimeDelta offset =
      base::TimeDelta::FromMilliseconds(kMaxTimeToLiveMS * 3 / 2);
  base::Time distant_past = now - offset;
  // Sanity check so logos aren't accidentally cached forever.
  if (metadata.expiration_time < distant_past) {
    return false;
  }
  return metadata.can_show_after_expiration || metadata.expiration_time >= now;
}

// Reads the logo from the cache and returns it. Returns NULL if the cache is
// empty, corrupt, expired, or doesn't apply to the current logo URL.
std::unique_ptr<EncodedLogo> GetLogoFromCacheOnFileThread(LogoCache* logo_cache,
                                                          const GURL& logo_url,
                                                          base::Time now) {
  const LogoMetadata* metadata = logo_cache->GetCachedLogoMetadata();
  if (!metadata)
    return nullptr;

  if (metadata->source_url != logo_url || !IsLogoOkToShow(*metadata, now)) {
    logo_cache->SetCachedLogo(nullptr);
    return nullptr;
  }

  return logo_cache->GetCachedLogo();
}

void NotifyAndClear(std::vector<EncodedLogoCallback>* encoded_callbacks,
                    std::vector<LogoCallback>* decoded_callbacks,
                    LogoCallbackReason type,
                    const EncodedLogo* encoded_logo,
                    const Logo* decoded_logo) {
  auto opt_encoded_logo =
      encoded_logo ? base::Optional<EncodedLogo>(*encoded_logo) : base::nullopt;
  for (EncodedLogoCallback& callback : *encoded_callbacks) {
    std::move(callback).Run(type, opt_encoded_logo);
  }
  encoded_callbacks->clear();

  auto opt_decoded_logo =
      decoded_logo ? base::Optional<Logo>(*decoded_logo) : base::nullopt;
  for (LogoCallback& callback : *decoded_callbacks) {
    std::move(callback).Run(type, opt_decoded_logo);
  }
  decoded_callbacks->clear();
}

}  // namespace

LogoTracker::LogoTracker(
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    std::unique_ptr<LogoDelegate> delegate,
    std::unique_ptr<LogoCache> logo_cache,
    base::Clock* clock)
    : is_idle_(true),
      is_cached_logo_valid_(false),
      logo_delegate_(std::move(delegate)),
      cache_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
      logo_cache_(logo_cache.release(),
                  base::OnTaskRunnerDeleter(cache_task_runner_)),
      clock_(clock),
      request_context_getter_(request_context_getter),
      weak_ptr_factory_(this) {}

LogoTracker::~LogoTracker() {
  ReturnToIdle(kDownloadOutcomeNotTracked);
}

void LogoTracker::SetServerAPI(
    const GURL& logo_url,
    const ParseLogoResponse& parse_logo_response_func,
    const AppendQueryparamsToLogoURL& append_queryparams_func) {
  if (logo_url == logo_url_)
    return;

  ReturnToIdle(kDownloadOutcomeNotTracked);

  logo_url_ = logo_url;
  parse_logo_response_func_ = parse_logo_response_func;
  append_queryparams_func_ = append_queryparams_func;
}

void LogoTracker::GetLogo(LogoCallbacks callbacks) {
  DCHECK(!logo_url_.is_empty());
  DCHECK(callbacks.on_cached_decoded_logo_available ||
         callbacks.on_cached_encoded_logo_available ||
         callbacks.on_fresh_decoded_logo_available ||
         callbacks.on_fresh_encoded_logo_available);

  if (callbacks.on_cached_encoded_logo_available) {
    on_cached_encoded_logo_.push_back(
        std::move(callbacks.on_cached_encoded_logo_available));
  }
  if (callbacks.on_cached_decoded_logo_available) {
    on_cached_decoded_logo_.push_back(
        std::move(callbacks.on_cached_decoded_logo_available));
  }
  if (callbacks.on_fresh_encoded_logo_available) {
    on_fresh_encoded_logo_.push_back(
        std::move(callbacks.on_fresh_encoded_logo_available));
  }
  if (callbacks.on_fresh_decoded_logo_available) {
    on_fresh_decoded_logo_.push_back(
        std::move(callbacks.on_fresh_decoded_logo_available));
  }

  if (is_idle_) {
    is_idle_ = false;
    base::PostTaskAndReplyWithResult(
        cache_task_runner_.get(), FROM_HERE,
        base::Bind(&GetLogoFromCacheOnFileThread,
                   base::Unretained(logo_cache_.get()), logo_url_,
                   clock_->Now()),
        base::Bind(&LogoTracker::OnCachedLogoRead,
                   weak_ptr_factory_.GetWeakPtr()));
  } else if (is_cached_logo_valid_) {
    NotifyAndClear(&on_cached_encoded_logo_, &on_cached_decoded_logo_,
                   LogoCallbackReason::DETERMINED, cached_encoded_logo_.get(),
                   cached_logo_.get());
  }
}

void LogoTracker::ClearCachedLogo() {
  // First cancel any fetch that might be ongoing.
  ReturnToIdle(kDownloadOutcomeNotTracked);
  // Then clear any cached logo.
  SetCachedLogo(nullptr);
}

void LogoTracker::ReturnToIdle(int outcome) {
  if (outcome != kDownloadOutcomeNotTracked) {
    UMA_HISTOGRAM_ENUMERATION("NewTabPage.LogoDownloadOutcome",
                              static_cast<LogoDownloadOutcome>(outcome),
                              DOWNLOAD_OUTCOME_COUNT);
  }
  // Cancel the current asynchronous operation, if any.
  fetcher_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();

  // Reset state.
  is_idle_ = true;
  cached_logo_.reset();
  cached_encoded_logo_.reset();
  is_cached_logo_valid_ = false;

  // Clear callbacks.
  NotifyAndClear(&on_cached_encoded_logo_, &on_cached_decoded_logo_,
                 LogoCallbackReason::CANCELED, nullptr, nullptr);
  NotifyAndClear(&on_fresh_encoded_logo_, &on_fresh_decoded_logo_,
                 LogoCallbackReason::CANCELED, nullptr, nullptr);
}

void LogoTracker::OnCachedLogoRead(std::unique_ptr<EncodedLogo> cached_logo) {
  DCHECK(!is_idle_);

  if (cached_logo) {
    // Store the value of logo->encoded_image for use below. This ensures that
    // logo->encoded_image is evaulated before base::Passed(&logo), which sets
    // logo to NULL.
    scoped_refptr<base::RefCountedString> encoded_image =
        cached_logo->encoded_image;
    logo_delegate_->DecodeUntrustedImage(
        encoded_image,
        base::Bind(&LogoTracker::OnCachedLogoAvailable,
                   weak_ptr_factory_.GetWeakPtr(), base::Passed(&cached_logo)));
  } else {
    OnCachedLogoAvailable({}, SkBitmap());
  }
}

void LogoTracker::OnCachedLogoAvailable(
    std::unique_ptr<EncodedLogo> encoded_logo,
    const SkBitmap& image) {
  DCHECK(!is_idle_);

  if (!image.isNull()) {
    cached_logo_.reset(new Logo());
    cached_logo_->metadata = encoded_logo->metadata;
    cached_logo_->image = image;
    cached_encoded_logo_ = std::move(encoded_logo);
  }
  is_cached_logo_valid_ = true;
  NotifyAndClear(&on_cached_encoded_logo_, &on_cached_decoded_logo_,
                 LogoCallbackReason::DETERMINED, cached_encoded_logo_.get(),
                 cached_logo_.get());
  FetchLogo();
}

void LogoTracker::SetCachedLogo(std::unique_ptr<EncodedLogo> logo) {
  cache_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&LogoCache::SetCachedLogo, base::Unretained(logo_cache_.get()),
                 base::Owned(logo.release())));
}

void LogoTracker::SetCachedMetadata(const LogoMetadata& metadata) {
  cache_task_runner_->PostTask(
      FROM_HERE, base::Bind(&LogoCache::UpdateCachedLogoMetadata,
                            base::Unretained(logo_cache_.get()), metadata));
}

void LogoTracker::FetchLogo() {
  DCHECK(!fetcher_);
  DCHECK(!is_idle_);

  std::string fingerprint;
  if (cached_logo_ && !cached_logo_->metadata.fingerprint.empty() &&
      cached_logo_->metadata.expiration_time >= clock_->Now()) {
    fingerprint = cached_logo_->metadata.fingerprint;
  }
  GURL url = append_queryparams_func_.Run(logo_url_, fingerprint);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("logo_tracker", R"(
        semantics {
          sender: "Logo Tracker"
          description:
            "Provides the logo image (aka Doodle) if Google is your configured "
            "search provider."
          trigger: "Displaying the new tab page on iOS or Android."
          data:
            "Logo ID, and the user's Google cookies to show for example "
            "birthday doodles at appropriate times."
          destination: OTHER
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting:
            "Choosing a non-Google search engine in Chromium settings under "
            "'Search Engine' will disable this feature."
          policy_exception_justification:
            "Not implemented, considered not useful as it does not upload any"
            "data and just downloads a logo image."
        })");
  fetcher_ = net::URLFetcher::Create(url, net::URLFetcher::GET, this,
                                     traffic_annotation);
  fetcher_->SetRequestContext(request_context_getter_.get());
  data_use_measurement::DataUseUserData::AttachToFetcher(
      fetcher_.get(),
      data_use_measurement::DataUseUserData::SEARCH_PROVIDER_LOGOS);
  fetcher_->Start();
  logo_download_start_time_ = base::TimeTicks::Now();
}

void LogoTracker::OnFreshLogoParsed(bool* parsing_failed,
                                    bool from_http_cache,
                                    std::unique_ptr<EncodedLogo> logo) {
  DCHECK(!is_idle_);

  if (logo)
    logo->metadata.source_url = logo_url_;

  if (!logo || !logo->encoded_image) {
    OnFreshLogoAvailable(std::move(logo), /*download_failed=*/false,
                         *parsing_failed, from_http_cache, SkBitmap());
  } else {
    // Store the value of logo->encoded_image for use below. This ensures that
    // logo->encoded_image is evaulated before base::Passed(&logo), which sets
    // logo to NULL.
    scoped_refptr<base::RefCountedString> encoded_image = logo->encoded_image;
    logo_delegate_->DecodeUntrustedImage(
        encoded_image,
        base::Bind(&LogoTracker::OnFreshLogoAvailable,
                   weak_ptr_factory_.GetWeakPtr(), base::Passed(&logo),
                   /*download_failed=*/false, *parsing_failed,
                   from_http_cache));
  }
}

void LogoTracker::OnFreshLogoAvailable(
    std::unique_ptr<EncodedLogo> encoded_logo,
    bool download_failed,
    bool parsing_failed,
    bool from_http_cache,
    const SkBitmap& image) {
  DCHECK(!is_idle_);

  LogoDownloadOutcome download_outcome = DOWNLOAD_OUTCOME_COUNT;
  std::unique_ptr<Logo> logo;

  if (download_failed) {
    download_outcome = DOWNLOAD_OUTCOME_DOWNLOAD_FAILED;
  } else if (encoded_logo && !encoded_logo->encoded_image && cached_logo_ &&
             !encoded_logo->metadata.fingerprint.empty() &&
             encoded_logo->metadata.fingerprint ==
                 cached_logo_->metadata.fingerprint) {
    // The cached logo was revalidated, i.e. its fingerprint was verified.
    // mime_type isn't sent when revalidating, so copy it from the cached logo.
    encoded_logo->metadata.mime_type = cached_logo_->metadata.mime_type;
    SetCachedMetadata(encoded_logo->metadata);
    download_outcome = DOWNLOAD_OUTCOME_LOGO_REVALIDATED;
  } else if (encoded_logo && image.isNull()) {
    // Image decoding failed. Do nothing.
    download_outcome = DOWNLOAD_OUTCOME_DECODING_FAILED;
  } else {
    // Check if the server returned a valid, non-empty response.
    if (encoded_logo) {
      UMA_HISTOGRAM_BOOLEAN("NewTabPage.LogoImageDownloaded", from_http_cache);

      DCHECK(!image.isNull());
      logo.reset(new Logo());
      logo->metadata = encoded_logo->metadata;
      logo->image = image;
    }

    if (logo) {
      download_outcome = DOWNLOAD_OUTCOME_NEW_LOGO_SUCCESS;
    } else {
      if (parsing_failed)
        download_outcome = DOWNLOAD_OUTCOME_PARSING_FAILED;
      else
        download_outcome = DOWNLOAD_OUTCOME_NO_LOGO_TODAY;
    }
  }

  LogoCallbackReason callback_type = LogoCallbackReason::FAILED;
  switch (download_outcome) {
    case DOWNLOAD_OUTCOME_NEW_LOGO_SUCCESS:
      DCHECK(encoded_logo);
      DCHECK(logo);
      callback_type = LogoCallbackReason::DETERMINED;
      break;

    case DOWNLOAD_OUTCOME_PARSING_FAILED:
    case DOWNLOAD_OUTCOME_NO_LOGO_TODAY:
      // Clear the cached logo if it was non-null. Otherwise, report this as a
      // revalidation of "no logo".
      DCHECK(!encoded_logo);
      DCHECK(!logo);
      if (cached_logo_) {
        callback_type = LogoCallbackReason::DETERMINED;
      } else {
        callback_type = LogoCallbackReason::REVALIDATED;
      }
      break;

    case DOWNLOAD_OUTCOME_DOWNLOAD_FAILED:
      // In the download failed, don't notify the callback at all, since the
      // callback should continue to use the cached logo.
      DCHECK(!encoded_logo);
      DCHECK(!logo);
      callback_type = LogoCallbackReason::FAILED;
      break;

    case DOWNLOAD_OUTCOME_DECODING_FAILED:
      DCHECK(encoded_logo);
      DCHECK(!logo);
      encoded_logo.reset();
      callback_type = LogoCallbackReason::FAILED;
      break;

    case DOWNLOAD_OUTCOME_LOGO_REVALIDATED:
      // In the server reported that the cached logo is still current, don't
      // notify the callback at all, since the callback should continue to use
      // the cached logo.
      DCHECK(encoded_logo);
      DCHECK(!logo);
      callback_type = LogoCallbackReason::REVALIDATED;
      break;

    case DOWNLOAD_OUTCOME_COUNT:
      NOTREACHED();
      return;
  }

  NotifyAndClear(&on_fresh_encoded_logo_, &on_fresh_decoded_logo_,
                 callback_type, encoded_logo.get(), logo.get());

  switch (callback_type) {
    case LogoCallbackReason::DETERMINED:
      SetCachedLogo(std::move(encoded_logo));
      break;

    default:
      break;
  }

  ReturnToIdle(download_outcome);
}

void LogoTracker::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(!is_idle_);
  std::unique_ptr<net::URLFetcher> cleanup_fetcher(fetcher_.release());

  if (!source->GetStatus().is_success()) {
    OnFreshLogoAvailable({}, /*download_failed=*/true, false, false,
                         SkBitmap());
    return;
  }

  int response_code = source->GetResponseCode();
  if (response_code != net::HTTP_OK &&
      response_code != net::URLFetcher::RESPONSE_CODE_INVALID) {
    // RESPONSE_CODE_INVALID is returned when fetching from a file: URL
    // (for testing). In all other cases we would have had a non-success status.
    OnFreshLogoAvailable({}, /*download_failed=*/true, false, false,
                         SkBitmap());
    return;
  }

  UMA_HISTOGRAM_TIMES("NewTabPage.LogoDownloadTime",
                      base::TimeTicks::Now() - logo_download_start_time_);

  std::unique_ptr<std::string> response(new std::string());
  source->GetResponseAsString(response.get());
  base::Time response_time = clock_->Now();

  bool from_http_cache = source->WasCached();

  bool* parsing_failed = new bool(false);
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(parse_logo_response_func_, std::move(response),
                     response_time, parsing_failed),
      base::BindOnce(&LogoTracker::OnFreshLogoParsed,
                     weak_ptr_factory_.GetWeakPtr(),
                     base::Owned(parsing_failed), from_http_cache));
}

void LogoTracker::OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                             int64_t current,
                                             int64_t total,
                                             int64_t current_network_bytes) {
  if (total > kMaxDownloadBytes || current > kMaxDownloadBytes) {
    LOG(WARNING) << "Search provider logo exceeded download size limit";
    OnFreshLogoAvailable({}, /*download_failed=*/true, false, false,
                         SkBitmap());
  }
}

}  // namespace search_provider_logos
