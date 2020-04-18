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
#include "core/queue.hpp"

using namespace clover;

PUBLIC cl_command_queue
clCreateCommandQueue(cl_context ctx, cl_device_id dev,
                     cl_command_queue_properties props,
                     cl_int *errcode_ret) try {
   if (!ctx)
      throw error(CL_INVALID_CONTEXT);

   if (!ctx->has_device(dev))
      throw error(CL_INVALID_DEVICE);

   if (props & ~(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                 CL_QUEUE_PROFILING_ENABLE))
      throw error(CL_INVALID_VALUE);

   ret_error(errcode_ret, CL_SUCCESS);
   return new command_queue(*ctx, *dev, props);

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_int
clRetainCommandQueue(cl_command_queue q) {
   if (!q)
      return CL_INVALID_COMMAND_QUEUE;

   q->retain();
   return CL_SUCCESS;
}

PUBLIC cl_int
clReleaseCommandQueue(cl_command_queue q) {
   if (!q)
      return CL_INVALID_COMMAND_QUEUE;

   if (q->release())
      delete q;

   return CL_SUCCESS;
}

PUBLIC cl_int
clGetCommandQueueInfo(cl_command_queue q, cl_command_queue_info param,
                      size_t size, void *buf, size_t *size_ret) {
   if (!q)
      return CL_INVALID_COMMAND_QUEUE;

   switch (param) {
   case CL_QUEUE_CONTEXT:
      return scalar_property<cl_context>(buf, size, size_ret, &q->ctx);

   case CL_QUEUE_DEVICE:
      return scalar_property<cl_device_id>(buf, size, size_ret, &q->dev);

   case CL_QUEUE_REFERENCE_COUNT:
      return scalar_property<cl_uint>(buf, size, size_ret, q->ref_count());

   case CL_QUEUE_PROPERTIES:
      return scalar_property<cl_command_queue_properties>(buf, size, size_ret,
                                                          q->props());

   default:
      return CL_INVALID_VALUE;
   }
}

PUBLIC cl_int
clFlush(cl_command_queue q) {
   if (!q)
      return CL_INVALID_COMMAND_QUEUE;

   q->flush();
   return CL_SUCCESS;
}
