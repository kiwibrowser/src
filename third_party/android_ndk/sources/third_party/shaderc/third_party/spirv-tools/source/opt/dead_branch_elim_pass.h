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

#ifndef LIBSPIRV_OPT_DEAD_BRANCH_ELIM_PASS_H_
#define LIBSPIRV_OPT_DEAD_BRANCH_ELIM_PASS_H_


#include <algorithm>
#include <map>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "basic_block.h"
#include "def_use_manager.h"
#include "module.h"
#include "mem_pass.h"

namespace spvtools {
namespace opt {

// See optimizer.hpp for documentation.
class DeadBranchElimPass : public MemPass {

  using cbb_ptr = const ir::BasicBlock*;

 public:
   using GetBlocksFunction =
     std::function<std::vector<ir::BasicBlock*>*(const ir::BasicBlock*)>;

  DeadBranchElimPass();
  const char* name() const override { return "dead-branch-elim"; }
  Status Process(ir::Module*) override;

 private:
  // Returns the id of the merge block declared by a merge instruction in 
  // this block |blk|, if any. If none, returns zero. If loop merge, returns
  // the continue target id in |cbid|. Otherwise sets to zero.
  uint32_t MergeBlockIdIfAny(const ir::BasicBlock& blk, uint32_t* cbid) const;

  // Compute structured successors for function |func|.
  // A block's structured successors are the blocks it branches to
  // together with its declared merge block if it has one.
  // When order matters, the merge block always appears first and if
  // a loop merge block, the continue target always appears second.
  // This assures correct depth first search in the presence of early 
  // returns and kills. If the successor vector contain duplicates
  // of the merge and continue blocks, they are safely ignored by DFS.
  void ComputeStructuredSuccessors(ir::Function* func);

  // Compute structured block order |order| for function |func|. This order
  // has the property that dominators are before all blocks they dominate and
  // merge blocks are after all blocks that are in the control constructs of
  // their header.
  void ComputeStructuredOrder(
    ir::Function* func, std::list<ir::BasicBlock*>* order);

  // If |condId| is boolean constant, return value in |condVal| and
  // |condIsConst| as true, otherwise return |condIsConst| as false.
  void GetConstCondition(uint32_t condId, bool* condVal, bool* condIsConst);

  // Add branch to |labelId| to end of block |bp|.
  void AddBranch(uint32_t labelId, ir::BasicBlock* bp);

  // Add selction merge of |labelId| to end of block |bp|.
  void AddSelectionMerge(uint32_t labelId, ir::BasicBlock* bp);

  // Add conditional branch of |condId|, |trueLabId| and |falseLabId| to end
  // of block |bp|.
  void AddBranchConditional(uint32_t condId, uint32_t trueLabId,
      uint32_t falseLabId, ir::BasicBlock* bp);

  // Kill all instructions in block |bp|.
  void KillAllInsts(ir::BasicBlock* bp);

  // If block |bp| contains constant conditional branch preceeded by an
  // OpSelctionMerge, return true and return branch and merge instructions
  // in |branchInst| and |mergeInst| and the boolean constant in |condVal|. 
  bool GetConstConditionalSelectionBranch(ir::BasicBlock* bp,
    ir::Instruction** branchInst, ir::Instruction** mergeInst,
    uint32_t *condId, bool *condVal);

  // Return true if |labelId| has any non-phi references
  bool HasNonPhiRef(uint32_t labelId);

  // For function |func|, look for BranchConditionals with constant condition
  // and convert to a Branch to the indicated label. Delete resulting dead
  // blocks. Assumes only structured control flow in shader. Note some such
  // branches and blocks may be left to avoid creating invalid control flow.
  // TODO(greg-lunarg): Remove remaining constant conditional branches and
  // dead blocks.
  bool EliminateDeadBranches(ir::Function* func);

  // Initialize extensions whitelist
  void InitExtensions();

  // Return true if all extensions in this module are allowed by this pass.
  bool AllExtensionsSupported() const;

  void Initialize(ir::Module* module);
  Pass::Status ProcessImpl();

  // Map from block's label id to block.
  std::unordered_map<uint32_t, ir::BasicBlock*> id2block_;

  // Map from block to its structured successor blocks. See 
  // ComputeStructuredSuccessors() for definition.
  std::unordered_map<const ir::BasicBlock*, std::vector<ir::BasicBlock*>>
      block2structured_succs_;
  
  // Extensions supported by this pass.
  std::unordered_set<std::string> extensions_whitelist_;
};

}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_DEAD_BRANCH_ELIM_PASS_H_

