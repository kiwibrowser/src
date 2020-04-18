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

#include <cstring>

#include "api/util.hpp"
#include "core/event.hpp"
#include "core/resource.hpp"

using namespace clover;

namespace {
   typedef resource::point point;

   ///
   /// Common argument checking shared by memory transfer commands.
   ///
   void
   validate_base(cl_command_queue q, cl_uint num_deps, const cl_event *deps) {
      if (!q)
         throw error(CL_INVALID_COMMAND_QUEUE);

      if (bool(num_deps) != bool(deps) ||
          any_of(is_zero<cl_event>(), deps, deps + num_deps))
         throw error(CL_INVALID_EVENT_WAIT_LIST);

      if (any_of([&](const cl_event ev) {
               return &ev->ctx != &q->ctx;
            }, deps, deps + num_deps))
         throw error(CL_INVALID_CONTEXT);
   }

   ///
   /// Memory object-specific argument checking shared by most memory
   /// transfer commands.
   ///
   void
   validate_obj(cl_command_queue q, cl_mem obj) {
      if (!obj)
         throw error(CL_INVALID_MEM_OBJECT);

      if (&obj->ctx != &q->ctx)
         throw error(CL_INVALID_CONTEXT);
   }

   ///
   /// Class that encapsulates the task of mapping an object of type
   /// \a T.  The return value of get() should be implicitly
   /// convertible to \a void *.
   ///
   template<typename T> struct __map;

   template<> struct __map<void *> {
      static void *
      get(cl_command_queue q, void *obj, cl_map_flags flags,
          size_t offset, size_t size) {
         return (char *)obj + offset;
      }
   };

   template<> struct __map<const void *> {
      static const void *
      get(cl_command_queue q, const void *obj, cl_map_flags flags,
          size_t offset, size_t size) {
         return (const char *)obj + offset;
      }
   };

   template<> struct __map<memory_obj *> {
      static mapping
      get(cl_command_queue q, memory_obj *obj, cl_map_flags flags,
          size_t offset, size_t size) {
         return { *q, obj->resource(q), flags, true, { offset }, { size }};
      }
   };

   ///
   /// Software copy from \a src_obj to \a dst_obj.  They can be
   /// either pointers or memory objects.
   ///
   template<typename T, typename S>
   std::function<void (event &)>
   soft_copy_op(cl_command_queue q,
                T dst_obj, const point &dst_orig, const point &dst_pitch,
                S src_obj, const point &src_orig, const point &src_pitch,
                const point &region) {
      return [=](event &) {
         auto dst = __map<T>::get(q, dst_obj, CL_MAP_WRITE,
                                  dst_pitch(dst_orig), dst_pitch(region));
         auto src = __map<S>::get(q, src_obj, CL_MAP_READ,
                                  src_pitch(src_orig), src_pitch(region));
         point p;

         for (p[2] = 0; p[2] < region[2]; ++p[2]) {
            for (p[1] = 0; p[1] < region[1]; ++p[1]) {
               std::memcpy(static_cast<char *>(dst) + dst_pitch(p),
                           static_cast<const char *>(src) + src_pitch(p),
                           src_pitch[0] * region[0]);
            }
         }
      };
   }

   ///
   /// Hardware copy from \a src_obj to \a dst_obj.
   ///
   template<typename T, typename S>
   std::function<void (event &)>
   hard_copy_op(cl_command_queue q, T dst_obj, const point &dst_orig,
                S src_obj, const point &src_orig, const point &region) {
      return [=](event &) {
         dst_obj->resource(q).copy(*q, dst_orig, region,
                                   src_obj->resource(q), src_orig);
      };
   }
}

PUBLIC cl_int
clEnqueueReadBuffer(cl_command_queue q, cl_mem obj, cl_bool blocking,
                    size_t offset, size_t size, void *ptr,
                    cl_uint num_deps, const cl_event *deps,
                    cl_event *ev) try {
   validate_base(q, num_deps, deps);
   validate_obj(q, obj);

   if (!ptr || offset > obj->size() || offset + size > obj->size())
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_READ_BUFFER, { deps, deps + num_deps },
      soft_copy_op(q,
                   ptr, { 0 }, { 1 },
                   obj, { offset }, { 1 },
                   { size, 1, 1 }));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueWriteBuffer(cl_command_queue q, cl_mem obj, cl_bool blocking,
                     size_t offset, size_t size, const void *ptr,
                     cl_uint num_deps, const cl_event *deps,
                     cl_event *ev) try {
   validate_base(q, num_deps, deps);
   validate_obj(q, obj);

   if (!ptr || offset > obj->size() || offset + size > obj->size())
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_WRITE_BUFFER, { deps, deps + num_deps },
      soft_copy_op(q,
                   obj, { offset }, { 1 },
                   ptr, { 0 }, { 1 },
                   { size, 1, 1 }));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueReadBufferRect(cl_command_queue q, cl_mem obj, cl_bool blocking,
                        const size_t *obj_origin, const size_t *host_origin,
                        const size_t *region,
                        size_t obj_row_pitch, size_t obj_slice_pitch,
                        size_t host_row_pitch, size_t host_slice_pitch,
                        void *ptr,
                        cl_uint num_deps, const cl_event *deps,
                        cl_event *ev) try {
   validate_base(q, num_deps, deps);
   validate_obj(q, obj);

   if (!ptr)
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_READ_BUFFER_RECT, { deps, deps + num_deps },
      soft_copy_op(q,
                   ptr, host_origin,
                   { 1, host_row_pitch, host_slice_pitch },
                   obj, obj_origin,
                   { 1, obj_row_pitch, obj_slice_pitch },
                   region));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueWriteBufferRect(cl_command_queue q, cl_mem obj, cl_bool blocking,
                         const size_t *obj_origin, const size_t *host_origin,
                         const size_t *region,
                         size_t obj_row_pitch, size_t obj_slice_pitch,
                         size_t host_row_pitch, size_t host_slice_pitch,
                         const void *ptr,
                         cl_uint num_deps, const cl_event *deps,
                         cl_event *ev) try {
   validate_base(q, num_deps, deps);
   validate_obj(q, obj);

   if (!ptr)
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_WRITE_BUFFER_RECT, { deps, deps + num_deps },
      soft_copy_op(q,
                   obj, obj_origin,
                   { 1, obj_row_pitch, obj_slice_pitch },
                   ptr, host_origin,
                   { 1, host_row_pitch, host_slice_pitch },
                   region));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueCopyBuffer(cl_command_queue q, cl_mem src_obj, cl_mem dst_obj,
                    size_t src_offset, size_t dst_offset, size_t size,
                    cl_uint num_deps, const cl_event *deps,
                    cl_event *ev) try {
   validate_base(q, num_deps, deps);
   validate_obj(q, src_obj);
   validate_obj(q, dst_obj);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_COPY_BUFFER, { deps, deps + num_deps },
      hard_copy_op(q, dst_obj, { dst_offset },
                   src_obj, { src_offset },
                   { size, 1, 1 }));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueCopyBufferRect(cl_command_queue q, cl_mem src_obj, cl_mem dst_obj,
                        const size_t *src_origin, const size_t *dst_origin,
                        const size_t *region,
                        size_t src_row_pitch, size_t src_slice_pitch,
                        size_t dst_row_pitch, size_t dst_slice_pitch,
                        cl_uint num_deps, const cl_event *deps,
                        cl_event *ev) try {
   validate_base(q, num_deps, deps);
   validate_obj(q, src_obj);
   validate_obj(q, dst_obj);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_COPY_BUFFER_RECT, { deps, deps + num_deps },
      soft_copy_op(q,
                   dst_obj, dst_origin,
                   { 1, dst_row_pitch, dst_slice_pitch },
                   src_obj, src_origin,
                   { 1, src_row_pitch, src_slice_pitch },
                   region));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueReadImage(cl_command_queue q, cl_mem obj, cl_bool blocking,
                   const size_t *origin, const size_t *region,
                   size_t row_pitch, size_t slice_pitch, void *ptr,
                   cl_uint num_deps, const cl_event *deps,
                   cl_event *ev) try {
   image *img = dynamic_cast<image *>(obj);

   validate_base(q, num_deps, deps);
   validate_obj(q, img);

   if (!ptr)
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_READ_IMAGE, { deps, deps + num_deps },
      soft_copy_op(q,
                   ptr, {},
                   { 1, row_pitch, slice_pitch },
                   obj, origin,
                   { 1, img->row_pitch(), img->slice_pitch() },
                   region));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueWriteImage(cl_command_queue q, cl_mem obj, cl_bool blocking,
                    const size_t *origin, const size_t *region,
                    size_t row_pitch, size_t slice_pitch, const void *ptr,
                    cl_uint num_deps, const cl_event *deps,
                    cl_event *ev) try {
   image *img = dynamic_cast<image *>(obj);

   validate_base(q, num_deps, deps);
   validate_obj(q, img);

   if (!ptr)
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_WRITE_IMAGE, { deps, deps + num_deps },
      soft_copy_op(q,
                   obj, origin,
                   { 1, img->row_pitch(), img->slice_pitch() },
                   ptr, {},
                   { 1, row_pitch, slice_pitch },
                   region));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueCopyImage(cl_command_queue q, cl_mem src_obj, cl_mem dst_obj,
                   const size_t *src_origin, const size_t *dst_origin,
                   const size_t *region,
                   cl_uint num_deps, const cl_event *deps,
                   cl_event *ev) try {
   image *src_img = dynamic_cast<image *>(src_obj);
   image *dst_img = dynamic_cast<image *>(dst_obj);

   validate_base(q, num_deps, deps);
   validate_obj(q, src_img);
   validate_obj(q, dst_img);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_COPY_IMAGE, { deps, deps + num_deps },
      hard_copy_op(q, dst_obj, dst_origin, src_obj, src_origin, region));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueCopyImageToBuffer(cl_command_queue q, cl_mem src_obj, cl_mem dst_obj,
                           const size_t *src_origin, const size_t *region,
                           size_t dst_offset,
                           cl_uint num_deps, const cl_event *deps,
                           cl_event *ev) try {
   image *src_img = dynamic_cast<image *>(src_obj);

   validate_base(q, num_deps, deps);
   validate_obj(q, src_img);
   validate_obj(q, dst_obj);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_COPY_IMAGE_TO_BUFFER, { deps, deps + num_deps },
      soft_copy_op(q,
                   dst_obj, { dst_offset },
                   { 0, 0, 0 },
                   src_obj, src_origin,
                   { 1, src_img->row_pitch(), src_img->slice_pitch() },
                   region));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueCopyBufferToImage(cl_command_queue q, cl_mem src_obj, cl_mem dst_obj,
                           size_t src_offset,
                           const size_t *dst_origin, const size_t *region,
                           cl_uint num_deps, const cl_event *deps,
                           cl_event *ev) try {
   image *dst_img = dynamic_cast<image *>(src_obj);

   validate_base(q, num_deps, deps);
   validate_obj(q, src_obj);
   validate_obj(q, dst_img);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_COPY_BUFFER_TO_IMAGE, { deps, deps + num_deps },
      soft_copy_op(q,
                   dst_obj, dst_origin,
                   { 1, dst_img->row_pitch(), dst_img->slice_pitch() },
                   src_obj, { src_offset },
                   { 0, 0, 0 },
                   region));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC void *
clEnqueueMapBuffer(cl_command_queue q, cl_mem obj, cl_bool blocking,
                   cl_map_flags flags, size_t offset, size_t size,
                   cl_uint num_deps, const cl_event *deps,
                   cl_event *ev, cl_int *errcode_ret) try {
   validate_base(q, num_deps, deps);
   validate_obj(q, obj);

   if (offset > obj->size() || offset + size > obj->size())
      throw error(CL_INVALID_VALUE);

   void *map = obj->resource(q).add_map(
      *q, flags, blocking, { offset }, { size });

   ret_object(ev, new hard_event(*q, CL_COMMAND_MAP_BUFFER,
                                 { deps, deps + num_deps }));
   ret_error(errcode_ret, CL_SUCCESS);
   return map;

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC void *
clEnqueueMapImage(cl_command_queue q, cl_mem obj, cl_bool blocking,
                  cl_map_flags flags,
                  const size_t *origin, const size_t *region,
                  size_t *row_pitch, size_t *slice_pitch,
                  cl_uint num_deps, const cl_event *deps,
                  cl_event *ev, cl_int *errcode_ret) try {
   image *img = dynamic_cast<image *>(obj);

   validate_base(q, num_deps, deps);
   validate_obj(q, img);

   void *map = obj->resource(q).add_map(
      *q, flags, blocking, origin, region);

   ret_object(ev, new hard_event(*q, CL_COMMAND_MAP_IMAGE,
                                 { deps, deps + num_deps }));
   ret_error(errcode_ret, CL_SUCCESS);
   return map;

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_int
clEnqueueUnmapMemObject(cl_command_queue q, cl_mem obj, void *ptr,
                        cl_uint num_deps, const cl_event *deps,
                        cl_event *ev) try {
   validate_base(q, num_deps, deps);
   validate_obj(q, obj);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_UNMAP_MEM_OBJECT, { deps, deps + num_deps },
      [=](event &) {
         obj->resource(q).del_map(ptr);
      });

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}
