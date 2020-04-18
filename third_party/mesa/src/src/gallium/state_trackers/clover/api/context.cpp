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
#include "core/context.hpp"

using namespace clover;

PUBLIC cl_context
clCreateContext(const cl_context_properties *props, cl_uint num_devs,
                const cl_device_id *devs,
                void (CL_CALLBACK *pfn_notify)(const char *, const void *,
                                               size_t, void *),
                void *user_data, cl_int *errcode_ret) try {
   auto mprops = property_map(props);

   if (!devs || !num_devs ||
       (!pfn_notify && user_data))
      throw error(CL_INVALID_VALUE);

   if (any_of(is_zero<cl_device_id>(), devs, devs + num_devs))
      throw error(CL_INVALID_DEVICE);

   for (auto p : mprops) {
      if (!(p.first == CL_CONTEXT_PLATFORM &&
            (cl_platform_id)p.second == NULL))
         throw error(CL_INVALID_PROPERTY);
   }

   ret_error(errcode_ret, CL_SUCCESS);
   return new context(
      property_vector(mprops),
      std::vector<cl_device_id>(devs, devs + num_devs));

} catch(error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_context
clCreateContextFromType(const cl_context_properties *props,
                        cl_device_type type,
                        void (CL_CALLBACK *pfn_notify)(
                           const char *, const void *, size_t, void *),
                        void *user_data, cl_int *errcode_ret) {
   cl_device_id dev;
   cl_int ret;

   ret = clGetDeviceIDs(0, type, 1, &dev, 0);
   if (ret) {
      ret_error(errcode_ret, ret);
      return NULL;
   }

   return clCreateContext(props, 1, &dev, pfn_notify, user_data, errcode_ret);
}

PUBLIC cl_int
clRetainContext(cl_context ctx) {
   if (!ctx)
      return CL_INVALID_CONTEXT;

   ctx->retain();
   return CL_SUCCESS;
}

PUBLIC cl_int
clReleaseContext(cl_context ctx) {
   if (!ctx)
      return CL_INVALID_CONTEXT;

   if (ctx->release())
      delete ctx;

   return CL_SUCCESS;
}

PUBLIC cl_int
clGetContextInfo(cl_context ctx, cl_context_info param,
                 size_t size, void *buf, size_t *size_ret) {
   if (!ctx)
      return CL_INVALID_CONTEXT;

   switch (param) {
   case CL_CONTEXT_REFERENCE_COUNT:
      return scalar_property<cl_uint>(buf, size, size_ret, ctx->ref_count());

   case CL_CONTEXT_NUM_DEVICES:
      return scalar_property<cl_uint>(buf, size, size_ret, ctx->devs.size());

   case CL_CONTEXT_DEVICES:
      return vector_property<cl_device_id>(buf, size, size_ret, ctx->devs);

   case CL_CONTEXT_PROPERTIES:
      return vector_property<cl_context_properties>(buf, size, size_ret,
                                                    ctx->props());

   default:
      return CL_INVALID_VALUE;
   }
}
