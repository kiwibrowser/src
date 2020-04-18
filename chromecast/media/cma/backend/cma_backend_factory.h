// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_CMA_BACKEND_FACTORY_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_CMA_BACKEND_FACTORY_H_

#include <memory>

namespace chromecast {
namespace media {

class CmaBackend;
struct MediaPipelineDeviceParams;

// Abstract base class to create CmaBackend.
class CmaBackendFactory {
 public:
  virtual ~CmaBackendFactory() = default;

  virtual std::unique_ptr<CmaBackend> CreateBackend(
      const MediaPipelineDeviceParams& params) = 0;
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_CMA_BACKEND_FACTORY_H_
