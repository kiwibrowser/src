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

#include "core/memory.hpp"
#include "core/resource.hpp"

using namespace clover;

_cl_mem::_cl_mem(clover::context &ctx, cl_mem_flags flags,
                 size_t size, void *host_ptr) :
   ctx(ctx), __flags(flags),
   __size(size), __host_ptr(host_ptr),
   __destroy_notify([]{}) {
   if (flags & CL_MEM_COPY_HOST_PTR)
      data.append((char *)host_ptr, size);
}

_cl_mem::~_cl_mem() {
   __destroy_notify();
}

void
_cl_mem::destroy_notify(std::function<void ()> f) {
   __destroy_notify = f;
}

cl_mem_flags
_cl_mem::flags() const {
   return __flags;
}

size_t
_cl_mem::size() const {
   return __size;
}

void *
_cl_mem::host_ptr() const {
   return __host_ptr;
}

buffer::buffer(clover::context &ctx, cl_mem_flags flags,
               size_t size, void *host_ptr) :
   memory_obj(ctx, flags, size, host_ptr) {
}

cl_mem_object_type
buffer::type() const {
   return CL_MEM_OBJECT_BUFFER;
}

root_buffer::root_buffer(clover::context &ctx, cl_mem_flags flags,
                         size_t size, void *host_ptr) :
   buffer(ctx, flags, size, host_ptr) {
}

clover::resource &
root_buffer::resource(cl_command_queue q) {
   // Create a new resource if there's none for this device yet.
   if (!resources.count(&q->dev)) {
      auto r = (!resources.empty() ?
                new root_resource(q->dev, *this, *resources.begin()->second) :
                new root_resource(q->dev, *this, *q, data));

      resources.insert(std::make_pair(&q->dev,
                                      std::unique_ptr<root_resource>(r)));
      data.clear();
   }

   return *resources.find(&q->dev)->second;
}

sub_buffer::sub_buffer(clover::root_buffer &parent, cl_mem_flags flags,
                       size_t offset, size_t size) :
   buffer(parent.ctx, flags, size,
          (char *)parent.host_ptr() + offset),
   parent(parent), __offset(offset) {
}

clover::resource &
sub_buffer::resource(cl_command_queue q) {
   // Create a new resource if there's none for this device yet.
   if (!resources.count(&q->dev)) {
      auto r = new sub_resource(parent.resource(q), { offset() });

      resources.insert(std::make_pair(&q->dev,
                                      std::unique_ptr<sub_resource>(r)));
   }

   return *resources.find(&q->dev)->second;
}

size_t
sub_buffer::offset() const {
   return __offset;
}

image::image(clover::context &ctx, cl_mem_flags flags,
             const cl_image_format *format,
             size_t width, size_t height, size_t depth,
             size_t row_pitch, size_t slice_pitch, size_t size,
             void *host_ptr) :
   memory_obj(ctx, flags, size, host_ptr),
   __format(*format), __width(width), __height(height), __depth(depth),
   __row_pitch(row_pitch), __slice_pitch(slice_pitch) {
}

clover::resource &
image::resource(cl_command_queue q) {
   // Create a new resource if there's none for this device yet.
   if (!resources.count(&q->dev)) {
      auto r = (!resources.empty() ?
                new root_resource(q->dev, *this, *resources.begin()->second) :
                new root_resource(q->dev, *this, *q, data));

      resources.insert(std::make_pair(&q->dev,
                                      std::unique_ptr<root_resource>(r)));
      data.clear();
   }

   return *resources.find(&q->dev)->second;
}

cl_image_format
image::format() const {
   return __format;
}

size_t
image::width() const {
   return __width;
}

size_t
image::height() const {
   return __height;
}

size_t
image::depth() const {
   return __depth;
}

size_t
image::row_pitch() const {
   return __row_pitch;
}

size_t
image::slice_pitch() const {
   return __slice_pitch;
}

image2d::image2d(clover::context &ctx, cl_mem_flags flags,
                 const cl_image_format *format, size_t width,
                 size_t height, size_t row_pitch,
                 void *host_ptr) :
   image(ctx, flags, format, width, height, 0,
         row_pitch, 0, height * row_pitch, host_ptr) {
}

cl_mem_object_type
image2d::type() const {
   return CL_MEM_OBJECT_IMAGE2D;
}

image3d::image3d(clover::context &ctx, cl_mem_flags flags,
                 const cl_image_format *format,
                 size_t width, size_t height, size_t depth,
                 size_t row_pitch, size_t slice_pitch,
                 void *host_ptr) :
   image(ctx, flags, format, width, height, depth,
         row_pitch, slice_pitch, depth * slice_pitch,
         host_ptr) {
}

cl_mem_object_type
image3d::type() const {
   return CL_MEM_OBJECT_IMAGE3D;
}
