// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_STATS_COLLECTION_OBSERVER_H_
#define CONTENT_RENDERER_STATS_COLLECTION_OBSERVER_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "content/public/renderer/render_view_observer.h"

namespace content {

class RenderViewImpl;

// Collect timing information for page loads.
// If a Renderview performs multiple loads, only the first one is recorded.
class StatsCollectionObserver : public RenderViewObserver {
 public:
  explicit StatsCollectionObserver(RenderViewImpl* render_view);
  ~StatsCollectionObserver() override;

  // RenderViewObserver implementation
  void DidStartLoading() override;
  void DidStopLoading() override;

  // Timing for the page load start and stop.  These functions may return
  // a null time value under various circumstances.
  const base::Time& load_start_time() { return start_time_; }
  const base::Time& load_stop_time() { return stop_time_; }

 private:
  // RenderViewObserver implementation.
  void OnDestruct() override;

  base::Time start_time_;
  base::Time stop_time_;

   DISALLOW_COPY_AND_ASSIGN(StatsCollectionObserver);
};

}  // namespace content

#endif  // CONTENT_RENDERER_STATS_COLLECTION_OBSERVER_H_
