// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/cache_stats_recorder.h"

#include "components/web_cache/browser/web_cache_manager.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"

CacheStatsRecorder::CacheStatsRecorder(int render_process_id)
    : render_process_id_(render_process_id) {}

CacheStatsRecorder::~CacheStatsRecorder() = default;

// static
void CacheStatsRecorder::Create(
    int render_process_id,
    chrome::mojom::CacheStatsRecorderAssociatedRequest request) {
  mojo::MakeStrongAssociatedBinding(
      std::make_unique<CacheStatsRecorder>(render_process_id),
      std::move(request));
}

void CacheStatsRecorder::RecordCacheStats(uint64_t capacity, uint64_t size) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  web_cache::WebCacheManager::GetInstance()->ObserveStats(render_process_id_,
                                                          capacity, size);
}
