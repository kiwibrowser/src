// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/dom_storage/webstoragenamespace_impl.h"

#include "base/logging.h"
#include "content/common/dom_storage/dom_storage_types.h"
#include "content/renderer/dom_storage/webstoragearea_impl.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "url/gurl.h"
#include "url/origin.h"

using blink::WebStorageArea;
using blink::WebStorageNamespace;

namespace content {

WebStorageNamespaceImpl::WebStorageNamespaceImpl(
    const std::string& namespace_id)
    : namespace_id_(namespace_id) {
  DCHECK(!namespace_id.empty());
}

WebStorageNamespaceImpl::~WebStorageNamespaceImpl() {
}

WebStorageArea* WebStorageNamespaceImpl::CreateStorageArea(
    const blink::WebSecurityOrigin& origin) {
  return new WebStorageAreaImpl(namespace_id_, url::Origin(origin).GetURL());
}

WebStorageNamespace* WebStorageNamespaceImpl::copy() {
  // By returning NULL, we're telling WebKit to lazily fetch it the next time
  // session storage is used.  In the WebViewClient::createView, we do the
  // book-keeping necessary to make it a true copy-on-write despite not doing
  // anything here, now.
  return nullptr;
}

bool WebStorageNamespaceImpl::IsSameNamespace(
    const WebStorageNamespace& other) const {
  const WebStorageNamespaceImpl* other_impl =
      static_cast<const WebStorageNamespaceImpl*>(&other);
  return namespace_id_ == other_impl->namespace_id_;
}

}  // namespace content
