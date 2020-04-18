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

#ifndef __CORE_DEVICE_HPP__
#define __CORE_DEVICE_HPP__

#include <set>
#include <vector>

#include "core/base.hpp"
#include "core/format.hpp"
#include "pipe-loader/pipe_loader.h"

namespace clover {
   typedef struct _cl_device_id device;
   class root_resource;
   class hard_event;
}

struct _cl_device_id {
public:
   _cl_device_id(pipe_loader_device *ldev);
   _cl_device_id(_cl_device_id &&dev);
   _cl_device_id(const _cl_device_id &dev) = delete;
   ~_cl_device_id();

   cl_device_type type() const;
   cl_uint vendor_id() const;
   size_t max_images_read() const;
   size_t max_images_write() const;
   cl_uint max_image_levels_2d() const;
   cl_uint max_image_levels_3d() const;
   cl_uint max_samplers() const;
   cl_ulong max_mem_global() const;
   cl_ulong max_mem_local() const;
   cl_ulong max_mem_input() const;
   cl_ulong max_const_buffer_size() const;
   cl_uint max_const_buffers() const;
   size_t max_threads_per_block() const;

   std::vector<size_t> max_block_size() const;
   std::string device_name() const;
   std::string vendor_name() const;
   enum pipe_shader_ir ir_format() const;
   std::string ir_target() const;

   friend struct _cl_command_queue;
   friend class clover::root_resource;
   friend class clover::hard_event;
   friend std::set<cl_image_format>
   clover::supported_formats(cl_context, cl_mem_object_type);

private:
   pipe_screen *pipe;
   pipe_loader_device *ldev;
};

namespace clover {
   ///
   /// Container of all the compute devices that are available in the
   /// system.
   ///
   class device_registry {
   public:
      typedef std::vector<device>::iterator iterator;

      device_registry();

      iterator begin() {
         return devs.begin();
      }

      iterator end() {
         return devs.end();
      }

      device &front() {
         return devs.front();
      }

      device &back() {
         return devs.back();
      }

   protected:
      std::vector<device> devs;
   };
}

#endif
