// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_APPCACHE_INFO_H_
#define CONTENT_PUBLIC_COMMON_APPCACHE_INFO_H_

#include <stdint.h>

#include <vector>

#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/public/common/appcache_info.mojom.h"
#include "third_party/blink/public/platform/web_application_cache_host.h"
#include "url/gurl.h"

namespace content {

typedef base::OnceCallback<void(int)> OnceCompletionCallback;

static const int kAppCacheNoHostId =
    blink::WebApplicationCacheHost::kAppCacheNoHostId;

using mojom::kAppCacheNoCacheId;
using mojom::kAppCacheNoResponseId;
using mojom::kAppCacheUnknownCacheId;

using mojom::AppCacheStatus;
using mojom::AppCacheInfo;

typedef std::vector<AppCacheInfo> AppCacheInfoVector;

}  // namespace

#endif  // CONTENT_PUBLIC_COMMON_APPCACHE_INFO_H_
