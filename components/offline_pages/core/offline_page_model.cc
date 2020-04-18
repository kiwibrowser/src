// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_model.h"

#include "url/gurl.h"

namespace offline_pages {

const int64_t OfflinePageModel::kInvalidOfflineId;

OfflinePageModel::SavePageParams::SavePageParams()
    : proposed_offline_id(OfflinePageModel::kInvalidOfflineId),
      is_background(false) {}

OfflinePageModel::SavePageParams::SavePageParams(const SavePageParams& other) =
    default;

OfflinePageModel::SavePageParams::~SavePageParams() = default;

OfflinePageModel::DeletedPageInfo::DeletedPageInfo() = default;
OfflinePageModel::DeletedPageInfo::DeletedPageInfo(
    const DeletedPageInfo& other) = default;
OfflinePageModel::DeletedPageInfo::~DeletedPageInfo() = default;
OfflinePageModel::DeletedPageInfo::DeletedPageInfo(
    int64_t offline_id,
    int64_t system_download_id,
    const ClientId& client_id,
    const std::string& request_origin,
    const GURL& url)
    : offline_id(offline_id),
      system_download_id(system_download_id),
      client_id(client_id),
      request_origin(request_origin),
      url(url) {}

// static
bool OfflinePageModel::CanSaveURL(const GURL& url) {
  return url.is_valid() && url.SchemeIsHTTPOrHTTPS();
}

OfflinePageModel::OfflinePageModel() = default;

OfflinePageModel::~OfflinePageModel() = default;

}  // namespace offline_pages
