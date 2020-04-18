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

#ifndef LIBSPIRV_OPT_LOCAL_SINGLE_BLOCK_ELIM_PASS_H_
#define LIBSPIRV_OPT_LOCAL_SINGLE_BLOCK_ELIM_PASS_H_


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
class LocalSingleBlockLoadStoreElimPass : public MemPass {
 public:
  LocalSingleBlockLoadStoreElimPass();
  const char* name() const override { return "eliminate-local-single-block"; }
  Status Process(ir::Module*) override;

 private:
  // Return true if all uses of |varId| are only through supported reference
  // operations ie. loads and store. Also cache in supported_ref_ptrs_;
  bool HasOnlySupportedRefs(uint32_t varId);

  // On all entry point functions, within each basic block, eliminate
  // loads and stores to function variables where possible. For
  // loads, if previous load or store to same variable, replace
  // load id with previous id and delete load. Finally, check if
  // remaining stores are useless, and delete store and variable
  // where possible. Assumes logical addressing.
  bool LocalSingleBlockLoadStoreElim(ir::Function* func);

  // Save next available id into |module|.
  inline void FinalizeNextId(ir::Module* module) {
    module->SetIdBound(next_id_);
  }

  // Return next available id and calculate next.
  inline uint32_t TakeNextId() {
    return next_id_++;
  }

  // Initialize extensions whitelist
  void InitExtensions();

  // Return true if all extensions in this module are supported by this pass.
  bool AllExtensionsSupported() const;

  void Initialize(ir::Module* module);
  Pass::Status ProcessImpl();

  // Map from function scope variable to a store of that variable in the
  // current block whose value is currently valid. This map is cleared
  // at the start of each block and incrementally updated as the block
  // is scanned. The stores are candidates for elimination. The map is
  // conservatively cleared when a function call is encountered.
  std::unordered_map<uint32_t, ir::Instruction*> var2store_;

  // Map from function scope variable to a load of that variable in the
  // current block whose value is currently valid. This map is cleared
  // at the start of each block and incrementally updated as the block
  // is scanned. The stores are candidates for elimination. The map is
  // conservatively cleared when a function call is encountered.
  std::unordered_map<uint32_t, ir::Instruction*> var2load_;

  // Set of variables whose most recent store in the current block cannot be
  // deleted, for example, if there is a load of the variable which is
  // dependent on the store and is not replaced and deleted by this pass,
  // for example, a load through an access chain. A variable is removed
  // from this set each time a new store of that variable is encountered.
  std::unordered_set<uint32_t> pinned_vars_;

  // Extensions supported by this pass.
  std::unordered_set<std::string> extensions_whitelist_;

  // Variables that are only referenced by supported operations for this
  // pass ie. loads and stores.
  std::unordered_set<uint32_t> supported_ref_ptrs_;

  // Next unused ID
  uint32_t next_id_;
};

}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_LOCAL_SINGLE_BLOCK_ELIM_PASS_H_

