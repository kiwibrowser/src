// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/srcobject/html_media_element_src_object.h"

#include "third_party/blink/renderer/core/html/media/html_media_element.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_descriptor.h"

namespace blink {

// static
MediaStream* HTMLMediaElementSrcObject::srcObject(HTMLMediaElement& element) {
  MediaStreamDescriptor* descriptor = element.GetSrcObject();
  if (descriptor) {
    MediaStream* stream = ToMediaStream(descriptor);
    return stream;
  }

  return nullptr;
}

// static
void HTMLMediaElementSrcObject::setSrcObject(HTMLMediaElement& element,
                                             MediaStream* media_stream) {
  if (!media_stream) {
    element.SetSrcObject(nullptr);
    return;
  }
  element.SetSrcObject(media_stream->Descriptor());
}

}  // namespace blink
