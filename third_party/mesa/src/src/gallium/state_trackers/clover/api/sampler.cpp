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
#include "core/sampler.hpp"

using namespace clover;

PUBLIC cl_sampler
clCreateSampler(cl_context ctx, cl_bool norm_mode,
                cl_addressing_mode addr_mode, cl_filter_mode filter_mode,
                cl_int *errcode_ret) try {
   if (!ctx)
      throw error(CL_INVALID_CONTEXT);

   ret_error(errcode_ret, CL_SUCCESS);
   return new sampler(*ctx, norm_mode, addr_mode, filter_mode);

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_int
clRetainSampler(cl_sampler s) {
   if (!s)
      throw error(CL_INVALID_SAMPLER);

   s->retain();
   return CL_SUCCESS;
}

PUBLIC cl_int
clReleaseSampler(cl_sampler s) {
   if (!s)
      throw error(CL_INVALID_SAMPLER);

   if (s->release())
      delete s;

   return CL_SUCCESS;
}

PUBLIC cl_int
clGetSamplerInfo(cl_sampler s, cl_sampler_info param,
                 size_t size, void *buf, size_t *size_ret) {
   if (!s)
      throw error(CL_INVALID_SAMPLER);

   switch (param) {
   case CL_SAMPLER_REFERENCE_COUNT:
      return scalar_property<cl_uint>(buf, size, size_ret, s->ref_count());

   case CL_SAMPLER_CONTEXT:
      return scalar_property<cl_context>(buf, size, size_ret, &s->ctx);

   case CL_SAMPLER_NORMALIZED_COORDS:
      return scalar_property<cl_bool>(buf, size, size_ret, s->norm_mode());

   case CL_SAMPLER_ADDRESSING_MODE:
      return scalar_property<cl_addressing_mode>(buf, size, size_ret,
                                                 s->addr_mode());

   case CL_SAMPLER_FILTER_MODE:
      return scalar_property<cl_filter_mode>(buf, size, size_ret,
                                             s->filter_mode());

   default:
      return CL_INVALID_VALUE;
   }
}
