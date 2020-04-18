// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_FONT_UTILS_LINUX_H_
#define CONTENT_BROWSER_RENDERER_HOST_FONT_UTILS_LINUX_H_

#include <stdint.h>

#include <string>

namespace content {

int MatchFontFaceWithFallback(const std::string& face,
                              bool is_bold,
                              bool is_italic,
                              uint32_t charset,
                              uint32_t fallback_family);

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_FONT_UTILS_LINUX_H_
