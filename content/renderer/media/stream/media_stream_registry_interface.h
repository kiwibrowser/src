// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_REGISTRY_INTERFACE_H_
#define CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_REGISTRY_INTERFACE_H_

#include <string>

#include "third_party/blink/public/platform/web_media_stream.h"

namespace content {

// Interface to get WebMediaStream from its url.
class MediaStreamRegistryInterface {
 public:
  virtual blink::WebMediaStream GetMediaStream(const std::string& url) = 0;

 protected:
  virtual ~MediaStreamRegistryInterface() {}
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_STREAM_MEDIA_STREAM_REGISTRY_INTERFACE_H_
