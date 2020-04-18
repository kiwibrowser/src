// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/gpu_jpeg_encode_accelerator_factory.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "media/base/media_switches.h"
#include "media/gpu/buildflags.h"

#if BUILDFLAG(USE_V4L2_CODEC) && defined(ARCH_CPU_ARM_FAMILY)
#define USE_V4L2_JEA
#endif

#if BUILDFLAG(USE_VAAPI)
#include "media/gpu/vaapi/vaapi_jpeg_encode_accelerator.h"
#endif

namespace media {

namespace {

#if BUILDFLAG(USE_VAAPI)
std::unique_ptr<JpegEncodeAccelerator> CreateVaapiJEA(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  return std::make_unique<VaapiJpegEncodeAccelerator>(
      std::move(io_task_runner));
}
#endif

}  // namespace

// static
bool GpuJpegEncodeAcceleratorFactory::IsAcceleratedJpegEncodeSupported() {
  auto accelerator_factory_functions = GetAcceleratorFactories();
  return !accelerator_factory_functions.empty();
}

// static
std::vector<GpuJpegEncodeAcceleratorFactory::CreateAcceleratorCB>
GpuJpegEncodeAcceleratorFactory::GetAcceleratorFactories() {
  // This list is ordered by priority of use.
  std::vector<CreateAcceleratorCB> result;
#if defined(USE_V4L2_JEA)
// TODO(mojahsu): Add CreateV4l2JEA here.
#endif
#if BUILDFLAG(USE_VAAPI)
  result.push_back(base::BindRepeating(&CreateVaapiJEA));
#endif
  return result;
}

}  // namespace media
