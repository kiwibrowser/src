// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_MOCK_WEBBLOB_REGISTRY_IMPL_H_
#define CONTENT_TEST_MOCK_WEBBLOB_REGISTRY_IMPL_H_

#include <stddef.h>

#include "base/macros.h"
#include "third_party/blink/public/platform/web_blob_registry.h"

namespace content {

class MockWebBlobRegistryImpl : public blink::WebBlobRegistry {
 public:
  MockWebBlobRegistryImpl();
  ~MockWebBlobRegistryImpl() override;

  void RegisterPublicBlobURL(const blink::WebURL&,
                             const blink::WebString& uuid) override;
  void RevokePublicBlobURL(const blink::WebURL&) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWebBlobRegistryImpl);
};

}  // namespace content

#endif  // CONTENT_TEST_MOCK_WEBBLOB_REGISTRY_IMPL_H_
