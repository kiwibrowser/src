// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_THUMBNAIL_FETCHER_IMPL_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_THUMBNAIL_FETCHER_IMPL_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/prefetch/thumbnail_fetcher.h"

namespace ntp_snippets {
class ContentSuggestionsService;
}

namespace offline_pages {

// Fetches thumbnails through ContentSuggestionsService.
class ThumbnailFetcherImpl : public ThumbnailFetcher {
 public:
  ThumbnailFetcherImpl();
  ~ThumbnailFetcherImpl() override;

  void SetContentSuggestionsService(
      ntp_snippets::ContentSuggestionsService* content_suggestions) override;

  void FetchSuggestionImageData(const ClientId& client_id,
                                bool is_first_attempt,
                                ImageDataFetchedCallback callback) override;

 private:
  ntp_snippets::ContentSuggestionsService* content_suggestions_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(ThumbnailFetcherImpl);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_THUMBNAIL_FETCHER_IMPL_H_
