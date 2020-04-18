// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SERVICE_UTILS_H_
#define GPU_COMMAND_BUFFER_SERVICE_SERVICE_UTILS_H_

#include "base/command_line.h"
#include "gpu/gpu_gles2_export.h"
#include "ui/gl/gl_context.h"

namespace gpu {
struct ContextCreationAttribs;
struct GpuPreferences;

namespace gles2 {
class ContextGroup;

GPU_GLES2_EXPORT gl::GLContextAttribs GenerateGLContextAttribs(
    const ContextCreationAttribs& attribs_helper,
    const ContextGroup* context_group);

// Returns true if the passthrough command decoder has been requested
GPU_GLES2_EXPORT bool UsePassthroughCommandDecoder(
    const base::CommandLine* command_line);

// Returns true if the driver supports creating passthrough command decoders
GPU_GLES2_EXPORT bool PassthroughCommandDecoderSupported();

GPU_GLES2_EXPORT GpuPreferences
ParseGpuPreferences(const base::CommandLine* command_line);

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_SERVICE_UTILS_H_
