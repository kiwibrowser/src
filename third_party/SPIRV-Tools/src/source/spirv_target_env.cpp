// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#include <assert.h>

#include "spirv-tools/libspirv.h"
#include "spirv_constant.h"

const char* spvTargetEnvDescription(spv_target_env env) {
  switch (env) {
    case SPV_ENV_UNIVERSAL_1_0:
      return "SPIR-V 1.0";
    case SPV_ENV_VULKAN_1_0:
      return "SPIR-V 1.0 (under Vulkan 1.0 semantics)";
    case SPV_ENV_UNIVERSAL_1_1:
      return "SPIR-V 1.1";
    default:
      break;
  }
  assert(0 && "Unhandled SPIR-V target environment");
  return "";
}

uint32_t spvVersionForTargetEnv(spv_target_env env) {
  switch (env) {
    case SPV_ENV_UNIVERSAL_1_0:
    case SPV_ENV_VULKAN_1_0:
      return SPV_SPIRV_VERSION_WORD(1, 0);
    case SPV_ENV_UNIVERSAL_1_1:
      return SPV_SPIRV_VERSION_WORD(1, 1);
    default:
      break;
  }
  assert(0 && "Unhandled SPIR-V target environment");
  return SPV_SPIRV_VERSION_WORD(0, 0);
}
