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

#include "api/util.hpp"
#include "core/memory.hpp"
#include "core/format.hpp"

using namespace clover;

PUBLIC cl_mem
clCreateBuffer(cl_context ctx, cl_mem_flags flags, size_t size,
               void *host_ptr, cl_int *errcode_ret) try {
   if (!ctx)
      throw error(CL_INVALID_CONTEXT);

   if (bool(host_ptr) != bool(flags & (CL_MEM_USE_HOST_PTR |
                                       CL_MEM_COPY_HOST_PTR)))
      throw error(CL_INVALID_HOST_PTR);

   if (!size)
      throw error(CL_INVALID_BUFFER_SIZE);

   if (flags & ~(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY |
                 CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR))
      throw error(CL_INVALID_VALUE);

   ret_error(errcode_ret, CL_SUCCESS);
   return new root_buffer(*ctx, flags, size, host_ptr);

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_mem
clCreateSubBuffer(cl_mem obj, cl_mem_flags flags, cl_buffer_create_type op,
                  const void *op_info, cl_int *errcode_ret) try {
   root_buffer *parent = dynamic_cast<root_buffer *>(obj);

   if (!parent)
      throw error(CL_INVALID_MEM_OBJECT);

   if ((flags & (CL_MEM_USE_HOST_PTR |
                 CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR)) ||
       (~flags & parent->flags() & (CL_MEM_READ_ONLY |
                                    CL_MEM_WRITE_ONLY)))
      throw error(CL_INVALID_VALUE);

   if (op == CL_BUFFER_CREATE_TYPE_REGION) {
      const cl_buffer_region *reg = (const cl_buffer_region *)op_info;

      if (!reg ||
          reg->origin > parent->size() ||
          reg->origin + reg->size > parent->size())
         throw error(CL_INVALID_VALUE);

      if (!reg->size)
         throw error(CL_INVALID_BUFFER_SIZE);

      ret_error(errcode_ret, CL_SUCCESS);
      return new sub_buffer(*parent, flags, reg->origin, reg->size);

   } else {
      throw error(CL_INVALID_VALUE);
   }

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_mem
clCreateImage2D(cl_context ctx, cl_mem_flags flags,
                const cl_image_format *format,
                size_t width, size_t height, size_t row_pitch,
                void *host_ptr, cl_int *errcode_ret) try {
   if (!ctx)
      throw error(CL_INVALID_CONTEXT);

   if (flags & ~(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY |
                 CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR))
      throw error(CL_INVALID_VALUE);

   if (!format)
      throw error(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);

   if (width < 1 || height < 1)
      throw error(CL_INVALID_IMAGE_SIZE);

   if (bool(host_ptr) != bool(flags & (CL_MEM_USE_HOST_PTR |
                                       CL_MEM_COPY_HOST_PTR)))
      throw error(CL_INVALID_HOST_PTR);

   if (!supported_formats(ctx, CL_MEM_OBJECT_IMAGE2D).count(*format))
      throw error(CL_IMAGE_FORMAT_NOT_SUPPORTED);

   ret_error(errcode_ret, CL_SUCCESS);
   return new image2d(*ctx, flags, format, width, height,
                      row_pitch, host_ptr);

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_mem
clCreateImage3D(cl_context ctx, cl_mem_flags flags,
                const cl_image_format *format,
                size_t width, size_t height, size_t depth,
                size_t row_pitch, size_t slice_pitch,
                void *host_ptr, cl_int *errcode_ret) try {
   if (!ctx)
      throw error(CL_INVALID_CONTEXT);

   if (flags & ~(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY |
                 CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR))
      throw error(CL_INVALID_VALUE);

   if (!format)
      throw error(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);

   if (width < 1 || height < 1 || depth < 2)
      throw error(CL_INVALID_IMAGE_SIZE);

   if (bool(host_ptr) != bool(flags & (CL_MEM_USE_HOST_PTR |
                                       CL_MEM_COPY_HOST_PTR)))
      throw error(CL_INVALID_HOST_PTR);

   if (!supported_formats(ctx, CL_MEM_OBJECT_IMAGE3D).count(*format))
      throw error(CL_IMAGE_FORMAT_NOT_SUPPORTED);

   ret_error(errcode_ret, CL_SUCCESS);
   return new image3d(*ctx, flags, format, width, height, depth,
                      row_pitch, slice_pitch, host_ptr);

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_int
clGetSupportedImageFormats(cl_context ctx, cl_mem_flags flags,
                           cl_mem_object_type type, cl_uint count,
                           cl_image_format *buf, cl_uint *count_ret) try {
   if (!ctx)
      throw error(CL_INVALID_CONTEXT);

   if (flags & ~(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY |
                 CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR))
      throw error(CL_INVALID_VALUE);

   if (!count && buf)
      throw error(CL_INVALID_VALUE);

   auto formats = supported_formats(ctx, type);

   if (buf)
      std::copy_n(formats.begin(), std::min((cl_uint)formats.size(), count),
                  buf);
   if (count_ret)
      *count_ret = formats.size();

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clGetMemObjectInfo(cl_mem obj, cl_mem_info param,
                   size_t size, void *buf, size_t *size_ret) {
   if (!obj)
      return CL_INVALID_MEM_OBJECT;

   switch (param) {
   case CL_MEM_TYPE:
      return scalar_property<cl_mem_object_type>(buf, size, size_ret,
                                                 obj->type());

   case CL_MEM_FLAGS:
      return scalar_property<cl_mem_flags>(buf, size, size_ret, obj->flags());

   case CL_MEM_SIZE:
      return scalar_property<size_t>(buf, size, size_ret, obj->size());

   case CL_MEM_HOST_PTR:
      return scalar_property<void *>(buf, size, size_ret, obj->host_ptr());

   case CL_MEM_MAP_COUNT:
      return scalar_property<cl_uint>(buf, size, size_ret, 0);

   case CL_MEM_REFERENCE_COUNT:
      return scalar_property<cl_uint>(buf, size, size_ret, obj->ref_count());

   case CL_MEM_CONTEXT:
      return scalar_property<cl_context>(buf, size, size_ret, &obj->ctx);

   case CL_MEM_ASSOCIATED_MEMOBJECT: {
      sub_buffer *sub = dynamic_cast<sub_buffer *>(obj);
      return scalar_property<cl_mem>(buf, size, size_ret,
                                     (sub ? &sub->parent : NULL));
   }
   case CL_MEM_OFFSET: {
      sub_buffer *sub = dynamic_cast<sub_buffer *>(obj);
      return scalar_property<size_t>(buf, size, size_ret,
                                     (sub ? sub->offset() : 0));
   }
   default:
      return CL_INVALID_VALUE;
   }
}

PUBLIC cl_int
clGetImageInfo(cl_mem obj, cl_image_info param,
               size_t size, void *buf, size_t *size_ret) {
   image *img = dynamic_cast<image *>(obj);
   if (!img)
      return CL_INVALID_MEM_OBJECT;

   switch (param) {
   case CL_IMAGE_FORMAT:
      return scalar_property<cl_image_format>(buf, size, size_ret,
                                              img->format());

   case CL_IMAGE_ELEMENT_SIZE:
      return scalar_property<size_t>(buf, size, size_ret, 0);

   case CL_IMAGE_ROW_PITCH:
      return scalar_property<size_t>(buf, size, size_ret, img->row_pitch());

   case CL_IMAGE_SLICE_PITCH:
      return scalar_property<size_t>(buf, size, size_ret, img->slice_pitch());

   case CL_IMAGE_WIDTH:
      return scalar_property<size_t>(buf, size, size_ret, img->width());

   case CL_IMAGE_HEIGHT:
      return scalar_property<size_t>(buf, size, size_ret, img->height());

   case CL_IMAGE_DEPTH:
      return scalar_property<size_t>(buf, size, size_ret, img->depth());

   default:
      return CL_INVALID_VALUE;
   }
}

PUBLIC cl_int
clRetainMemObject(cl_mem obj) {
   if (!obj)
      return CL_INVALID_MEM_OBJECT;

   obj->retain();
   return CL_SUCCESS;
}

PUBLIC cl_int
clReleaseMemObject(cl_mem obj) {
   if (!obj)
      return CL_INVALID_MEM_OBJECT;

   if (obj->release())
      delete obj;

   return CL_SUCCESS;
}

PUBLIC cl_int
clSetMemObjectDestructorCallback(cl_mem obj,
                                 void (CL_CALLBACK *pfn_notify)(cl_mem, void *),
                                 void *user_data) {
   if (!obj)
      return CL_INVALID_MEM_OBJECT;

   if (!pfn_notify)
      return CL_INVALID_VALUE;

   obj->destroy_notify([=]{ pfn_notify(obj, user_data); });

   return CL_SUCCESS;
}
