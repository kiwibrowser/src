// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/resource_context_impl.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/logging.h"
#include "content/browser/blob_storage/chrome_blob_storage_context.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/browser/streams/stream_context.h"
#include "content/browser/webui/url_data_manager_backend.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

using base::UserDataAdapter;

namespace content {

// Key names on ResourceContext.
const char kStreamContextKeyName[] = "content_stream_context";
const char kURLDataManagerBackendKeyName[] = "url_data_manager_backend";

ResourceContext::ResourceContext() {}

ResourceContext::~ResourceContext() {
  if (ResourceDispatcherHostImpl::Get())
    ResourceDispatcherHostImpl::Get()->CancelRequestsForContext(this);
}

ChromeBlobStorageContext* GetChromeBlobStorageContextForResourceContext(
    const ResourceContext* resource_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return UserDataAdapter<ChromeBlobStorageContext>::Get(
      resource_context, kBlobStorageContextKeyName);
}

StreamContext* GetStreamContextForResourceContext(
    const ResourceContext* resource_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return UserDataAdapter<StreamContext>::Get(
      resource_context, kStreamContextKeyName);
}

URLDataManagerBackend* GetURLDataManagerForResourceContext(
    ResourceContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!context->GetUserData(kURLDataManagerBackendKeyName)) {
    context->SetUserData(kURLDataManagerBackendKeyName,
                         std::make_unique<URLDataManagerBackend>());
  }
  return static_cast<URLDataManagerBackend*>(
      context->GetUserData(kURLDataManagerBackendKeyName));
}

void InitializeResourceContext(BrowserContext* browser_context) {
  ResourceContext* resource_context = browser_context->GetResourceContext();

  resource_context->SetUserData(
      kBlobStorageContextKeyName,
      std::make_unique<UserDataAdapter<ChromeBlobStorageContext>>(
          ChromeBlobStorageContext::GetFor(browser_context)));

  resource_context->SetUserData(
      kStreamContextKeyName, std::make_unique<UserDataAdapter<StreamContext>>(
                                 StreamContext::GetFor(browser_context)));

  resource_context->DetachFromSequence();
}

}  // namespace content
