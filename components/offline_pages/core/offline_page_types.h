// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_TYPES_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_TYPES_H_

#include <stdint.h>

#include <set>
#include <vector>

#include "base/callback.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/offline_page_thumbnail.h"

class GURL;

// This file contains common callbacks used by OfflinePageModel and is a
// temporary step to refactor and interface of the model out of the
// implementation.
namespace offline_pages {
// Result of saving a page offline.
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.components.offlinepages
enum class SavePageResult {
  SUCCESS,
  CANCELLED,
  DEVICE_FULL,
  CONTENT_UNAVAILABLE,
  ARCHIVE_CREATION_FAILED,
  STORE_FAILURE,
  ALREADY_EXISTS,
  // Certain pages, i.e. file URL or NTP, will not be saved because these
  // are already locally accessible.
  SKIPPED,
  SECURITY_CERTIFICATE_ERROR,
  // Returned when we detect trying to save a chrome error page.
  ERROR_PAGE,
  // Returned when we detect trying to save a chrome interstitial page.
  INTERSTITIAL_PAGE,
  // Failed to compute digest for the archive file.
  DIGEST_CALCULATION_FAILED,
  // Unable to move the file into a public directory.
  FILE_MOVE_FAILED,
  // Unable to add the file to the system download manager.
  ADD_TO_DOWNLOAD_MANAGER_FAILED,
  // Unable to get write permission on public directory.
  PERMISSION_DENIED,
  // NOTE: always keep this entry at the end. Add new result types only
  // immediately above this line. Make sure to update the corresponding
  // histogram enum accordingly.
  RESULT_COUNT,
};

// Result of adding an offline page.
enum class AddPageResult {
  SUCCESS,
  STORE_FAILURE,
  ALREADY_EXISTS,
  // NOTE: always keep this entry at the end. Add new result types only
  // immediately above this line. Make sure to update the corresponding
  // histogram enum accordingly.
  RESULT_COUNT,
};

// Result of deleting an offline page.
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.components.offlinepages
enum class DeletePageResult {
  SUCCESS,
  CANCELLED,
  STORE_FAILURE,
  DEVICE_FAILURE,
  // Deprecated. Deleting pages which are not in metadata store would be
  // returing |SUCCESS|. Should not be used anymore.
  NOT_FOUND,
  // NOTE: always keep this entry at the end. Add new result types only
  // immediately above this line. Make sure to update the corresponding
  // histogram enum accordingly.
  RESULT_COUNT,
};

// Controls how to search on differnt URLs for pages.
enum class URLSearchMode {
  // Match against the last committed URL only.
  SEARCH_BY_FINAL_URL_ONLY,
  // Match against all stored URLs, including the last committed URL and
  // the original request URL.
  SEARCH_BY_ALL_URLS,
};

typedef std::vector<int64_t> MultipleOfflineIdResult;
typedef std::vector<OfflinePageItem> MultipleOfflinePageItemResult;

// TODO(carlosk): All or most of these should use base::OnceCallback.
typedef base::Callback<void(SavePageResult, int64_t)> SavePageCallback;
typedef base::Callback<void(AddPageResult, int64_t)> AddPageCallback;
typedef base::Callback<void(DeletePageResult)> DeletePageCallback;
typedef base::Callback<void(bool)> HasPagesCallback;
typedef base::Callback<void(const MultipleOfflineIdResult&)>
    MultipleOfflineIdCallback;
typedef base::OnceCallback<void(const OfflinePageItem*)>
    SingleOfflinePageItemCallback;
typedef base::OnceCallback<void(const MultipleOfflinePageItemResult&)>
    MultipleOfflinePageItemCallback;
typedef base::Callback<bool(const GURL&)> UrlPredicate;
typedef base::Callback<void(int64_t)> SizeInBytesCallback;
typedef base::OnceCallback<void(std::unique_ptr<OfflinePageThumbnail>)>
    GetThumbnailCallback;
typedef base::OnceCallback<void(bool)> CleanupThumbnailsCallback;

// Callbacks used for publishing an offline page.
using PublishPageCallback =
    base::OnceCallback<void(const base::FilePath&, SavePageResult)>;
using UpdateFilePathDoneCallback = base::OnceCallback<void(bool)>;

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_TYPES_H_
