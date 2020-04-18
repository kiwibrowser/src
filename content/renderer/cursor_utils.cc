// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/cursor_utils.h"

#include "build/build_config.h"
#include "content/common/cursors/webcursor.h"
#include "third_party/blink/public/platform/web_cursor_info.h"

using blink::WebCursorInfo;

namespace content {

bool GetWebCursorInfo(const WebCursor& cursor,
                      WebCursorInfo* web_cursor_info) {
  CursorInfo cursor_info;
  cursor.GetCursorInfo(&cursor_info);

  web_cursor_info->type = cursor_info.type;
  web_cursor_info->hot_spot = cursor_info.hotspot;
  web_cursor_info->custom_image = cursor_info.custom_image;
  web_cursor_info->image_scale_factor = cursor_info.image_scale_factor;
  return true;
}

void InitializeCursorFromWebCursorInfo(WebCursor* cursor,
                                       const WebCursorInfo& web_cursor_info) {
  CursorInfo cursor_info;
  cursor_info.type = web_cursor_info.type;
  cursor_info.image_scale_factor = web_cursor_info.image_scale_factor;
  cursor_info.hotspot = web_cursor_info.hot_spot;
  cursor_info.custom_image = web_cursor_info.custom_image.GetSkBitmap();
  cursor->InitFromCursorInfo(cursor_info);
}

}  // namespce content
