// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_MOCK_RESOURCE_FETCH_CONTEXT_H_
#define MEDIA_BLINK_MOCK_RESOURCE_FETCH_CONTEXT_H_

#include "base/macros.h"
#include "media/blink/resource_fetch_context.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media {

class MockResourceFetchContext : public ResourceFetchContext {
 public:
  MockResourceFetchContext();
  ~MockResourceFetchContext() override;

  MOCK_METHOD1(CreateUrlLoader,
               std::unique_ptr<blink::WebAssociatedURLLoader>(
                   const blink::WebAssociatedURLLoaderOptions&));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockResourceFetchContext);
};

}  // namespace media

#endif  // MEDIA_BLINK_MOCK_RESOURCE_FETCH_CONTEXT_H_
