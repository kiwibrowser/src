// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/blob_storage/webblobregistry_impl.h"

#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "content/child/thread_safe_sender.h"
#include "content/common/fileapi/webblob_messages.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"

using blink::WebString;
using blink::WebURL;

namespace content {

WebBlobRegistryImpl::WebBlobRegistryImpl(scoped_refptr<ThreadSafeSender> sender)
    : sender_(std::move(sender)) {
  // Record a dummy trace event on startup so the 'Storage' category shows up
  // in the chrome://tracing viewer.
  TRACE_EVENT0("Blob", "Init");
}

WebBlobRegistryImpl::~WebBlobRegistryImpl() {
}

void WebBlobRegistryImpl::RegisterPublicBlobURL(const WebURL& url,
                                                const WebString& uuid) {
  // Measure how much jank the following synchronous IPC introduces.
  SCOPED_UMA_HISTOGRAM_TIMER("Storage.Blob.RegisterPublicURLTime");

  sender_->Send(new BlobHostMsg_RegisterPublicURL(url, uuid.Utf8()));
}

void WebBlobRegistryImpl::RevokePublicBlobURL(const WebURL& url) {
  sender_->Send(new BlobHostMsg_RevokePublicURL(url));
}

}  // namespace content
