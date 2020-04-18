// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/offline_page_thumbnail.h"

#include <iostream>
#include "base/base64.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "components/offline_pages/core/offline_store_utils.h"

namespace offline_pages {

OfflinePageThumbnail::OfflinePageThumbnail() = default;
OfflinePageThumbnail::OfflinePageThumbnail(int64_t id,
                                           base::Time in_expiration,
                                           const std::string& in_thumbnail)
    : offline_id(id), expiration(in_expiration), thumbnail(in_thumbnail) {}
OfflinePageThumbnail::OfflinePageThumbnail(const OfflinePageThumbnail& other) =
    default;
OfflinePageThumbnail::OfflinePageThumbnail(OfflinePageThumbnail&& other) =
    default;
OfflinePageThumbnail::~OfflinePageThumbnail() {}

bool OfflinePageThumbnail::operator==(const OfflinePageThumbnail& other) const {
  return offline_id == other.offline_id && expiration == other.expiration &&
         thumbnail == other.thumbnail;
}

bool OfflinePageThumbnail::operator<(const OfflinePageThumbnail& other) const {
  return offline_id < other.offline_id;
}

std::string OfflinePageThumbnail::ToString() const {
  std::string thumb_data_base64;
  base::Base64Encode(thumbnail, &thumb_data_base64);

  std::string s("OfflinePageThumbnail(");
  s.append(base::Int64ToString(offline_id)).append(", ");
  s.append(base::Int64ToString(store_utils::ToDatabaseTime(expiration)))
      .append(", ");
  s.append(thumb_data_base64).append(")");
  return s;
}

std::ostream& operator<<(std::ostream& out, const OfflinePageThumbnail& thumb) {
  return out << thumb.ToString();
}

}  // namespace offline_pages
