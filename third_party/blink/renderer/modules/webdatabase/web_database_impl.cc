// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webdatabase/web_database_impl.h"

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/blink/renderer/modules/webdatabase/database_tracker.h"
#include "third_party/blink/renderer/modules/webdatabase/quota_tracker.h"

namespace blink {

WebDatabaseImpl::WebDatabaseImpl() = default;

WebDatabaseImpl::~WebDatabaseImpl() = default;

void WebDatabaseImpl::Create(mojom::blink::WebDatabaseRequest request) {
  mojo::MakeStrongBinding(std::make_unique<WebDatabaseImpl>(),
                          std::move(request));
}

void WebDatabaseImpl::UpdateSize(
    const scoped_refptr<const SecurityOrigin>& origin,
    const String& name,
    int64_t size) {
  DCHECK(origin->CanAccessDatabase());
  QuotaTracker::Instance().UpdateDatabaseSize(origin.get(), name, size);
}

void WebDatabaseImpl::CloseImmediately(
    const scoped_refptr<const SecurityOrigin>& origin,
    const String& name) {
  DCHECK(origin->CanAccessDatabase());
  DatabaseTracker::Tracker().CloseDatabasesImmediately(origin.get(), name);
}

}  // namespace blink
