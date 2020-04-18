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
#include "core/device.hpp"

using namespace clover;

static device_registry registry;

PUBLIC cl_int
clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type,
               cl_uint num_entries, cl_device_id *devices,
               cl_uint *num_devices) {
   std::vector<cl_device_id> devs;

   if (platform != NULL)
      return CL_INVALID_PLATFORM;

   if ((!num_entries && devices) ||
       (!num_devices && !devices))
      return CL_INVALID_VALUE;

   // Collect matching devices
   for (device &dev : registry) {
      if (((device_type & CL_DEVICE_TYPE_DEFAULT) &&
           &dev == &registry.front()) ||
          (device_type & dev.type()))
         devs.push_back(&dev);
   }

   if (devs.empty())
      return CL_DEVICE_NOT_FOUND;

   // ...and return the requested data.
   if (num_devices)
      *num_devices = devs.size();
   if (devices)
      std::copy_n(devs.begin(),
                  std::min((cl_uint)devs.size(), num_entries),
                  devices);

   return CL_SUCCESS;
}

PUBLIC cl_int
clGetDeviceInfo(cl_device_id dev, cl_device_info param,
                size_t size, void *buf, size_t *size_ret) {
   if (!dev)
      return CL_INVALID_DEVICE;

   switch (param) {
   case CL_DEVICE_TYPE:
      return scalar_property<cl_device_type>(buf, size, size_ret, dev->type());

   case CL_DEVICE_VENDOR_ID:
      return scalar_property<cl_uint>(buf, size, size_ret, dev->vendor_id());

   case CL_DEVICE_MAX_COMPUTE_UNITS:
      return scalar_property<cl_uint>(buf, size, size_ret, 1);

   case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
      return scalar_property<cl_uint>(buf, size, size_ret,
                                      dev->max_block_size().size());

   case CL_DEVICE_MAX_WORK_ITEM_SIZES:
      return vector_property<size_t>(buf, size, size_ret,
                                     dev->max_block_size());

   case CL_DEVICE_MAX_WORK_GROUP_SIZE:
      return scalar_property<size_t>(buf, size, size_ret,
                                     dev->max_threads_per_block());

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
      return scalar_property<cl_uint>(buf, size, size_ret, 16);

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
      return scalar_property<cl_uint>(buf, size, size_ret, 8);

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
      return scalar_property<cl_uint>(buf, size, size_ret, 4);

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
      return scalar_property<cl_uint>(buf, size, size_ret, 2);

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
      return scalar_property<cl_uint>(buf, size, size_ret, 4);

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
      return scalar_property<cl_uint>(buf, size, size_ret, 2);

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
      return scalar_property<cl_uint>(buf, size, size_ret, 0);

   case CL_DEVICE_MAX_CLOCK_FREQUENCY:
      return scalar_property<cl_uint>(buf, size, size_ret, 0);

   case CL_DEVICE_ADDRESS_BITS:
      return scalar_property<cl_uint>(buf, size, size_ret, 32);

   case CL_DEVICE_MAX_READ_IMAGE_ARGS:
      return scalar_property<cl_uint>(buf, size, size_ret,
                                      dev->max_images_read());

   case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
      return scalar_property<cl_uint>(buf, size, size_ret,
                                      dev->max_images_write());

   case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
      return scalar_property<cl_ulong>(buf, size, size_ret, 0);

   case CL_DEVICE_IMAGE2D_MAX_WIDTH:
   case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
      return scalar_property<size_t>(buf, size, size_ret,
                                     1 << dev->max_image_levels_2d());

   case CL_DEVICE_IMAGE3D_MAX_WIDTH:
   case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
   case CL_DEVICE_IMAGE3D_MAX_DEPTH:
      return scalar_property<size_t>(buf, size, size_ret,
                                     1 << dev->max_image_levels_3d());

   case CL_DEVICE_IMAGE_SUPPORT:
      return scalar_property<cl_bool>(buf, size, size_ret, CL_TRUE);

   case CL_DEVICE_MAX_PARAMETER_SIZE:
      return scalar_property<size_t>(buf, size, size_ret,
                                     dev->max_mem_input());

   case CL_DEVICE_MAX_SAMPLERS:
      return scalar_property<cl_uint>(buf, size, size_ret,
                                      dev->max_samplers());

   case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
   case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
      return scalar_property<cl_uint>(buf, size, size_ret, 128);

   case CL_DEVICE_SINGLE_FP_CONFIG:
      return scalar_property<cl_device_fp_config>(buf, size, size_ret,
         CL_FP_DENORM | CL_FP_INF_NAN | CL_FP_ROUND_TO_NEAREST);

   case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
      return scalar_property<cl_device_mem_cache_type>(buf, size, size_ret,
                                                       CL_NONE);

   case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
      return scalar_property<cl_uint>(buf, size, size_ret, 0);

   case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
      return scalar_property<cl_ulong>(buf, size, size_ret, 0);

   case CL_DEVICE_GLOBAL_MEM_SIZE:
      return scalar_property<cl_ulong>(buf, size, size_ret,
                                       dev->max_mem_global());

   case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
      return scalar_property<cl_ulong>(buf, size, size_ret,
                                       dev->max_const_buffer_size());

   case CL_DEVICE_MAX_CONSTANT_ARGS:
      return scalar_property<cl_uint>(buf, size, size_ret,
                                      dev->max_const_buffers());

   case CL_DEVICE_LOCAL_MEM_TYPE:
      return scalar_property<cl_device_local_mem_type>(buf, size, size_ret,
                                                       CL_LOCAL);

   case CL_DEVICE_LOCAL_MEM_SIZE:
      return scalar_property<cl_ulong>(buf, size, size_ret,
                                       dev->max_mem_local());

   case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
      return scalar_property<cl_bool>(buf, size, size_ret, CL_FALSE);

   case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
      return scalar_property<size_t>(buf, size, size_ret, 0);

   case CL_DEVICE_ENDIAN_LITTLE:
      return scalar_property<cl_bool>(buf, size, size_ret, CL_TRUE);

   case CL_DEVICE_AVAILABLE:
   case CL_DEVICE_COMPILER_AVAILABLE:
      return scalar_property<cl_bool>(buf, size, size_ret, CL_TRUE);

   case CL_DEVICE_EXECUTION_CAPABILITIES:
      return scalar_property<cl_device_exec_capabilities>(buf, size, size_ret,
                                                          CL_EXEC_KERNEL);

   case CL_DEVICE_QUEUE_PROPERTIES:
      return scalar_property<cl_command_queue_properties>(buf, size, size_ret,
         CL_QUEUE_PROFILING_ENABLE);

   case CL_DEVICE_NAME:
      return string_property(buf, size, size_ret, dev->device_name());

   case CL_DEVICE_VENDOR:
      return string_property(buf, size, size_ret, dev->vendor_name());

   case CL_DRIVER_VERSION:
      return string_property(buf, size, size_ret, MESA_VERSION);

   case CL_DEVICE_PROFILE:
      return string_property(buf, size, size_ret, "FULL_PROFILE");

   case CL_DEVICE_VERSION:
      return string_property(buf, size, size_ret, "OpenCL 1.1 MESA " MESA_VERSION);

   case CL_DEVICE_EXTENSIONS:
      return string_property(buf, size, size_ret, "");

   case CL_DEVICE_PLATFORM:
      return scalar_property<cl_platform_id>(buf, size, size_ret, NULL);

   case CL_DEVICE_HOST_UNIFIED_MEMORY:
      return scalar_property<cl_bool>(buf, size, size_ret, CL_TRUE);

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
      return scalar_property<cl_uint>(buf, size, size_ret, 16);

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
      return scalar_property<cl_uint>(buf, size, size_ret, 8);

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
      return scalar_property<cl_uint>(buf, size, size_ret, 4);

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
      return scalar_property<cl_uint>(buf, size, size_ret, 2);

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
      return scalar_property<cl_uint>(buf, size, size_ret, 4);

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
      return scalar_property<cl_uint>(buf, size, size_ret, 2);

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
      return scalar_property<cl_uint>(buf, size, size_ret, 0);

   case CL_DEVICE_OPENCL_C_VERSION:
      return string_property(buf, size, size_ret, "OpenCL C 1.1");

   default:
      return CL_INVALID_VALUE;
   }
}
