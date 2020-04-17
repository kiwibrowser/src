// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/subresource_filter_safe_browsing_client_request.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "components/subresource_filter/content/browser/subresource_filter_safe_browsing_client.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

namespace subresource_filter {

constexpr base::TimeDelta
    SubresourceFilterSafeBrowsingClientRequest::kCheckURLTimeout;

SubresourceFilterSafeBrowsingClientRequest::
    SubresourceFilterSafeBrowsingClientRequest(
        size_t request_id,
        scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager>
            database_manager,
        scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
        SubresourceFilterSafeBrowsingClient* client)
    : request_id_(request_id),
      database_manager_(std::move(database_manager)),
      client_(client) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  timer_.SetTaskRunner(std::move(io_task_runner));
}

SubresourceFilterSafeBrowsingClientRequest::
    ~SubresourceFilterSafeBrowsingClientRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_completed_)
    database_manager_->CancelCheck(this);
  timer_.Stop();
}

void SubresourceFilterSafeBrowsingClientRequest::Start(const GURL& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  start_time_ = base::TimeTicks::Now();

  // Just return SAFE if the database is not supported.
  bool synchronous_finish =
      !database_manager_->IsSupported() ||
      database_manager_->CheckUrlForSubresourceFilter(url, this);
  if (synchronous_finish) {
    request_completed_ = true;
    SendCheckResultToClient(false /* served_from_network */,
                            safe_browsing::SB_THREAT_TYPE_SAFE,
                            safe_browsing::ThreatMetadata());
    return;
  }
  timer_.Start(
      FROM_HERE, kCheckURLTimeout,
      base::Bind(&SubresourceFilterSafeBrowsingClientRequest::OnCheckUrlTimeout,
                 base::Unretained(this)));
}

void SubresourceFilterSafeBrowsingClientRequest::OnCheckBrowseUrlResult(
    const GURL& url,
    safe_browsing::SBThreatType threat_type,
    const safe_browsing::ThreatMetadata& metadata) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  request_completed_ = true;
  SendCheckResultToClient(true /* served_from_network */, threat_type,
                          metadata);
}

void SubresourceFilterSafeBrowsingClientRequest::OnCheckUrlTimeout() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  SendCheckResultToClient(true /* served_from_network */,
                          safe_browsing::SB_THREAT_TYPE_SAFE,
                          safe_browsing::ThreatMetadata());
}

void SubresourceFilterSafeBrowsingClientRequest::SendCheckResultToClient(
    bool served_from_network,
    safe_browsing::SBThreatType threat_type,
    const safe_browsing::ThreatMetadata& metadata) {
  SubresourceFilterSafeBrowsingClient::CheckResult result;
  result.request_id = request_id_;
  result.threat_type = threat_type;
  result.threat_metadata = metadata;
  result.check_time = base::TimeTicks::Now() - start_time_;

  // This memeber is separate from |request_completed_|, in that it just
  // indicates that this request is done processing (due to completion or
  // timeout).
  result.finished = true;

  UMA_HISTOGRAM_TIMES("SubresourceFilter.SafeBrowsing.CheckTime",
                      result.check_time);
  // Will delete |this|.
  client_->OnCheckBrowseUrlResult(this, result);
}

}  // namespace subresource_filter
