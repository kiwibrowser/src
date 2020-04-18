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

#ifndef LIBSPIRV_OPT_LOCAL_SSA_ELIM_PASS_H_
#define LIBSPIRV_OPT_LOCAL_SSA_ELIM_PASS_H_


#include <algorithm>
#include <map>
#include <queue>
#include <utility>
#include <unordered_map>
#include <unordered_set>

#include "basic_block.h"
#include "def_use_manager.h"
#include "module.h"
#include "mem_pass.h"

namespace spvtools {
namespace opt {

// See optimizer.hpp for documentation.
class LocalMultiStoreElimPass : public MemPass {
  using cbb_ptr = const ir::BasicBlock*;

 public:
   using GetBlocksFunction =
     std::function<std::vector<ir::BasicBlock*>*(const ir::BasicBlock*)>;

  LocalMultiStoreElimPass();
  const char* name() const override { return "eliminate-local-multi-store"; }
  Status Process(ir::Module*) override;

 private:
  // Return type id for |ptrInst|'s pointee
  uint32_t GetPointeeTypeId(const ir::Instruction* ptrInst) const;

  // Return true if all uses of |varId| are only through supported reference
  // operations ie. loads and store. Also cache in supported_ref_vars_;
  bool HasOnlySupportedRefs(uint32_t varId);

  // Initialize data structures used by EliminateLocalMultiStore for
  // function |func|, specifically block predecessors and target variables.
  void InitSSARewrite(ir::Function& func);

  // Returns the id of the merge block declared by a merge instruction in 
  // this block, if any.  If none, returns zero.
  uint32_t MergeBlockIdIfAny(const ir::BasicBlock& blk, uint32_t* cbid);

  // Compute structured successors for function |func|.
  // A block's structured successors are the blocks it branches to
  // together with its declared merge block if it has one.
  // When order matters, the merge block always appears first.
  // This assures correct depth first search in the presence of early 
  // returns and kills. If the successor vector contain duplicates
  // if the merge block, they are safely ignored by DFS.
  void ComputeStructuredSuccessors(ir::Function* func);

  // Compute structured block order for |func| into |structuredOrder|. This
  // order has the property that dominators come before all blocks they
  // dominate and merge blocks come after all blocks that are in the control
  // constructs of their header.
  void ComputeStructuredOrder(ir::Function* func,
      std::list<ir::BasicBlock*>* order);

  // Return true if loop header block
  bool IsLoopHeader(ir::BasicBlock* block_ptr) const;

  // Initialize label2ssa_map_ entry for block |block_ptr| with single
  // predecessor.
  void SSABlockInitSinglePred(ir::BasicBlock* block_ptr);

  // Return true if variable is loaded in block with |label| or in
  // any succeeding block in structured order.
  bool IsLiveAfter(uint32_t var_id, uint32_t label) const;

  // Initialize label2ssa_map_ entry for loop header block pointed to
  // |block_itr| by merging entries from all predecessors. If any value
  // ids differ for any variable across predecessors, create a phi function
  // in the block and use that value id for the variable in the new map.
  // Assumes all predecessors have been visited by EliminateLocalMultiStore
  // except the back edge. Use a dummy value in the phi for the back edge
  // until the back edge block is visited and patch the phi value then.
  void SSABlockInitLoopHeader(std::list<ir::BasicBlock*>::iterator block_itr);

  // Initialize label2ssa_map_ entry for multiple predecessor block
  // |block_ptr| by merging label2ssa_map_ entries for all predecessors.
  // If any value ids differ for any variable across predecessors, create
  // a phi function in the block and use that value id for the variable in
  // the new map. Assumes all predecessors have been visited by
  // EliminateLocalMultiStore.
  void SSABlockInitMultiPred(ir::BasicBlock* block_ptr);

  // Initialize the label2ssa_map entry for a block pointed to by |block_itr|.
  // Insert phi instructions into block when necessary. All predecessor
  // blocks must have been visited by EliminateLocalMultiStore except for
  // backedges.
  void SSABlockInit(std::list<ir::BasicBlock*>::iterator block_itr);

  // Return undef in function for type. Create and insert an undef after the
  // first non-variable in the function if it doesn't already exist. Add
  // undef to function undef map.
  uint32_t Type2Undef(uint32_t type_id);

  // Patch phis in loop header block now that the map is complete for the
  // backedge predecessor. Specifically, for each phi, find the value
  // corresponding to the backedge predecessor. That contains the variable id
  // that this phi corresponds to. Change this phi operand to the the value
  // which corresponds to that variable in the predecessor map.
  void PatchPhis(uint32_t header_id, uint32_t back_id);

  // Initialize extensions whitelist
  void InitExtensions();

  // Return true if all extensions in this module are allowed by this pass.
  bool AllExtensionsSupported() const;

  // Remove remaining loads and stores of function scope variables only
  // referenced with non-access-chain loads and stores from function |func|.
  // Insert Phi functions where necessary. Running LocalAccessChainRemoval,
  // SingleBlockLocalElim and SingleStoreLocalElim beforehand will improve
  // the runtime and effectiveness of this function.
  bool EliminateMultiStoreLocal(ir::Function* func);

  // Save next available id into |module|.
  inline void FinalizeNextId(ir::Module* module) {
    module->SetIdBound(next_id_);
  }

  // Return next available id and calculate next.
  inline uint32_t TakeNextId() {
    return next_id_++;
  }

  void Initialize(ir::Module* module);
  Pass::Status ProcessImpl();

  // Map from block's label id to block.
  std::unordered_map<uint32_t, ir::BasicBlock*> id2block_;

  // Set of label ids of visited blocks
  std::unordered_set<uint32_t> visitedBlocks_;

  // Map from type to undef
  std::unordered_map<uint32_t, uint32_t> type2undefs_;

  // Variables that are only referenced by supported operations for this
  // pass ie. loads and stores.
  std::unordered_set<uint32_t> supported_ref_vars_;

  // Map from block to its structured successor blocks. See 
  // ComputeStructuredSuccessors() for definition.
  std::unordered_map<const ir::BasicBlock*, std::vector<ir::BasicBlock*>>
      block2structured_succs_;

  // Map from block's label id to its predecessor blocks ids
  std::unordered_map<uint32_t, std::vector<uint32_t>> label2preds_;

  // Map from block's label id to a map of a variable to its value at the
  // end of the block.
  std::unordered_map<uint32_t, std::unordered_map<uint32_t, uint32_t>>
      label2ssa_map_;

  // Extra block whose successors are all blocks with no predecessors
  // in function.
  ir::BasicBlock pseudo_entry_block_;

  // Extensions supported by this pass.
  std::unordered_set<std::string> extensions_whitelist_;

  // Next unused ID
  uint32_t next_id_;
};

}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_LOCAL_SSA_ELIM_PASS_H_

