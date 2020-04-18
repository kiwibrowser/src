// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_PIPELINE_VIDEO_PIPELINE_CLIENT_H_
#define CHROMECAST_MEDIA_CMA_PIPELINE_VIDEO_PIPELINE_CLIENT_H_

#include "base/callback.h"
#include "chromecast/media/cma/pipeline/av_pipeline_client.h"

namespace gfx {
class Size;
}

namespace chromecast {
namespace media {

struct VideoPipelineClient {
  typedef base::Callback<void(
      const gfx::Size& natural_size)> NaturalSizeChangedCB;

  VideoPipelineClient();
  VideoPipelineClient(const VideoPipelineClient& other);
  ~VideoPipelineClient();

  // All the default callbacks.
  AvPipelineClient av_pipeline_client;

  // Video resolution change notification.
  NaturalSizeChangedCB natural_size_changed_cb;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_PIPELINE_VIDEO_PIPELINE_CLIENT_H_
