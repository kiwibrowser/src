// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_BACK_FORWARD_CACHE_H_
#define CONTENT_BROWSER_FRAME_HOST_BACK_FORWARD_CACHE_H_

#include <list>
#include <memory>

#include "base/macros.h"
#include "content/common/content_export.h"

namespace content {

class RenderFrameHostImpl;

// BackForwardCache:
//
// After the user navigates away from a document, the old one goes into the
// frozen state and is kept in this object. They can potentially be reused
// after an history navigation. Reusing a document means swapping it back with
// the current_frame_host.
class CONTENT_EXPORT BackForwardCache {
 public:
  BackForwardCache();
  ~BackForwardCache();

  // Returns true when a RenderFrameHost can be stored into the
  // BackForwardCache. Depends on the |render_frame_host| and its children's
  // state.
  bool CanStoreDocument(RenderFrameHostImpl* render_frame_host);

  // Moves |render_frame_host| into the BackForwardCache. It can be reused in
  // a future history navigation by using RestoreDocument(). When the
  // BackForwardCache is full, the least recently used document is evicted.
  // Precondition: CanStoreDocument(render_frame_host).
  void StoreDocument(std::unique_ptr<RenderFrameHostImpl>);

  // During a history navigation, move a document out of the BackForwardCache
  // knowing its navigation entry ID. Returns nullptr when none is found.
  std::unique_ptr<RenderFrameHostImpl> RestoreDocument(int navigation_entry_id);

  // Remove all entries from the BackForwardCache.
  void Flush();

 private:
  // Contains the set of stored RenderFrameHost.
  // Invariant:
  // - Ordered from the most recently used to the last recently used.
  // - Once the list is full, the least recently used document is evicted.
  std::list<std::unique_ptr<RenderFrameHostImpl>> render_frame_hosts_;

  DISALLOW_COPY_AND_ASSIGN(BackForwardCache);
};
}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_BACK_FORWARD_CACHE_H_
