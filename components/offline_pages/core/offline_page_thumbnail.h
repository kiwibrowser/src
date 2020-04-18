// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_THUMBNAIL_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_THUMBNAIL_H_

#include <stdint.h>
#include <iosfwd>
#include <string>

#include "base/time/time.h"

namespace offline_pages {

// Thumbnail for an offline page. This maps to a row in the page_thumbnails
// table.
struct OfflinePageThumbnail {
 public:
  OfflinePageThumbnail();
  OfflinePageThumbnail(int64_t offline_id,
                       base::Time expiration,
                       const std::string& thumbnail);
  OfflinePageThumbnail(const OfflinePageThumbnail& other);
  OfflinePageThumbnail(OfflinePageThumbnail&& other);
  ~OfflinePageThumbnail();
  OfflinePageThumbnail& operator=(const OfflinePageThumbnail& other) = default;
  bool operator==(const OfflinePageThumbnail& other) const;
  bool operator<(const OfflinePageThumbnail& other) const;
  std::string ToString() const;

  // The primary key/ID for the page in offline pages internal database.
  int64_t offline_id = 0;
  // The time at which the thumbnail can be removed from the table, but only
  // if the offline_id does not match an offline_id in the offline pages table.
  base::Time expiration;
  // The thumbnail raw image data.
  std::string thumbnail;
};

std::ostream& operator<<(std::ostream& out, const OfflinePageThumbnail& thumb);
}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_OFFLINE_PAGE_THUMBNAIL_H_
