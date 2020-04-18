// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_content_suggestions_service_proxy.h"

#include <utility>

#include "base/bind.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace contextual_suggestions {

namespace {
// TODO(pnoland): check if this is the correct base URL for all images.
static constexpr char kImageURLFormat[] =
    "http://www.google.com/images?q=tbn:%s";

GURL ImageUrlFromId(const std::string& image_id) {
  return GURL(base::StringPrintf(kImageURLFormat, image_id.c_str()));
}
}  // namespace

ContextualContentSuggestionsServiceProxy::
    ContextualContentSuggestionsServiceProxy(
        ContextualContentSuggestionsService* service,
        std::unique_ptr<ContextualSuggestionsReporter> reporter)
    : service_(service),
      reporter_(std::move(reporter)),
      last_ukm_source_id_(ukm::kInvalidSourceId),
      weak_ptr_factory_(this) {}

ContextualContentSuggestionsServiceProxy::
    ~ContextualContentSuggestionsServiceProxy() {}

void ContextualContentSuggestionsServiceProxy::FetchContextualSuggestions(
    const GURL& url,
    FetchClustersCallback callback) {
  service_->FetchContextualSuggestionClusters(
      url,
      base::BindOnce(
          &ContextualContentSuggestionsServiceProxy::CacheSuggestions,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      base::BindRepeating(
          &ContextualContentSuggestionsServiceProxy::ReportEvent,
          weak_ptr_factory_.GetWeakPtr(), last_ukm_source_id_, url.spec()));
}

void ContextualContentSuggestionsServiceProxy::FetchContextualSuggestionImage(
    const std::string& suggestion_id,
    ntp_snippets::ImageFetchedCallback callback) {
  auto suggestion_iter = suggestions_.find(suggestion_id);
  if (suggestion_iter == suggestions_.end()) {
    DVLOG(1) << "Unkown suggestion ID: " << suggestion_id;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), gfx::Image()));
    return;
  }

  std::string& image_id = suggestion_iter->second.image_id;
  GURL image_url = ImageUrlFromId(image_id);

  FetchImageImpl(image_url, image_id, std::move(callback));
}

void ContextualContentSuggestionsServiceProxy::FetchContextualSuggestionFavicon(
    const std::string& suggestion_id,
    ntp_snippets::ImageFetchedCallback callback) {
  auto suggestion_iter = suggestions_.find(suggestion_id);
  if (suggestion_iter == suggestions_.end()) {
    DVLOG(1) << "Unkown suggestion ID: " << suggestion_id;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), gfx::Image()));
    return;
  }

  FetchImageImpl(GURL(suggestion_iter->second.favicon_image_url),
                 suggestion_iter->second.favicon_image_id, std::move(callback));
}

void ContextualContentSuggestionsServiceProxy::ClearState() {
  suggestions_.clear();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void ContextualContentSuggestionsServiceProxy::ReportEvent(
    ukm::SourceId ukm_source_id,
    const std::string& url,
    ContextualSuggestionsEvent event) {
  // TODO(pnoland): investigate how we can get into this state(one known
  // example is if we switch tabs and there's no committed navigation in the new
  // tab) and prevent it from happening. Replace the early return with a DCHECK
  // once this is done.
  if (ukm_source_id == ukm::kInvalidSourceId) {
    return;
  }

  // Flush the previous page (if any) and setup the new page.
  if (ukm_source_id != last_ukm_source_id_) {
    if (last_ukm_source_id_ != ukm::kInvalidSourceId)
      reporter_->Flush();
    last_ukm_source_id_ = ukm_source_id;
    reporter_->SetupForPage(url, ukm_source_id);
  }

  reporter_->RecordEvent(event);
}

void ContextualContentSuggestionsServiceProxy::FlushMetrics() {
  if (last_ukm_source_id_ != ukm::kInvalidSourceId)
    reporter_->Flush();
  last_ukm_source_id_ = ukm::kInvalidSourceId;
}

void ContextualContentSuggestionsServiceProxy::FetchImageImpl(
    const GURL& image_url,
    const std::string& image_id,
    ntp_snippets::ImageFetchedCallback callback) {
  ntp_snippets::ContentSuggestion::ID synthetic_cache_id(
      ntp_snippets::Category::FromKnownCategory(
          ntp_snippets::KnownCategories::CONTEXTUAL),
      image_id);

  service_->FetchContextualSuggestionImage(synthetic_cache_id, image_url,
                                           std::move(callback));
}

void ContextualContentSuggestionsServiceProxy::CacheSuggestions(
    FetchClustersCallback callback,
    ContextualSuggestionsResult result) {
  suggestions_.clear();
  for (auto& cluster : result.clusters) {
    for (auto& suggestion : cluster.suggestions) {
      suggestions_.emplace(std::make_pair(suggestion.id, suggestion));
    }
  }
  std::move(callback).Run(std::move(result));
}

}  // namespace contextual_suggestions
