// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_WEB_MEDIA_ELEMENT_SOURCE_UTILS_H_
#define CONTENT_RENDERER_MEDIA_WEB_MEDIA_ELEMENT_SOURCE_UTILS_H_

namespace blink {
class WebMediaPlayerSource;
class WebMediaStream;
}

namespace content {

// Obtains a WebMediaStream from a WebMediaPlayerSource. If the
// WebMediaPlayerSource does not contain a WebMediaStream, a null
// WebMediaStream is returned.
blink::WebMediaStream GetWebMediaStreamFromWebMediaPlayerSource(
    const blink::WebMediaPlayerSource& source);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_WEB_MEDIA_ELEMENT_SOURCE_UTILS_H_
