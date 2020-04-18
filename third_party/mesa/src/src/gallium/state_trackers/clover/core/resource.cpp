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

#include "core/resource.hpp"
#include "pipe/p_screen.h"
#include "util/u_sampler.h"
#include "util/u_format.h"

using namespace clover;

namespace {
   class box {
   public:
      box(const resource::point &origin, const resource::point &size) :
         pipe({ (unsigned)origin[0], (unsigned)origin[1],
                (unsigned)origin[2], (unsigned)size[0],
                (unsigned)size[1], (unsigned)size[2] }) {
      }

      operator const pipe_box *() {
         return &pipe;
      }

   protected:
      pipe_box pipe;
   };
}

resource::resource(clover::device &dev, clover::memory_obj &obj) :
   dev(dev), obj(obj), pipe(NULL), offset{0} {
}

resource::~resource() {
}

void
resource::copy(command_queue &q, const point &origin, const point &region,
               resource &src_res, const point &src_origin) {
   point p = offset + origin;

   q.pipe->resource_copy_region(q.pipe, pipe, 0, p[0], p[1], p[2],
                                src_res.pipe, 0,
                                box(src_res.offset + src_origin, region));
}

void *
resource::add_map(command_queue &q, cl_map_flags flags, bool blocking,
                  const point &origin, const point &region) {
   maps.emplace_back(q, *this, flags, blocking, origin, region);
   return maps.back();
}

void
resource::del_map(void *p) {
   auto it = std::find(maps.begin(), maps.end(), p);
   if (it != maps.end())
      maps.erase(it);
}

unsigned
resource::map_count() const {
   return maps.size();
}

pipe_sampler_view *
resource::bind_sampler_view(clover::command_queue &q) {
   pipe_sampler_view info;

   u_sampler_view_default_template(&info, pipe, pipe->format);
   return q.pipe->create_sampler_view(q.pipe, pipe, &info);
}

void
resource::unbind_sampler_view(clover::command_queue &q,
                              pipe_sampler_view *st) {
   q.pipe->sampler_view_destroy(q.pipe, st);
}

pipe_surface *
resource::bind_surface(clover::command_queue &q, bool rw) {
   pipe_surface info {};

   info.format = pipe->format;
   info.usage = pipe->bind;
   info.writable = rw;

   if (pipe->target == PIPE_BUFFER)
      info.u.buf.last_element = pipe->width0 - 1;

   return q.pipe->create_surface(q.pipe, pipe, &info);
}

void
resource::unbind_surface(clover::command_queue &q, pipe_surface *st) {
   q.pipe->surface_destroy(q.pipe, st);
}

root_resource::root_resource(clover::device &dev, clover::memory_obj &obj,
                             clover::command_queue &q,
                             const std::string &data) :
   resource(dev, obj) {
   pipe_resource info {};

   if (image *img = dynamic_cast<image *>(&obj)) {
      info.format = translate_format(img->format());
      info.width0 = img->width();
      info.height0 = img->height();
      info.depth0 = img->depth();
   } else {
      info.width0 = obj.size();
      info.height0 = 1;
      info.depth0 = 1;
   }

   info.target = translate_target(obj.type());
   info.bind = (PIPE_BIND_SAMPLER_VIEW |
                PIPE_BIND_COMPUTE_RESOURCE |
                PIPE_BIND_GLOBAL |
                PIPE_BIND_TRANSFER_READ |
                PIPE_BIND_TRANSFER_WRITE);

   pipe = dev.pipe->resource_create(dev.pipe, &info);
   if (!pipe)
      throw error(CL_OUT_OF_RESOURCES);

   if (!data.empty()) {
      box rect { { 0, 0, 0 }, { info.width0, info.height0, info.depth0 } };
      unsigned cpp = util_format_get_blocksize(info.format);

      q.pipe->transfer_inline_write(q.pipe, pipe, 0, PIPE_TRANSFER_WRITE,
                                    rect, data.data(), cpp * info.width0,
                                    cpp * info.width0 * info.height0);
   }
}

root_resource::root_resource(clover::device &dev, clover::memory_obj &obj,
                             clover::root_resource &r) :
   resource(dev, obj) {
   assert(0); // XXX -- resource shared among dev and r.dev
}

root_resource::~root_resource() {
   dev.pipe->resource_destroy(dev.pipe, pipe);
}

sub_resource::sub_resource(clover::resource &r, point offset) :
   resource(r.dev, r.obj) {
   pipe = r.pipe;
   offset = r.offset + offset;
}

mapping::mapping(command_queue &q, resource &r,
                 cl_map_flags flags, bool blocking,
                 const resource::point &origin,
                 const resource::point &region) :
   pctx(q.pipe) {
   unsigned usage = ((flags & CL_MAP_WRITE ? PIPE_TRANSFER_WRITE : 0 ) |
                     (flags & CL_MAP_READ ? PIPE_TRANSFER_READ : 0 ) |
                     (blocking ? PIPE_TRANSFER_UNSYNCHRONIZED : 0));

   pxfer = pctx->get_transfer(pctx, r.pipe, 0, usage,
                              box(origin + r.offset, region));
   if (!pxfer)
      throw error(CL_OUT_OF_RESOURCES);

   p = pctx->transfer_map(pctx, pxfer);
   if (!p) {
      pctx->transfer_destroy(pctx, pxfer);
      throw error(CL_OUT_OF_RESOURCES);
   }
}

mapping::mapping(mapping &&m) :
   pctx(m.pctx), pxfer(m.pxfer), p(m.p) {
   m.p = NULL;
   m.pxfer = NULL;
}

mapping::~mapping() {
   if (pxfer) {
      pctx->transfer_unmap(pctx, pxfer);
      pctx->transfer_destroy(pctx, pxfer);
   }
}
