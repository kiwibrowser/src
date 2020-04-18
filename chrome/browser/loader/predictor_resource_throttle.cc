// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/loader/predictor_resource_throttle.h"

#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/net/predictor.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace {

bool IsNavigationRequest(net::URLRequest* request) {
  content::ResourceType resource_type =
      content::ResourceRequestInfo::ForRequest(request)->GetResourceType();
  return resource_type == content::RESOURCE_TYPE_MAIN_FRAME ||
         resource_type == content::RESOURCE_TYPE_SUB_FRAME;
}

}  // namespace

PredictorResourceThrottle::PredictorResourceThrottle(
    net::URLRequest* request,
    chrome_browser_net::Predictor* predictor)
    : request_(request), predictor_(predictor), weak_factory_(this) {}

PredictorResourceThrottle::~PredictorResourceThrottle() {}

// static
std::unique_ptr<PredictorResourceThrottle>
PredictorResourceThrottle::MaybeCreate(net::URLRequest* request,
                                       ProfileIOData* io_data) {
  if (io_data->GetPredictor()) {
    return base::WrapUnique(
        new PredictorResourceThrottle(request, io_data->GetPredictor()));
  }
  return nullptr;
}

void PredictorResourceThrottle::WillStartRequest(bool* defer) {
  GURL request_scheme_host(
      chrome_browser_net::Predictor::CanonicalizeUrl(request_->url()));
  if (request_scheme_host.is_empty())
    return;

  // Learn what URLs are likely to be needed during next startup.
  // TODO(csharrison): Rename this method, as this code path is used for more
  // than just navigation requests.
  predictor_->LearnAboutInitialNavigation(request_scheme_host);

  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request_);
  DCHECK(info);
  content::ResourceType resource_type = info->GetResourceType();
  const GURL& referring_scheme_host =
      GURL(request_->referrer()).GetWithEmptyPath();

  // Learn about our referring URL, for use in the future. Only learn
  // subresource relationships.
  if (!referring_scheme_host.is_empty() &&
      resource_type != content::RESOURCE_TYPE_MAIN_FRAME &&
      predictor_->timed_cache()->WasRecentlySeen(referring_scheme_host)) {
    predictor_->LearnFromNavigation(referring_scheme_host, request_scheme_host);
  }

  // If the referring host is equal to the request host, then the predictor has
  // already made any/all predictions when navigating to the referring host.
  // Don't update the RecentlySeen() time because the timed cache is already
  // populated (with the correct timeout) based on the initial navigation.
  if (IsNavigationRequest(request_) &&
      referring_scheme_host != request_scheme_host) {
    predictor_->timed_cache()->SetRecentlySeen(request_scheme_host);
    DispatchPredictions(request_scheme_host, request_->site_for_cookies());
  }
}

void PredictorResourceThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {
  GURL new_scheme_host(
      chrome_browser_net::Predictor::CanonicalizeUrl(redirect_info.new_url));
  GURL original_scheme_host(request_->original_url().GetWithEmptyPath());
  // Note: This is comparing a canonicalizd URL with a non-canonicalized URL.
  // This should be fixed and the idea of canonicalization revisited.
  if (new_scheme_host == original_scheme_host || new_scheme_host.is_empty())
    return;
  // Don't learn or predict subresource redirects.
  if (!IsNavigationRequest(request_))
    return;

  // Don't learn from redirects that take path as an argument, but do
  // learn from short-hand typing entries, such as "cnn.com" redirects to
  // "www.cnn.com".  We can't just check for has_path(), as a mere "/"
  // will count as a path, so we check that the path is at most a "/"
  // (1 character long) to decide the redirect is "definitive" and has no
  // significant path.
  // TODO(jar): It may be ok to learn from all redirects, as the adaptive
  // system will not respond until several identical redirects have taken
  // place.  Hence a use of a path (that changes) wouldn't really be
  // learned from anyway.
  if (request_->original_url().path().length() <= 1 &&
      predictor_->timed_cache()->WasRecentlySeen(original_scheme_host)) {
    // TODO(jar): These definite redirects could be learned much faster.
    predictor_->LearnFromNavigation(original_scheme_host, new_scheme_host);
  }

  predictor_->timed_cache()->SetRecentlySeen(new_scheme_host);
  DispatchPredictions(new_scheme_host, redirect_info.new_site_for_cookies);
}

const char* PredictorResourceThrottle::GetNameForLogging() const {
  return "PredictorResourceThrottle";
}

void PredictorResourceThrottle::DispatchPredictions(
    const GURL& url,
    const GURL& site_for_cookies) {
  // Dispatch predictions asynchronously to avoid blocking the actual request
  // from going out to the network.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&PredictorResourceThrottle::DoPredict,
                     weak_factory_.GetWeakPtr(), url, site_for_cookies));
}

void PredictorResourceThrottle::DoPredict(const GURL& url,
                                          const GURL& site_for_cookies) {
  TRACE_EVENT0("loading", "PredictorResourceThrottle::DoPredict");
  predictor_->PredictFrameSubresources(url, site_for_cookies);
}
