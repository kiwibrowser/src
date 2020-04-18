// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_CURSOR_UTILS_H_
#define CONTENT_RENDERER_CURSOR_UTILS_H_

class WebCursor;

namespace blink {
struct WebCursorInfo;
}

namespace content {

// Adapts our cursor info to blink::WebCursorInfo.
bool GetWebCursorInfo(const WebCursor& cursor,
                      blink::WebCursorInfo* web_cursor_info);

// Adapts blink::WebCursorInfo to our cursor.
void InitializeCursorFromWebCursorInfo(
    WebCursor* cursor,
    const blink::WebCursorInfo& web_cursor_info);

}  // namespace content

#endif  // CONTENT_RENDERER_CURSOR_UTILS_H_
