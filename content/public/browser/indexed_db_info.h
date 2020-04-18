// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_INDEXED_DB_INFO_H_
#define CONTENT_PUBLIC_BROWSER_INDEXED_DB_INFO_H_

#include <stddef.h>
#include <stdint.h>

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace content {

struct CONTENT_EXPORT IndexedDBInfo {
  IndexedDBInfo(const GURL& origin,
                int64_t size,
                const base::Time& last_modified,
                size_t connection_count)
      : origin(origin),
        size(size),
        last_modified(last_modified),
        connection_count(connection_count) {}

  GURL origin;
  int64_t size;
  base::Time last_modified;
  size_t connection_count;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_INDEXED_DB_INFO_H_
