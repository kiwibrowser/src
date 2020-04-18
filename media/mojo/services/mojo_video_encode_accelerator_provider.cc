// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_video_encode_accelerator_provider.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/limits.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

// static
void MojoVideoEncodeAcceleratorProvider::Create(
    mojom::VideoEncodeAcceleratorProviderRequest request,
    const CreateAndInitializeVideoEncodeAcceleratorCallback&
        create_vea_callback,
    const gpu::GpuPreferences& gpu_preferences) {
  mojo::MakeStrongBinding(std::make_unique<MojoVideoEncodeAcceleratorProvider>(
                              create_vea_callback, gpu_preferences),
                          std::move(request));
}

MojoVideoEncodeAcceleratorProvider::MojoVideoEncodeAcceleratorProvider(
    const CreateAndInitializeVideoEncodeAcceleratorCallback&
        create_vea_callback,
    const gpu::GpuPreferences& gpu_preferences)
    : create_vea_callback_(create_vea_callback),
      gpu_preferences_(gpu_preferences) {}

MojoVideoEncodeAcceleratorProvider::~MojoVideoEncodeAcceleratorProvider() =
    default;

void MojoVideoEncodeAcceleratorProvider::CreateVideoEncodeAccelerator(
    mojom::VideoEncodeAcceleratorRequest request) {
  MojoVideoEncodeAcceleratorService::Create(
      std::move(request), create_vea_callback_, gpu_preferences_);
}

}  // namespace media
