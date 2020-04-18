// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/content/source_url_recorder.h"

#include <utility>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_binding_set.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/metrics/public/cpp/delegating_ukm_recorder.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "third_party/blink/public/platform/ukm.mojom.h"
#include "url/gurl.h"

namespace ukm {

namespace internal {

// SourceUrlRecorderWebContentsObserver is responsible for recording UKM source
// URLs, for all (any only) main frame navigations in a given WebContents.
// SourceUrlRecorderWebContentsObserver records both the final URL for a
// navigation, and, if the navigation was redirected, the initial URL as well.
class SourceUrlRecorderWebContentsObserver
    : public blink::mojom::UkmSourceIdFrameHost,
      public content::WebContentsObserver,
      public content::WebContentsUserData<
          SourceUrlRecorderWebContentsObserver> {
 public:
  // Creates a SourceUrlRecorderWebContentsObserver for the given
  // WebContents. If a SourceUrlRecorderWebContentsObserver is already
  // associated with the WebContents, this method is a no-op.
  static void CreateForWebContents(content::WebContents* web_contents);

  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  ukm::SourceId GetLastCommittedSourceId() const;

  // blink::mojom::UkmSourceIdFrameHost
  void SetDocumentSourceId(int64_t source_id) override;

 private:
  explicit SourceUrlRecorderWebContentsObserver(
      content::WebContents* web_contents);
  friend class content::WebContentsUserData<
      SourceUrlRecorderWebContentsObserver>;

  // Record any pending DocumentCreated events to UKM.
  void MaybeFlushPendingEvents();

  void MaybeRecordUrl(content::NavigationHandle* navigation_handle,
                      const GURL& initial_url);

  // Recieves document source IDs from the renderer.
  content::WebContentsFrameBindingSet<blink::mojom::UkmSourceIdFrameHost>
      bindings_;

  // Map from navigation ID to the initial URL for that navigation.
  base::flat_map<int64_t, GURL> pending_navigations_;

  // Holds pending DocumentCreated events.
  using PendingEvent = std::pair<int64_t, bool>;
  std::vector<PendingEvent> pending_document_created_events_;

  int64_t last_committed_source_id_;

  DISALLOW_COPY_AND_ASSIGN(SourceUrlRecorderWebContentsObserver);
};

SourceUrlRecorderWebContentsObserver::SourceUrlRecorderWebContentsObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      bindings_(web_contents, this),
      last_committed_source_id_(ukm::kInvalidSourceId) {}

void SourceUrlRecorderWebContentsObserver::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  // UKM only records URLs for main frame (web page) navigations, so ignore
  // non-main frame navs. Additionally, at least for the time being, we don't
  // track metrics for same-document navigations (e.g. changes in URL fragment,
  // or URL changes due to history.pushState) in UKM.
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  // UKM doesn't want to record URLs for downloads. However, at the point a
  // navigation is started, we don't yet know if the navigation will result in a
  // download. Thus, we record the URL at the time a navigation was initiated,
  // and only record it later, once we verify that the navigation didn't result
  // in a download.
  pending_navigations_.insert(std::make_pair(
      navigation_handle->GetNavigationId(), navigation_handle->GetURL()));

  // Clear any unassociated pending events.
  pending_document_created_events_.clear();
}

void SourceUrlRecorderWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  auto it = pending_navigations_.find(navigation_handle->GetNavigationId());
  if (it == pending_navigations_.end())
    return;

  DCHECK(navigation_handle->IsInMainFrame());
  DCHECK(!navigation_handle->IsSameDocument());

  if (navigation_handle->HasCommitted()) {
    last_committed_source_id_ = ukm::ConvertToSourceId(
        navigation_handle->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);
  }

  GURL initial_url = std::move(it->second);
  pending_navigations_.erase(it);

  // UKM doesn't want to record URLs for navigations that result in downloads.
  if (navigation_handle->IsDownload())
    return;

  MaybeRecordUrl(navigation_handle, initial_url);

  MaybeFlushPendingEvents();
}

ukm::SourceId SourceUrlRecorderWebContentsObserver::GetLastCommittedSourceId()
    const {
  return last_committed_source_id_;
}

void SourceUrlRecorderWebContentsObserver::SetDocumentSourceId(
    int64_t source_id) {
  pending_document_created_events_.emplace_back(
      source_id, !bindings_.GetCurrentTargetFrame()->GetParent());
  MaybeFlushPendingEvents();
}

void SourceUrlRecorderWebContentsObserver::MaybeFlushPendingEvents() {
  if (!last_committed_source_id_)
    return;

  ukm::DelegatingUkmRecorder* ukm_recorder = ukm::DelegatingUkmRecorder::Get();
  if (!ukm_recorder)
    return;

  while (!pending_document_created_events_.empty()) {
    auto record = pending_document_created_events_.back();

    ukm::builders::DocumentCreated(record.first)
        .SetNavigationSourceId(last_committed_source_id_)
        .SetIsMainFrame(record.second)
        .Record(ukm_recorder);

    pending_document_created_events_.pop_back();
  }
}

void SourceUrlRecorderWebContentsObserver::MaybeRecordUrl(
    content::NavigationHandle* navigation_handle,
    const GURL& initial_url) {
  DCHECK(navigation_handle->IsInMainFrame());
  DCHECK(!navigation_handle->IsSameDocument());

  ukm::DelegatingUkmRecorder* ukm_recorder = ukm::DelegatingUkmRecorder::Get();
  if (!ukm_recorder)
    return;

  const ukm::SourceId source_id = ukm::ConvertToSourceId(
      navigation_handle->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);
  ukm_recorder->UpdateNavigationURL(source_id, initial_url);

  const GURL& final_url = navigation_handle->GetURL();
  if (final_url != initial_url)
    ukm_recorder->UpdateNavigationURL(source_id, final_url);
}

// static
void SourceUrlRecorderWebContentsObserver::CreateForWebContents(
    content::WebContents* web_contents) {
  if (!SourceUrlRecorderWebContentsObserver::FromWebContents(web_contents)) {
    web_contents->SetUserData(
        SourceUrlRecorderWebContentsObserver::UserDataKey(),
        base::WrapUnique(
            new SourceUrlRecorderWebContentsObserver(web_contents)));
  }
}

}  // namespace internal

void InitializeSourceUrlRecorderForWebContents(
    content::WebContents* web_contents) {
  internal::SourceUrlRecorderWebContentsObserver::CreateForWebContents(
      web_contents);
}

SourceId GetSourceIdForWebContentsDocument(
    const content::WebContents* web_contents) {
  const internal::SourceUrlRecorderWebContentsObserver* obs =
      internal::SourceUrlRecorderWebContentsObserver::FromWebContents(
          web_contents);
  return obs ? obs->GetLastCommittedSourceId() : kInvalidSourceId;
}

}  // namespace ukm

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    ukm::internal::SourceUrlRecorderWebContentsObserver);
