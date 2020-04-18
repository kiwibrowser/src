// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_MOCK_THUMBNAIL_FETCHER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_MOCK_THUMBNAIL_FETCHER_H_

#include "components/offline_pages/core/prefetch/thumbnail_fetcher.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace offline_pages {

class MockThumbnailFetcher : public ThumbnailFetcher {
 public:
  MockThumbnailFetcher();
  ~MockThumbnailFetcher() override;
  MOCK_METHOD3(FetchSuggestionImageData_,
               void(const ClientId& client_id,
                    bool is_first_attempt,
                    ImageDataFetchedCallback* callback));
  void FetchSuggestionImageData(const ClientId& client_id,
                                bool is_first_attempt,
                                ImageDataFetchedCallback callback) override {
    FetchSuggestionImageData_(client_id, is_first_attempt, &callback);
  }
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_MOCK_THUMBNAIL_FETCHER_H_
