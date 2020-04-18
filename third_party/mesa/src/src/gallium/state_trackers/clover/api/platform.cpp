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

using namespace clover;

PUBLIC cl_int
clGetPlatformIDs(cl_uint num_entries, cl_platform_id *platforms,
                 cl_uint *num_platforms) {
   if ((!num_entries && platforms) ||
       (!num_platforms && !platforms))
      return CL_INVALID_VALUE;

   if (num_platforms)
      *num_platforms = 1;
   if (platforms)
      *platforms = NULL;

   return CL_SUCCESS;
}

PUBLIC cl_int
clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                  size_t size, void *buf, size_t *size_ret) {
   if (platform != NULL)
      return CL_INVALID_PLATFORM;

   switch (param_name) {
   case CL_PLATFORM_PROFILE:
      return string_property(buf, size, size_ret, "FULL_PROFILE");

   case CL_PLATFORM_VERSION:
      return string_property(buf, size, size_ret,
                             "OpenCL 1.1 MESA " MESA_VERSION);

   case CL_PLATFORM_NAME:
      return string_property(buf, size, size_ret, "Default");

   case CL_PLATFORM_VENDOR:
      return string_property(buf, size, size_ret, "Mesa");

   case CL_PLATFORM_EXTENSIONS:
      return string_property(buf, size, size_ret, "");

   default:
      return CL_INVALID_VALUE;
   }
}
