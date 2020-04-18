// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_cache/renderer/web_cache_impl.h"

#include <limits>
#include <memory>

#include "base/bind.h"
#include "base/numerics/safe_conversions.h"
#include "content/public/child/child_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/simple_connection_filter.h"
#include "content/public/renderer/render_thread.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/blink/public/platform/web_cache.h"

namespace web_cache {

WebCacheImpl::WebCacheImpl() : clear_cache_state_(kInit) {
  auto registry = std::make_unique<service_manager::BinderRegistry>();
  registry->AddInterface(
      base::Bind(&WebCacheImpl::BindRequest, base::Unretained(this)),
      base::ThreadTaskRunnerHandle::Get());
  if (content::ChildThread::Get()) {
    content::ChildThread::Get()
        ->GetServiceManagerConnection()
        ->AddConnectionFilter(std::make_unique<content::SimpleConnectionFilter>(
            std::move(registry)));
  }
}

WebCacheImpl::~WebCacheImpl() {}

void WebCacheImpl::BindRequest(mojom::WebCacheRequest web_cache_request) {
  bindings_.AddBinding(this, std::move(web_cache_request));
}

void WebCacheImpl::ExecutePendingClearCache() {
  switch (clear_cache_state_) {
    case kInit:
      clear_cache_state_ = kNavigate_Pending;
      break;
    case kNavigate_Pending:
      break;
    case kClearCache_Pending:
      blink::WebCache::Clear();
      clear_cache_state_ = kInit;
      break;
  }
}

void WebCacheImpl::SetCacheCapacity(uint64_t capacity64) {
  size_t capacity = base::checked_cast<size_t>(capacity64);

  blink::WebCache::SetCapacity(capacity);
}

void WebCacheImpl::ClearCache(bool on_navigation) {
  if (!on_navigation) {
    blink::WebCache::Clear();
    return;
  }

  switch (clear_cache_state_) {
    case kInit:
      clear_cache_state_ = kClearCache_Pending;
      break;
    case kNavigate_Pending:
      blink::WebCache::Clear();
      clear_cache_state_ = kInit;
      break;
    case kClearCache_Pending:
      break;
  }
}

}  // namespace web_cache
