// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_webblob_registry_impl.h"

#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"

using blink::WebString;
using blink::WebURL;

namespace content {

MockWebBlobRegistryImpl::MockWebBlobRegistryImpl() {
}

MockWebBlobRegistryImpl::~MockWebBlobRegistryImpl() {
}

void MockWebBlobRegistryImpl::RegisterPublicBlobURL(const WebURL& url,
                                                    const WebString& uuid) {}

void MockWebBlobRegistryImpl::RevokePublicBlobURL(const WebURL& url) {}

}  // namespace content
