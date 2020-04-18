// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_LOCAL_STORAGE_USAGE_INFO_H_
#define CONTENT_PUBLIC_BROWSER_LOCAL_STORAGE_USAGE_INFO_H_

#include <stddef.h>

#include "base/time/time.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace content {

// Used to report Local Storage usage info by DOMStorageContext.
struct CONTENT_EXPORT LocalStorageUsageInfo {
  LocalStorageUsageInfo() {}

  GURL origin;
  size_t data_size = 0;
  base::Time last_modified;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_LOCAL_STORAGE_USAGE_INFO_H_
