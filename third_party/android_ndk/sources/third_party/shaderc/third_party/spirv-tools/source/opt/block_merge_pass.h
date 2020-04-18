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

#ifndef LIBSPIRV_OPT_BLOCK_MERGE_PASS_H_
#define LIBSPIRV_OPT_BLOCK_MERGE_PASS_H_

#include <algorithm>
#include <map>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "basic_block.h"
#include "def_use_manager.h"
#include "module.h"
#include "pass.h"

namespace spvtools {
namespace opt {

// See optimizer.hpp for documentation.
class BlockMergePass : public Pass {
 public:
  BlockMergePass();
  const char* name() const override { return "sroa"; }
  Status Process(ir::Module*) override;

 private:
  // Return true if |block_ptr| is loop header block
  bool IsLoopHeader(ir::BasicBlock* block_ptr);

  // Return true if |labId| has multiple refs. Do not count OpName.
  bool HasMultipleRefs(uint32_t labId);

  // Kill any OpName instruction referencing |inst|, then kill |inst|.
  void KillInstAndName(ir::Instruction* inst);

  // Search |func| for blocks which have a single Branch to a block
  // with no other predecessors. Merge these blocks into a single block.
  bool MergeBlocks(ir::Function* func);

  // Initialize extensions whitelist
  void InitExtensions();

  // Return true if all extensions in this module are allowed by this pass.
  bool AllExtensionsSupported() const;

  void Initialize(ir::Module* module);
  Pass::Status ProcessImpl();

  // Module this pass is processing
  ir::Module* module_;

  // Def-Uses for the module we are processing
  std::unique_ptr<analysis::DefUseManager> def_use_mgr_;

  // Extensions supported by this pass.
  std::unordered_set<std::string> extensions_whitelist_;
};

}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_BLOCK_MERGE_PASS_H_

