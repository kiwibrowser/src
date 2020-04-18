// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_FAVICON_URL_H_
#define CONTENT_PUBLIC_COMMON_FAVICON_URL_H_

#include <vector>

#include "content/common/content_export.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace content {

// The favicon url from the render.
struct CONTENT_EXPORT FaviconURL {
  // The icon type in a page. The definition must be same as
  // favicon_base::IconType.
  enum class IconType {
    kInvalid,
    kFavicon,
    kTouchIcon,
    kTouchPrecomposedIcon,
    kMax = kTouchPrecomposedIcon,
  };

  FaviconURL();
  FaviconURL(const GURL& url,
             IconType type,
             const std::vector<gfx::Size>& sizes);
  FaviconURL(const FaviconURL& other);
  ~FaviconURL();

  // The url of the icon.
  GURL icon_url;

  // The type of the icon
  IconType icon_type;

  // Icon's bitmaps' size
  std::vector<gfx::Size> icon_sizes;
};

} // namespace content

#endif  // CONTENT_PUBLIC_COMMON_FAVICON_URL_H_
