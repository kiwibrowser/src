// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/chrome_render_view_observer.h"

#include "components/web_cache/renderer/web_cache_impl.h"

ChromeRenderViewObserver::ChromeRenderViewObserver(
    content::RenderView* render_view,
    web_cache::WebCacheImpl* web_cache_impl)
    : content::RenderViewObserver(render_view),
      web_cache_impl_(web_cache_impl) {}

ChromeRenderViewObserver::~ChromeRenderViewObserver() {
}

void ChromeRenderViewObserver::Navigate(const GURL& url) {
  // Execute cache clear operations that were postponed until a navigation
  // event (including tab reload).
  if (web_cache_impl_)
    web_cache_impl_->ExecutePendingClearCache();
}

void ChromeRenderViewObserver::OnDestruct() {
  delete this;
}
