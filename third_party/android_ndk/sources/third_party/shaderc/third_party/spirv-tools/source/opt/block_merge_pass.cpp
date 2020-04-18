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

#include "block_merge_pass.h"

#include "iterator.h"

namespace spvtools {
namespace opt {

bool BlockMergePass::IsLoopHeader(ir::BasicBlock* block_ptr) {
  auto iItr = block_ptr->tail();
  if (iItr == block_ptr->begin())
    return false;
  --iItr;
  return iItr->opcode() == SpvOpLoopMerge;
}

bool BlockMergePass::HasMultipleRefs(uint32_t labId) {
  const analysis::UseList* uses = def_use_mgr_->GetUses(labId);
  int rcnt = 0;
  for (const auto u : *uses) {
    // Don't count OpName
    if (u.inst->opcode() == SpvOpName)
      continue;
    if (rcnt == 1)
      return true;
    ++rcnt;
  }
  return false;
}

void BlockMergePass::KillInstAndName(ir::Instruction* inst) {
  const uint32_t id = inst->result_id();
  if (id != 0) {
    analysis::UseList* uses = def_use_mgr_->GetUses(id);
    if (uses != nullptr)
      for (auto u : *uses)
        if (u.inst->opcode() == SpvOpName) {
          def_use_mgr_->KillInst(u.inst);
          break;
        }
  }
  def_use_mgr_->KillInst(inst);
}

bool BlockMergePass::MergeBlocks(ir::Function* func) {
  bool modified = false;
  for (auto bi = func->begin(); bi != func->end(); ) {
    // Do not merge loop header blocks, at least for now.
    if (IsLoopHeader(&*bi)) {
      ++bi;
      continue;
    }
    // Find block with single successor which has no other predecessors.
    // Continue and Merge blocks are currently ruled out as second blocks.
    // Happily any such candidate blocks will have >1 uses due to their
    // LoopMerge instruction.
    // TODO(): Deal with phi instructions that reference the
    // second block. Happily, these references currently inhibit
    // the merge.
    auto ii = bi->end();
    --ii;
    ir::Instruction* br = &*ii;
    if (br->opcode() != SpvOpBranch) {
      ++bi;
      continue;
    }
    const uint32_t labId = br->GetSingleWordInOperand(0);
    if (HasMultipleRefs(labId)) {
      ++bi;
      continue;
    }
    // Merge blocks
    def_use_mgr_->KillInst(br);
    auto sbi = bi;
    for (; sbi != func->end(); ++sbi)
      if (sbi->id() == labId)
        break;
    // If bi is sbi's only predecessor, it dominates sbi and thus
    // sbi must follow bi in func's ordering.
    assert(sbi != func->end());
    bi->AddInstructions(&*sbi);
    KillInstAndName(sbi->GetLabelInst());
    (void) sbi.Erase();
    // reprocess block
    modified = true;
  }
  return modified;
}

void BlockMergePass::Initialize(ir::Module* module) {

  module_ = module;

  // TODO(greg-lunarg): Reuse def/use from previous passes
  def_use_mgr_.reset(new analysis::DefUseManager(consumer(), module_));

  // Initialize extension whitelist
  InitExtensions();
};

bool BlockMergePass::AllExtensionsSupported() const {
  // If any extension not in whitelist, return false
  for (auto& ei : module_->extensions()) {
    const char* extName = reinterpret_cast<const char*>(
        &ei.GetInOperand(0).words[0]);
    if (extensions_whitelist_.find(extName) == extensions_whitelist_.end())
      return false;
  }
  return true;
}

Pass::Status BlockMergePass::ProcessImpl() {
  // Do not process if any disallowed extensions are enabled
  if (!AllExtensionsSupported())
    return Status::SuccessWithoutChange;
  // Process all entry point functions.
  ProcessFunction pfn = [this](ir::Function* fp) {
    return MergeBlocks(fp);
  };
  bool modified = ProcessEntryPointCallTree(pfn, module_);
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

BlockMergePass::BlockMergePass()
    : module_(nullptr), def_use_mgr_(nullptr) {}

Pass::Status BlockMergePass::Process(ir::Module* module) {
  Initialize(module);
  return ProcessImpl();
}

void BlockMergePass::InitExtensions() {
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
    "SPV_KHR_variable_pointers",
    "SPV_AMD_gpu_shader_int16",
    "SPV_KHR_post_depth_coverage",
    "SPV_KHR_shader_atomic_counter_ops",
  });
}

}  // namespace opt
}  // namespace spvtools

