// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/stats_collection_observer.h"

#include "base/time/time.h"
#include "content/renderer/render_view_impl.h"

namespace content {

StatsCollectionObserver::StatsCollectionObserver(RenderViewImpl* render_view)
    : RenderViewObserver(render_view) {
}

StatsCollectionObserver::~StatsCollectionObserver() {
}

void StatsCollectionObserver::DidStartLoading() {
  DCHECK(start_time_.is_null());
  start_time_ = base::Time::Now();
}

void StatsCollectionObserver::DidStopLoading() {
  DCHECK(stop_time_.is_null());
  stop_time_ = base::Time::Now();

  // Stop observing so we don't get called again.
  RenderViewImpl* impl = static_cast<RenderViewImpl*>(render_view());
  impl->RemoveObserver(this);
}

void StatsCollectionObserver::OnDestruct() {
  delete this;
}

}  // namespace content
