// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_MOCK_RESOURCE_CONTEXT_H_
#define CONTENT_PUBLIC_TEST_MOCK_RESOURCE_CONTEXT_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/resource_context.h"

namespace net {
class URLRequestContext;
}

namespace content {

class MockResourceContext : public ResourceContext {
 public:
  MockResourceContext();

  // Does not take ownership of |test_request_context|.
  explicit MockResourceContext(net::URLRequestContext* test_request_context);

  ~MockResourceContext() override;

  // ResourceContext implementation:
  net::HostResolver* GetHostResolver() override;
  net::URLRequestContext* GetRequestContext() override;

 private:
  net::URLRequestContext* test_request_context_;

  DISALLOW_COPY_AND_ASSIGN(MockResourceContext);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_MOCK_RESOURCE_CONTEXT_H_
