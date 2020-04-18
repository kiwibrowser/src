// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_PEPPER_PEPPER_TO_VIDEO_TRACK_ADAPTER_H_
#define CONTENT_RENDERER_MEDIA_PEPPER_PEPPER_TO_VIDEO_TRACK_ADAPTER_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"

namespace content {

class MediaStreamRegistryInterface;
class PPB_ImageData_Impl;

// Interface used by a Pepper plugin to output frames to a video track.
class CONTENT_EXPORT FrameWriterInterface {
 public:
  // The ownership of the |image_data| doesn't transfer. So the implementation
  // of this interface should make a copy of the |image_data| before return.
  virtual void PutFrame(PPB_ImageData_Impl* image_data,
                        int64_t time_stamp_ns) = 0;
  virtual ~FrameWriterInterface() {}
};

// PepperToVideoTrackAdapter is a glue class between the content MediaStream and
// the effects pepper plugin host.
class CONTENT_EXPORT PepperToVideoTrackAdapter {
 public:
  // Instantiates and adds a new video track to the MediaStream specified by
  // |url|. Returns a handler for delivering frames to the new video track as
  // |frame_writer|.
  // If |registry| is NULL the global blink::WebMediaStreamRegistry will be
  // used to look up the media stream.
  // The caller of the function takes the ownership of |frame_writer|.
  // Returns true on success and false on failure.
  static bool Open(MediaStreamRegistryInterface* registry,
                   const std::string& url,
                   FrameWriterInterface** frame_writer);

 private:
  DISALLOW_COPY_AND_ASSIGN(PepperToVideoTrackAdapter);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_PEPPER_PEPPER_TO_VIDEO_TRACK_ADAPTER_H_
