// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/net/sync_websocket_factory.h"

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "chrome/test/chromedriver/net/sync_websocket_impl.h"
#include "chrome/test/chromedriver/net/url_request_context_getter.h"

namespace {

std::unique_ptr<SyncWebSocket> CreateSyncWebSocket(
    scoped_refptr<URLRequestContextGetter> context_getter) {
  return std::unique_ptr<SyncWebSocket>(
      new SyncWebSocketImpl(context_getter.get()));
}

}  // namespace

SyncWebSocketFactory CreateSyncWebSocketFactory(
    URLRequestContextGetter* getter) {
  return base::Bind(&CreateSyncWebSocket, base::WrapRefCounted(getter));
}
