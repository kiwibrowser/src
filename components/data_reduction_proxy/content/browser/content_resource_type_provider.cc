// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/content/browser/content_resource_type_provider.h"

#include "content/public/browser/resource_request_info.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace data_reduction_proxy {

namespace {

// The maximum size of the cache that contains the mapping from GURL to
// ResourceTypeProvider::ContentType.
static const size_t kMaxCacheSize = 15;

// Returns the content type of |request|.
ResourceTypeProvider::ContentType GetContentTypeInternal(
    const net::URLRequest& request) {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(&request);

  if (!request_info)
    return ContentResourceTypeProvider::CONTENT_TYPE_UNKNOWN;

  content::ResourceType resource_type = request_info->GetResourceType();
  if (resource_type == content::RESOURCE_TYPE_MEDIA)
    return ContentResourceTypeProvider::CONTENT_TYPE_MEDIA;
  if (resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    return ContentResourceTypeProvider::CONTENT_TYPE_MAIN_FRAME;
  return ContentResourceTypeProvider::CONTENT_TYPE_UNKNOWN;
}

}  // namespace

ContentResourceTypeProvider::ContentResourceTypeProvider()
    : resource_content_types_(kMaxCacheSize) {
  // |this| is used on a different thread than the one on which it was created.
  thread_checker_.DetachFromThread();
}

ContentResourceTypeProvider::~ContentResourceTypeProvider() {}

ResourceTypeProvider::ContentType ContentResourceTypeProvider::GetContentType(
    const GURL& url) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Use Peek() instead of Get() to avoid changing the recency timestamp.
  auto iter = resource_content_types_.Peek(url.spec());
  if (iter == resource_content_types_.end())
    return ResourceTypeProvider::CONTENT_TYPE_UNKNOWN;
  return iter->second;
}

void ContentResourceTypeProvider::SetContentType(
    const net::URLRequest& request) {
  DCHECK(thread_checker_.CalledOnValidThread());
  ResourceTypeProvider::ContentType content_type =
      GetContentTypeInternal(request);
  if (content_type == CONTENT_TYPE_UNKNOWN) {
    // No need to insert entries with value |CONTENT_TYPE_UNKNOWN| since
    // |CONTENT_TYPE_UNKNOWN| is the default value and is returned when the URL
    // is not found in |resource_content_types_|.
    return;
  }
  resource_content_types_.Put(request.url().spec(), content_type);
}

}  // namespace data_reduction_proxy
