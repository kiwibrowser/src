// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/back_forward_cache.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/common/navigation_policy.h"

namespace content {

namespace {

// The number of document the BackForwardCache can hold per tab.
static constexpr size_t kBackForwardCacheLimit = 3;

}  // namespace

BackForwardCache::BackForwardCache() = default;
BackForwardCache::~BackForwardCache() = default;

bool BackForwardCache::CanStoreDocument(RenderFrameHostImpl* rfh) {
  // Use the BackForwardCache only for the main frame.
  if (rfh->GetParent())
    return false;

  if (!IsBackForwardCacheEnabled())
    return false;

  // TODO(arthursonzogni): In a lot of other cases, a document must not be in
  // the BackForwardCache. The main frame needs to be checked, but also its
  // iframes.
  // * Document using plugin.
  // * Document not fully loaded.
  // * Document with unload handlers.
  // * Error pages.
  // * AppCache?
  // * ...

  return true;
}

void BackForwardCache::StoreDocument(std::unique_ptr<RenderFrameHostImpl> rfh) {
  DCHECK(CanStoreDocument(rfh.get()));

  rfh->EnterBackForwardCache();
  render_frame_hosts_.push_front(std::move(rfh));

  // Remove the last recently used document if the BackForwardCache list is
  // full.
  if (render_frame_hosts_.size() > kBackForwardCacheLimit) {
    // TODO(arthursonzogni): Handle RenderFrame deletion appropriately.
    render_frame_hosts_.pop_back();
  }
}

std::unique_ptr<RenderFrameHostImpl> BackForwardCache::RestoreDocument(
    int navigation_entry_id) {
  // Select the RenderFrameHostImpl matching the navigation entry.
  auto matching_rfh = std::find_if(
      render_frame_hosts_.begin(), render_frame_hosts_.end(),
      [navigation_entry_id](std::unique_ptr<RenderFrameHostImpl>& rfh) {
        return rfh->nav_entry_id() == navigation_entry_id;
      });

  // Not found.
  if (matching_rfh == render_frame_hosts_.end())
    return nullptr;

  std::unique_ptr<RenderFrameHostImpl> rfh = std::move(*matching_rfh);
  render_frame_hosts_.erase(matching_rfh);
  rfh->LeaveBackForwardCache();
  return rfh;
}

// Remove all entries from the BackForwardCache.
void BackForwardCache::Flush() {
  render_frame_hosts_.clear();
}

}  // namespace content
