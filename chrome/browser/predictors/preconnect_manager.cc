// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/preconnect_manager.h"

#include <utility>

#include "base/trace_event/trace_event.h"
#include "chrome/browser/predictors/resource_prefetch_predictor.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_hints.h"
#include "net/base/net_errors.h"
#include "net/http/transport_security_state.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"

namespace predictors {

const bool kAllowCredentialsOnPreconnectByDefault = true;

PreconnectedRequestStats::PreconnectedRequestStats(const GURL& origin,
                                                   bool was_preresolve_cached,
                                                   bool was_preconnected)
    : origin(origin),
      was_preresolve_cached(was_preresolve_cached),
      was_preconnected(was_preconnected) {}

PreconnectedRequestStats::PreconnectedRequestStats(
    const PreconnectedRequestStats& other) = default;
PreconnectedRequestStats::~PreconnectedRequestStats() = default;

PreconnectStats::PreconnectStats(const GURL& url)
    : url(url), start_time(base::TimeTicks::Now()) {}
PreconnectStats::~PreconnectStats() = default;

PreresolveInfo::PreresolveInfo(const GURL& url, size_t count)
    : url(url),
      queued_count(count),
      inflight_count(0),
      was_canceled(false),
      stats(std::make_unique<PreconnectStats>(url)) {}

PreresolveInfo::~PreresolveInfo() = default;

PreresolveJob::PreresolveJob(const GURL& url,
                             int num_sockets,
                             bool allow_credentials,
                             PreresolveInfo* info)
    : url(url),
      num_sockets(num_sockets),
      allow_credentials(allow_credentials),
      info(info) {
  DCHECK_GE(num_sockets, 0);
}

PreresolveJob::PreresolveJob(const PreresolveJob& other) = default;
PreresolveJob::~PreresolveJob() = default;

PreconnectManager::PreconnectManager(
    base::WeakPtr<Delegate> delegate,
    scoped_refptr<net::URLRequestContextGetter> context_getter)
    : delegate_(std::move(delegate)),
      context_getter_(std::move(context_getter)),
      inflight_preresolves_count_(0),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(context_getter_);
}

PreconnectManager::~PreconnectManager() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

void PreconnectManager::Start(const GURL& url,
                              std::vector<PreconnectRequest>&& requests) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  const std::string host = url.host();
  if (preresolve_info_.find(host) != preresolve_info_.end())
    return;

  auto iterator_and_whether_inserted = preresolve_info_.emplace(
      host, std::make_unique<PreresolveInfo>(url, requests.size()));
  PreresolveInfo* info = iterator_and_whether_inserted.first->second.get();

  for (const auto& request : requests) {
    DCHECK(request.origin.GetOrigin() == request.origin);
    queued_jobs_.emplace_back(request.origin, request.num_sockets,
                              request.allow_credentials, info);
  }

  TryToLaunchPreresolveJobs();
}

void PreconnectManager::StartPreresolveHost(const GURL& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!url.SchemeIsHTTPOrHTTPS())
    return;
  queued_jobs_.emplace_front(url.GetOrigin(), 0,
                             kAllowCredentialsOnPreconnectByDefault, nullptr);

  TryToLaunchPreresolveJobs();
}

void PreconnectManager::StartPreresolveHosts(
    const std::vector<std::string>& hostnames) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // Push jobs in front of the queue due to higher priority.
  for (auto it = hostnames.rbegin(); it != hostnames.rend(); ++it) {
    queued_jobs_.emplace_front(GURL("http://" + *it), 0,
                               kAllowCredentialsOnPreconnectByDefault, nullptr);
  }

  TryToLaunchPreresolveJobs();
}

void PreconnectManager::StartPreconnectUrl(const GURL& url,
                                           bool allow_credentials) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!url.SchemeIsHTTPOrHTTPS())
    return;
  queued_jobs_.emplace_front(url.GetOrigin(), 1, allow_credentials, nullptr);

  TryToLaunchPreresolveJobs();
}

void PreconnectManager::Stop(const GURL& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  auto it = preresolve_info_.find(url.host());
  if (it == preresolve_info_.end()) {
    return;
  }

  it->second->was_canceled = true;
}

void PreconnectManager::PreconnectUrl(const GURL& url,
                                      const GURL& site_for_cookies,
                                      int num_sockets,
                                      bool allow_credentials) const {
  DCHECK(url.GetOrigin() == url);
  DCHECK(url.SchemeIsHTTPOrHTTPS());
  content::PreconnectUrl(context_getter_.get(), url, site_for_cookies,
                         num_sockets, allow_credentials);
}

int PreconnectManager::PreresolveUrl(
    const GURL& url,
    const net::CompletionCallback& callback) const {
  DCHECK(url.GetOrigin() == url);
  DCHECK(url.SchemeIsHTTPOrHTTPS());
  return content::PreresolveUrl(context_getter_.get(), url, callback);
}

void PreconnectManager::TryToLaunchPreresolveJobs() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  while (!queued_jobs_.empty() &&
         inflight_preresolves_count_ < kMaxInflightPreresolves) {
    auto& job = queued_jobs_.front();
    PreresolveInfo* info = job.info;

    if (!info || !info->was_canceled) {
      int status;
      if (WouldLikelyProxyURL(job.url)) {
        // Skip preresolve and go straight to preconnect if a proxy is enabled.
        status = net::OK;
      } else {
        status = PreresolveUrl(
            job.url, base::Bind(&PreconnectManager::OnPreresolveFinished,
                                weak_factory_.GetWeakPtr(), job));
      }

      if (status == net::ERR_IO_PENDING) {
        // Will complete asynchronously.
        if (info)
          ++info->inflight_count;
        ++inflight_preresolves_count_;
      } else {
        // Completed synchronously (was already cached by HostResolver), or else
        // there was (equivalently) some network error that prevents us from
        // finding the name. Status net::OK means it was "found."
        FinishPreresolve(job, status == net::OK, true);
      }
    }

    queued_jobs_.pop_front();
    if (info)
      --info->queued_count;
    if (info && info->is_done())
      AllPreresolvesForUrlFinished(info);
  }
}

void PreconnectManager::OnPreresolveFinished(const PreresolveJob& job,
                                             int result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  FinishPreresolve(job, result == net::OK, false);
  PreresolveInfo* info = job.info;
  --inflight_preresolves_count_;
  if (info)
    --info->inflight_count;
  if (info && info->is_done())
    AllPreresolvesForUrlFinished(info);
  TryToLaunchPreresolveJobs();
}

void PreconnectManager::FinishPreresolve(const PreresolveJob& job,
                                         bool found,
                                         bool cached) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  PreresolveInfo* info = job.info;
  bool need_preconnect =
      found && job.need_preconnect() && (!info || !info->was_canceled);
  if (need_preconnect) {
    PreconnectUrl(GetHSTSRedirect(job.url), info ? info->url : GURL(),
                  job.num_sockets, job.allow_credentials);
  }
  if (info && found)
    info->stats->requests_stats.emplace_back(job.url, cached, need_preconnect);
}

void PreconnectManager::AllPreresolvesForUrlFinished(PreresolveInfo* info) {
  DCHECK(info);
  DCHECK(info->is_done());
  auto it = preresolve_info_.find(info->url.host());
  DCHECK(it != preresolve_info_.end());
  DCHECK(info == it->second.get());
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&Delegate::PreconnectFinished, delegate_,
                     std::move(info->stats)));
  preresolve_info_.erase(it);
}

GURL PreconnectManager::GetHSTSRedirect(const GURL& url) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!url.SchemeIs(url::kHttpScheme))
    return url;

  auto* transport_security_state =
      context_getter_->GetURLRequestContext()->transport_security_state();
  if (!transport_security_state)
    return url;

  if (!transport_security_state->ShouldUpgradeToSSL(url.host()))
    return url;

  GURL::Replacements replacements;
  replacements.SetSchemeStr(url::kHttpsScheme);
  return url.ReplaceComponents(replacements);
}

bool PreconnectManager::WouldLikelyProxyURL(const GURL& url) const {
  auto* proxy_resolution_service =
      context_getter_->GetURLRequestContext()->proxy_resolution_service();
  if (!proxy_resolution_service)
    return false;

  net::ProxyInfo info;
  bool synchronous_success =
      proxy_resolution_service->TryResolveProxySynchronously(
          url, std::string(), &info, nullptr, net::NetLogWithSource());
  return synchronous_success && !info.is_direct();
}

}  // namespace predictors
