// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/cma_backend_factory_impl.h"

#include "chromecast/media/cma/backend/cma_backend.h"
#include "chromecast/media/cma/backend/media_pipeline_backend_manager.h"
#include "chromecast/public/media/media_pipeline_device_params.h"

namespace chromecast {
namespace media {

CmaBackendFactoryImpl::CmaBackendFactoryImpl(
    MediaPipelineBackendManager* media_pipeline_backend_manager)
    : media_pipeline_backend_manager_(media_pipeline_backend_manager) {
  DCHECK(media_pipeline_backend_manager_);
}

CmaBackendFactoryImpl::~CmaBackendFactoryImpl() = default;

std::unique_ptr<CmaBackend> CmaBackendFactoryImpl::CreateBackend(
    const MediaPipelineDeviceParams& params) {
  return media_pipeline_backend_manager_->CreateMediaPipelineBackend(params);
}

}  // namespace media
}  // namespace chromecast
