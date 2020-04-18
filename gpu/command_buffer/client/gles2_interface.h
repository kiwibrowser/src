// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_GLES2_INTERFACE_H_
#define GPU_COMMAND_BUFFER_CLIENT_GLES2_INTERFACE_H_

#include <GLES2/gl2.h>

#include "base/compiler_specific.h"

namespace cc {
class ClientTransferCacheEntry;
class DisplayItemList;
class ImageProvider;
}  // namespace cc

namespace gfx {
class Rect;
class Vector2d;
class Vector2dF;
}  // namespace gfx

extern "C" typedef struct _ClientBuffer* ClientBuffer;
extern "C" typedef struct _GLColorSpace* GLColorSpace;
extern "C" typedef struct _ClientGpuFence* ClientGpuFence;

namespace gpu {
namespace gles2 {

// This class is the interface for all client side GL functions.
class GLES2Interface {
 public:
  GLES2Interface() = default;
  virtual ~GLES2Interface() = default;

  virtual void FreeSharedMemory(void*) {};

  // Include the auto-generated part of this class. We split this because
  // it means we can easily edit the non-auto generated parts right here in
  // this file instead of having to edit some template or the code generator.
  #include "gpu/command_buffer/client/gles2_interface_autogen.h"
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_GLES2_INTERFACE_H_
