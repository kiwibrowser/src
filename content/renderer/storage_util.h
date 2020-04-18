// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_STORAGE_UTIL_H_
#define CONTENT_RENDERER_STORAGE_UTIL_H_

class GURL;

namespace blink {
class WebSecurityOrigin;
}  // namespace blink

namespace content {

// Storage APIs rely on GURLs to identify the origin, with some special cases.
// New uses of this function should be avoided; url::Origin should be used
// instead.
// TODO(jsbell): Eliminate this. https://crbug.com/591482
GURL WebSecurityOriginToGURL(const blink::WebSecurityOrigin& origin);

}  // namespace content

#endif  // CONTENT_RENDERER_STORAGE_UTIL_H_
