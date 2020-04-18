// Copyright (c) 2017 The Khronos Group Inc.
// Copyright (c) 2017 Valve Corporation
// Copyright (c) 2017 LunarG Inc.
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

#include "aggressive_dead_code_elim_pass.h"

#include "iterator.h"
#include "spirv/1.0/GLSL.std.450.h"

namespace spvtools {
namespace opt {

namespace {

const uint32_t kTypePointerStorageClassInIdx = 0;
const uint32_t kExtInstSetIdInIndx = 0;
const uint32_t kExtInstInstructionInIndx = 1;

}  // namespace anonymous

bool AggressiveDCEPass::IsLocalVar(uint32_t varId) {
  const ir::Instruction* varInst = def_use_mgr_->GetDef(varId);
  const SpvOp op = varInst->opcode();
  if (op != SpvOpVariable && op != SpvOpFunctionParameter) 
    return false;
  const uint32_t varTypeId = varInst->type_id();
  const ir::Instruction* varTypeInst = def_use_mgr_->GetDef(varTypeId);
  if (varTypeInst->opcode() != SpvOpTypePointer)
    return false;
  return varTypeInst->GetSingleWordInOperand(kTypePointerStorageClassInIdx) ==
      SpvStorageClassFunction;
}

void AggressiveDCEPass::AddStores(uint32_t ptrId) {
  const analysis::UseList* uses = def_use_mgr_->GetUses(ptrId);
  if (uses == nullptr)
    return;
  for (const auto u : *uses) {
    const SpvOp op = u.inst->opcode();
    switch (op) {
      case SpvOpAccessChain:
      case SpvOpInBoundsAccessChain:
      case SpvOpCopyObject: {
        AddStores(u.inst->result_id());
      } break;
      case SpvOpLoad:
        break;
      // If default, assume it stores eg frexp, modf, function call
      case SpvOpStore:
      default: {
        if (live_insts_.find(u.inst) == live_insts_.end())
          worklist_.push(u.inst);
      } break;
    }
  }
}

bool AggressiveDCEPass::IsCombinator(uint32_t op) const {
  return combinator_ops_shader_.find(op) != combinator_ops_shader_.end();
}

bool AggressiveDCEPass::IsCombinatorExt(ir::Instruction* inst) const {
  assert(inst->opcode() == SpvOpExtInst);
  if (inst->GetSingleWordInOperand(kExtInstSetIdInIndx) == glsl_std_450_id_) {
    uint32_t op = inst->GetSingleWordInOperand(kExtInstInstructionInIndx);
    return combinator_ops_glsl_std_450_.find(op) !=
        combinator_ops_glsl_std_450_.end();
  }
  else
    return false;
}

bool AggressiveDCEPass::AllExtensionsSupported() const {
  // If any extension not in whitelist, return false
  for (auto& ei : module_->extensions()) {
    const char* extName = reinterpret_cast<const char*>(
        &ei.GetInOperand(0).words[0]);
    if (extensions_whitelist_.find(extName) == extensions_whitelist_.end())
      return false;
  }
  return true;
}

bool AggressiveDCEPass::KillInstIfTargetDead(ir::Instruction* inst) {
  const uint32_t tId = inst->GetSingleWordInOperand(0);
  const ir::Instruction* tInst = def_use_mgr_->GetDef(tId);
  if (dead_insts_.find(tInst) != dead_insts_.end()) {
    def_use_mgr_->KillInst(inst);
    return true;
  }
  return false;
}

void AggressiveDCEPass::ProcessLoad(uint32_t varId) {
  // Only process locals
  if (!IsLocalVar(varId))
    return;
  // Return if already processed
  if (live_local_vars_.find(varId) != live_local_vars_.end()) 
    return;
  // Mark all stores to varId as live
  AddStores(varId);
  // Cache varId as processed
  live_local_vars_.insert(varId);
}

bool AggressiveDCEPass::AggressiveDCE(ir::Function* func) {
  bool modified = false;
  // Add all control flow and instructions with external side effects 
  // to worklist
  // TODO(greg-lunarg): Handle Frexp, Modf more optimally
  for (auto& blk : *func) {
    for (auto& inst : blk) {
      uint32_t op = inst.opcode();
      switch (op) {
        case SpvOpStore: {
          uint32_t varId;
          (void) GetPtr(&inst, &varId);
          // non-function-scope stores
          if (!IsLocalVar(varId)) {
            worklist_.push(&inst);
          }
        } break;
        case SpvOpExtInst: {
          // eg. GLSL frexp, modf
          if (!IsCombinatorExt(&inst))
            worklist_.push(&inst);
        } break;
        default: {
          // eg. control flow, function call, atomics, function param,
          // function return
          // TODO(greg-lunarg): function calls live only if write to non-local
          if (!IsCombinator(op))
            worklist_.push(&inst);
        } break;
      }
    }
  }
  // Add OpGroupDecorates to worklist because they are a pain to remove
  // ids from.
  // TODO(greg-lunarg): Handle dead ids in OpGroupDecorate
  for (auto& ai : module_->annotations()) {
    if (ai.opcode() == SpvOpGroupDecorate)
      worklist_.push(&ai);
  }
  // Perform closure on live instruction set. 
  while (!worklist_.empty()) {
    ir::Instruction* liveInst = worklist_.front();
    live_insts_.insert(liveInst);
    // Add all operand instructions if not already live
    liveInst->ForEachInId([this](const uint32_t* iid) {
      ir::Instruction* inInst = def_use_mgr_->GetDef(*iid);
      if (live_insts_.find(inInst) == live_insts_.end())
        worklist_.push(inInst);
    });
    // If local load, add all variable's stores if variable not already live
    if (liveInst->opcode() == SpvOpLoad) {
      uint32_t varId;
      (void) GetPtr(liveInst, &varId);
      ProcessLoad(varId);
    }
    // If function call, treat as if it loads from all pointer arguments
    else if (liveInst->opcode() == SpvOpFunctionCall) {
      liveInst->ForEachInId([this](const uint32_t* iid) {
        // Skip non-ptr args
        if (!IsPtr(*iid)) return;
        uint32_t varId;
        (void) GetPtr(*iid, &varId);
        ProcessLoad(varId);
      });
    }
    // If function parameter, treat as if it's result id is loaded from
    else if (liveInst->opcode() == SpvOpFunctionParameter) {
      ProcessLoad(liveInst->result_id());
    }
    worklist_.pop();
  }
  // Mark all non-live instructions dead
  for (auto& blk : *func) {
    for (auto& inst : blk) {
      if (live_insts_.find(&inst) != live_insts_.end())
        continue;
      dead_insts_.insert(&inst);
    }
  }
  // Remove debug and annotation statements referencing dead instructions.
  // This must be done before killing the instructions, otherwise there are
  // dead objects in the def/use database.
  for (auto& di : module_->debugs()) {
    if (di.opcode() != SpvOpName)
      continue;
    if (KillInstIfTargetDead(&di))
      modified = true;
  }
  for (auto& ai : module_->annotations()) {
    if (ai.opcode() != SpvOpDecorate && ai.opcode() != SpvOpDecorateId)
      continue;
    if (KillInstIfTargetDead(&ai))
      modified = true;
  }
  // Kill dead instructions
  for (auto& blk : *func) {
    for (auto& inst : blk) {
      if (dead_insts_.find(&inst) == dead_insts_.end())
        continue;
      def_use_mgr_->KillInst(&inst);
      modified = true;
    }
  }
  return modified;
}

void AggressiveDCEPass::Initialize(ir::Module* module) {
  module_ = module;

  // Clear collections
  worklist_ = std::queue<ir::Instruction*>{};
  live_insts_.clear();
  live_local_vars_.clear();
  dead_insts_.clear();
  combinator_ops_shader_.clear();
  combinator_ops_glsl_std_450_.clear();

  // TODO(greg-lunarg): Reuse def/use from previous passes
  def_use_mgr_.reset(new analysis::DefUseManager(consumer(), module_));

  // Initialize extensions whitelist
  InitExtensions();
}

Pass::Status AggressiveDCEPass::ProcessImpl() {
  // Current functionality assumes shader capability 
  // TODO(greg-lunarg): Handle additional capabilities
  if (!module_->HasCapability(SpvCapabilityShader))
    return Status::SuccessWithoutChange;
  // Current functionality assumes logical addressing only
  // TODO(greg-lunarg): Handle non-logical addressing
  if (module_->HasCapability(SpvCapabilityAddresses))
    return Status::SuccessWithoutChange;
  // If any extensions in the module are not explicitly supported,
  // return unmodified. 
  if (!AllExtensionsSupported())
    return Status::SuccessWithoutChange;
  // Initialize combinator whitelists
  InitCombinatorSets();
  // Process all entry point functions
  ProcessFunction pfn = [this](ir::Function* fp) {
    return AggressiveDCE(fp);
  };
  bool modified = ProcessEntryPointCallTree(pfn, module_);
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

AggressiveDCEPass::AggressiveDCEPass() {}

Pass::Status AggressiveDCEPass::Process(ir::Module* module) {
  Initialize(module);
  return ProcessImpl();
}

void AggressiveDCEPass::InitCombinatorSets() {
  combinator_ops_shader_ = {
    SpvOpNop,
    SpvOpUndef,
    SpvOpVariable,
    SpvOpImageTexelPointer,
    SpvOpLoad,
    SpvOpAccessChain,
    SpvOpInBoundsAccessChain,
    SpvOpArrayLength,
    SpvOpVectorExtractDynamic,
    SpvOpVectorInsertDynamic,
    SpvOpVectorShuffle,
    SpvOpCompositeConstruct,
    SpvOpCompositeExtract,
    SpvOpCompositeInsert,
    SpvOpCopyObject,
    SpvOpTranspose,
    SpvOpSampledImage,
    SpvOpImageSampleImplicitLod,
    SpvOpImageSampleExplicitLod,
    SpvOpImageSampleDrefImplicitLod,
    SpvOpImageSampleDrefExplicitLod,
    SpvOpImageSampleProjImplicitLod,
    SpvOpImageSampleProjExplicitLod,
    SpvOpImageSampleProjDrefImplicitLod,
    SpvOpImageSampleProjDrefExplicitLod,
    SpvOpImageFetch,
    SpvOpImageGather,
    SpvOpImageDrefGather,
    SpvOpImageRead,
    SpvOpImage,
    SpvOpConvertFToU,
    SpvOpConvertFToS,
    SpvOpConvertSToF,
    SpvOpConvertUToF,
    SpvOpUConvert,
    SpvOpSConvert,
    SpvOpFConvert,
    SpvOpQuantizeToF16,
    SpvOpBitcast,
    SpvOpSNegate,
    SpvOpFNegate,
    SpvOpIAdd,
    SpvOpFAdd,
    SpvOpISub,
    SpvOpFSub,
    SpvOpIMul,
    SpvOpFMul,
    SpvOpUDiv,
    SpvOpSDiv,
    SpvOpFDiv,
    SpvOpUMod,
    SpvOpSRem,
    SpvOpSMod,
    SpvOpFRem,
    SpvOpFMod,
    SpvOpVectorTimesScalar,
    SpvOpMatrixTimesScalar,
    SpvOpVectorTimesMatrix,
    SpvOpMatrixTimesVector,
    SpvOpMatrixTimesMatrix,
    SpvOpOuterProduct,
    SpvOpDot,
    SpvOpIAddCarry,
    SpvOpISubBorrow,
    SpvOpUMulExtended,
    SpvOpSMulExtended,
    SpvOpAny,
    SpvOpAll,
    SpvOpIsNan,
    SpvOpIsInf,
    SpvOpLogicalEqual,
    SpvOpLogicalNotEqual,
    SpvOpLogicalOr,
    SpvOpLogicalAnd,
    SpvOpLogicalNot,
    SpvOpSelect,
    SpvOpIEqual,
    SpvOpINotEqual,
    SpvOpUGreaterThan,
    SpvOpSGreaterThan,
    SpvOpUGreaterThanEqual,
    SpvOpSGreaterThanEqual,
    SpvOpULessThan,
    SpvOpSLessThan,
    SpvOpULessThanEqual,
    SpvOpSLessThanEqual,
    SpvOpFOrdEqual,
    SpvOpFUnordEqual,
    SpvOpFOrdNotEqual,
    SpvOpFUnordNotEqual,
    SpvOpFOrdLessThan,
    SpvOpFUnordLessThan,
    SpvOpFOrdGreaterThan,
    SpvOpFUnordGreaterThan,
    SpvOpFOrdLessThanEqual,
    SpvOpFUnordLessThanEqual,
    SpvOpFOrdGreaterThanEqual,
    SpvOpFUnordGreaterThanEqual,
    SpvOpShiftRightLogical,
    SpvOpShiftRightArithmetic,
    SpvOpShiftLeftLogical,
    SpvOpBitwiseOr,
    SpvOpBitwiseXor,
    SpvOpBitwiseAnd,
    SpvOpNot,
    SpvOpBitFieldInsert,
    SpvOpBitFieldSExtract,
    SpvOpBitFieldUExtract,
    SpvOpBitReverse,
    SpvOpBitCount,
    SpvOpDPdx,
    SpvOpDPdy,
    SpvOpFwidth,
    SpvOpDPdxFine,
    SpvOpDPdyFine,
    SpvOpFwidthFine,
    SpvOpDPdxCoarse,
    SpvOpDPdyCoarse,
    SpvOpFwidthCoarse,
    SpvOpPhi,
    SpvOpImageSparseSampleImplicitLod,
    SpvOpImageSparseSampleExplicitLod,
    SpvOpImageSparseSampleDrefImplicitLod,
    SpvOpImageSparseSampleDrefExplicitLod,
    SpvOpImageSparseSampleProjImplicitLod,
    SpvOpImageSparseSampleProjExplicitLod,
    SpvOpImageSparseSampleProjDrefImplicitLod,
    SpvOpImageSparseSampleProjDrefExplicitLod,
    SpvOpImageSparseFetch,
    SpvOpImageSparseGather,
    SpvOpImageSparseDrefGather,
    SpvOpImageSparseTexelsResident,
    SpvOpImageSparseRead,
    SpvOpSizeOf
    // TODO(dneto): Add instructions enabled by ImageQuery
  };

  // Find supported extension instruction set ids
  glsl_std_450_id_ = module_->GetExtInstImportId("GLSL.std.450");

  combinator_ops_glsl_std_450_ = {
    GLSLstd450Round,
    GLSLstd450RoundEven,
    GLSLstd450Trunc,
    GLSLstd450FAbs,
    GLSLstd450SAbs,
    GLSLstd450FSign,
    GLSLstd450SSign,
    GLSLstd450Floor,
    GLSLstd450Ceil,
    GLSLstd450Fract,
    GLSLstd450Radians,
    GLSLstd450Degrees,
    GLSLstd450Sin,
    GLSLstd450Cos,
    GLSLstd450Tan,
    GLSLstd450Asin,
    GLSLstd450Acos,
    GLSLstd450Atan,
    GLSLstd450Sinh,
    GLSLstd450Cosh,
    GLSLstd450Tanh,
    GLSLstd450Asinh,
    GLSLstd450Acosh,
    GLSLstd450Atanh,
    GLSLstd450Atan2,
    GLSLstd450Pow,
    GLSLstd450Exp,
    GLSLstd450Log,
    GLSLstd450Exp2,
    GLSLstd450Log2,
    GLSLstd450Sqrt,
    GLSLstd450InverseSqrt,
    GLSLstd450Determinant,
    GLSLstd450MatrixInverse,
    GLSLstd450ModfStruct,
    GLSLstd450FMin,
    GLSLstd450UMin,
    GLSLstd450SMin,
    GLSLstd450FMax,
    GLSLstd450UMax,
    GLSLstd450SMax,
    GLSLstd450FClamp,
    GLSLstd450UClamp,
    GLSLstd450SClamp,
    GLSLstd450FMix,
    GLSLstd450IMix,
    GLSLstd450Step,
    GLSLstd450SmoothStep,
    GLSLstd450Fma,
    GLSLstd450FrexpStruct,
    GLSLstd450Ldexp,
    GLSLstd450PackSnorm4x8,
    GLSLstd450PackUnorm4x8,
    GLSLstd450PackSnorm2x16,
    GLSLstd450PackUnorm2x16,
    GLSLstd450PackHalf2x16,
    GLSLstd450PackDouble2x32,
    GLSLstd450UnpackSnorm2x16,
    GLSLstd450UnpackUnorm2x16,
    GLSLstd450UnpackHalf2x16,
    GLSLstd450UnpackSnorm4x8,
    GLSLstd450UnpackUnorm4x8,
    GLSLstd450UnpackDouble2x32,
    GLSLstd450Length,
    GLSLstd450Distance,
    GLSLstd450Cross,
    GLSLstd450Normalize,
    GLSLstd450FaceForward,
    GLSLstd450Reflect,
    GLSLstd450Refract,
    GLSLstd450FindILsb,
    GLSLstd450FindSMsb,
    GLSLstd450FindUMsb,
    GLSLstd450InterpolateAtCentroid,
    GLSLstd450InterpolateAtSample,
    GLSLstd450InterpolateAtOffset,
    GLSLstd450NMin,
    GLSLstd450NMax,
    GLSLstd450NClamp
  };
}

void AggressiveDCEPass::InitExtensions() {
  extensions_whitelist_.clear();
  extensions_whitelist_.insert({
    "SPV_AMD_shader_explicit_vertex_parameter",
    "SPV_AMD_shader_trinary_minmax",
    "SPV_AMD_gcn_shader",
    "SPV_KHR_shader_ballot",
    "SPV_AMD_shader_ballot",
    "SPV_AMD_gpu_shader_half_float",
    "SPV_KHR_shader_draw_parameters",
    "SPV_KHR_subgroup_vote",
    "SPV_KHR_16bit_storage",
    "SPV_KHR_device_group",
    "SPV_KHR_multiview",
    "SPV_NVX_multiview_per_view_attributes",
    "SPV_NV_viewport_array2",
    "SPV_NV_stereo_view_rendering",
    "SPV_NV_sample_mask_override_coverage",
    "SPV_NV_geometry_shader_passthrough",
    "SPV_AMD_texture_gather_bias_lod",
    "SPV_KHR_storage_buffer_storage_class",
    // SPV_KHR_variable_pointers
    //   Currently do not support extended pointer expressions
    "SPV_AMD_gpu_shader_int16",
    "SPV_KHR_post_depth_coverage",
    "SPV_KHR_shader_atomic_counter_ops",
  });
}

}  // namespace opt
}  // namespace spvtools

