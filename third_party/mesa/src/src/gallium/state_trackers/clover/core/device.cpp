//
// Copyright 2012 Francisco Jerez
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "core/device.hpp"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

using namespace clover;

namespace {
   template<typename T>
   std::vector<T>
   get_compute_param(pipe_screen *pipe, pipe_compute_cap cap) {
      int sz = pipe->get_compute_param(pipe, cap, NULL);
      std::vector<T> v(sz / sizeof(T));

      pipe->get_compute_param(pipe, cap, &v.front());
      return v;
   }
}

_cl_device_id::_cl_device_id(pipe_loader_device *ldev) : ldev(ldev) {
   pipe = pipe_loader_create_screen(ldev, PIPE_SEARCH_DIR);
   if (!pipe || !pipe->get_param(pipe, PIPE_CAP_COMPUTE))
      throw error(CL_INVALID_DEVICE);
}

_cl_device_id::_cl_device_id(_cl_device_id &&dev) : pipe(dev.pipe), ldev(dev.ldev) {
   dev.ldev = NULL;
   dev.pipe = NULL;
}

_cl_device_id::~_cl_device_id() {
   if (pipe)
      pipe->destroy(pipe);
   if (ldev)
      pipe_loader_release(&ldev, 1);
}

cl_device_type
_cl_device_id::type() const {
   switch (ldev->type) {
   case PIPE_LOADER_DEVICE_SOFTWARE:
      return CL_DEVICE_TYPE_CPU;
   case PIPE_LOADER_DEVICE_PCI:
      return CL_DEVICE_TYPE_GPU;
   default:
      assert(0);
      return 0;
   }
}

cl_uint
_cl_device_id::vendor_id() const {
   switch (ldev->type) {
   case PIPE_LOADER_DEVICE_SOFTWARE:
      return 0;
   case PIPE_LOADER_DEVICE_PCI:
      return ldev->u.pci.vendor_id;
   default:
      assert(0);
      return 0;
   }
}

size_t
_cl_device_id::max_images_read() const {
   return PIPE_MAX_SHADER_RESOURCES;
}

size_t
_cl_device_id::max_images_write() const {
   return PIPE_MAX_SHADER_RESOURCES;
}

cl_uint
_cl_device_id::max_image_levels_2d() const {
   return pipe->get_param(pipe, PIPE_CAP_MAX_TEXTURE_2D_LEVELS);
}

cl_uint
_cl_device_id::max_image_levels_3d() const {
   return pipe->get_param(pipe, PIPE_CAP_MAX_TEXTURE_3D_LEVELS);
}

cl_uint
_cl_device_id::max_samplers() const {
   return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
                                 PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS);
}

cl_ulong
_cl_device_id::max_mem_global() const {
   return get_compute_param<uint64_t>(pipe,
                                      PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE)[0];
}

cl_ulong
_cl_device_id::max_mem_local() const {
   return get_compute_param<uint64_t>(pipe,
                                      PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE)[0];
}

cl_ulong
_cl_device_id::max_mem_input() const {
   return get_compute_param<uint64_t>(pipe,
                                      PIPE_COMPUTE_CAP_MAX_INPUT_SIZE)[0];
}

cl_ulong
_cl_device_id::max_const_buffer_size() const {
   return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
                                 PIPE_SHADER_CAP_MAX_CONSTS) * 16;
}

cl_uint
_cl_device_id::max_const_buffers() const {
   return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
                                 PIPE_SHADER_CAP_MAX_CONST_BUFFERS);
}

size_t
_cl_device_id::max_threads_per_block() const {
   return get_compute_param<uint64_t>(
      pipe, PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK)[0];
}

std::vector<size_t>
_cl_device_id::max_block_size() const {
   auto v = get_compute_param<uint64_t>(pipe, PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE);
   return { v.begin(), v.end() };
}

std::string
_cl_device_id::device_name() const {
   return pipe->get_name(pipe);
}

std::string
_cl_device_id::vendor_name() const {
   return pipe->get_vendor(pipe);
}

enum pipe_shader_ir
_cl_device_id::ir_format() const {
   return (enum pipe_shader_ir) pipe->get_shader_param(pipe,
                                                  PIPE_SHADER_COMPUTE,
                                                  PIPE_SHADER_CAP_PREFERRED_IR);
}

std::string
_cl_device_id::ir_target() const {
   std::vector<char> target = get_compute_param<char>(pipe,
                                                    PIPE_COMPUTE_CAP_IR_TARGET);
   return { target.data() };
}

device_registry::device_registry() {
   int n = pipe_loader_probe(NULL, 0);
   std::vector<pipe_loader_device *> ldevs(n);

   pipe_loader_probe(&ldevs.front(), n);

   for (pipe_loader_device *ldev : ldevs) {
      try {
         devs.emplace_back(ldev);
      } catch (error &) {}
   }
}
