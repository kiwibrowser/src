// Copyright (c) 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Validates OpCapability instruction.

#include "validate.h"

#include <cassert>
#include <unordered_set>

#include "diagnostic.h"
#include "opcode.h"
#include "val/instruction.h"
#include "val/validation_state.h"

namespace libspirv {

namespace {

bool IsSupportGuaranteedVulkan_1_0(uint32_t capability) {
  switch (capability) {
    case SpvCapabilityMatrix:
    case SpvCapabilityShader:
    case SpvCapabilityInputAttachment:
    case SpvCapabilitySampled1D:
    case SpvCapabilityImage1D:
    case SpvCapabilitySampledBuffer:
    case SpvCapabilityImageBuffer:
    case SpvCapabilityImageQuery:
    case SpvCapabilityDerivativeControl:
      return true;
  }
  return false;
}

bool IsSupportOptionalVulkan_1_0(uint32_t capability) {
  switch (capability) {
    case SpvCapabilityGeometry:
    case SpvCapabilityTessellation:
    case SpvCapabilityFloat64:
    case SpvCapabilityInt64:
    case SpvCapabilityInt16:
    case SpvCapabilityTessellationPointSize:
    case SpvCapabilityGeometryPointSize:
    case SpvCapabilityImageGatherExtended:
    case SpvCapabilityStorageImageMultisample:
    case SpvCapabilityUniformBufferArrayDynamicIndexing:
    case SpvCapabilitySampledImageArrayDynamicIndexing:
    case SpvCapabilityStorageBufferArrayDynamicIndexing:
    case SpvCapabilityStorageImageArrayDynamicIndexing:
    case SpvCapabilityClipDistance:
    case SpvCapabilityCullDistance:
    case SpvCapabilityImageCubeArray:
    case SpvCapabilitySampleRateShading:
    case SpvCapabilitySparseResidency:
    case SpvCapabilityMinLod:
    case SpvCapabilitySampledCubeArray:
    case SpvCapabilityImageMSArray:
    case SpvCapabilityStorageImageExtendedFormats:
    case SpvCapabilityInterpolationFunction:
    case SpvCapabilityStorageImageReadWithoutFormat:
    case SpvCapabilityStorageImageWriteWithoutFormat:
    case SpvCapabilityMultiViewport:
      return true;
  }
  return false;
}

// Checks if |capability| was enabled by extension.
bool IsEnabledByExtension(ValidationState_t& _, uint32_t capability) {
  spv_operand_desc operand_desc = nullptr;
  _.grammar().lookupOperand(
      SPV_OPERAND_TYPE_CAPABILITY, capability, &operand_desc);

  // operand_desc is expected to be not null, otherwise validator would have
  // failed at an earlier stage. This 'assert' is 'just in case'.
  assert(operand_desc);

  if (operand_desc->extensions.IsEmpty())
    return false;

  return _.HasAnyOfExtensions(operand_desc->extensions);
}

}  // namespace

// Validates that capability declarations use operands allowed in the current
// context.
spv_result_t CapabilityPass(ValidationState_t& _,
                            const spv_parsed_instruction_t* inst) {
  const SpvOp opcode = static_cast<SpvOp>(inst->opcode);
  if (opcode != SpvOpCapability)
    return SPV_SUCCESS;

  assert(inst->num_operands == 1);

  const spv_parsed_operand_t& operand = inst->operands[0];

  assert(operand.num_words == 1);
  assert(operand.offset < inst->num_words);

  const uint32_t capability = inst->words[operand.offset];

  const auto env = _.context()->target_env;
  if (env == SPV_ENV_VULKAN_1_0) {
    if (!IsSupportGuaranteedVulkan_1_0(capability) &&
        !IsSupportOptionalVulkan_1_0(capability) &&
        !IsEnabledByExtension(_, capability)) {
      return _.diag(SPV_ERROR_INVALID_CAPABILITY)
          << "Capability value " << capability
          << " is not allowed by Vulkan 1.0 specification"
          << " (or requires extension)";
    }
  }

  return SPV_SUCCESS;
}

}  // namespace libspirv
