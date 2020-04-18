// Copyright (c) 2016 Google Inc.
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

#ifndef LIBSPIRV_OPT_DEF_USE_MANAGER_H_
#define LIBSPIRV_OPT_DEF_USE_MANAGER_H_

#include <list>
#include <unordered_map>
#include <vector>

#include "instruction.h"
#include "module.h"
#include "spirv-tools/libspirv.hpp"

namespace spvtools {
namespace opt {
namespace analysis {

// Class for representing a use of id. Note that:
// * Result type id is a use.
// * Ids referenced in OpSectionMerge & OpLoopMerge are considered as use.
// * Ids referenced in OpPhi's in operands are considered as use.
struct Use {
  ir::Instruction* inst;   // Instruction using the id.
  uint32_t operand_index;  // logical operand index of the id use. This can be
                           // the index of result type id.
};

using UseList = std::list<Use>;

// A class for analyzing and managing defs and uses in an ir::Module.
class DefUseManager {
 public:
  using IdToDefMap = std::unordered_map<uint32_t, ir::Instruction*>;
  using IdToUsesMap = std::unordered_map<uint32_t, UseList>;

  // Constructs a def-use manager from the given |module|. All internal messages
  // will be communicated to the outside via the given message |consumer|. This
  // instance only keeps a reference to the |consumer|, so the |consumer| should
  // outlive this instance.
  DefUseManager(const MessageConsumer& consumer, ir::Module* module)
      : consumer_(consumer) {
    AnalyzeDefUse(module);
  }

  DefUseManager(const DefUseManager&) = delete;
  DefUseManager(DefUseManager&&) = delete;
  DefUseManager& operator=(const DefUseManager&) = delete;
  DefUseManager& operator=(DefUseManager&&) = delete;

  // Analyzes the defs in the given |inst|.
  void AnalyzeInstDef(ir::Instruction* inst);

  // Analyzes the uses in the given |inst|.
  void AnalyzeInstUse(ir::Instruction* inst);

  // Analyzes the defs and uses in the given |inst|.
  void AnalyzeInstDefUse(ir::Instruction* inst);

  // Returns the def instruction for the given |id|. If there is no instruction
  // defining |id|, returns nullptr.
  ir::Instruction* GetDef(uint32_t id);
  // Returns the use instructions for the given |id|. If there is no uses of
  // |id|, returns nullptr.
  UseList* GetUses(uint32_t id);
  const UseList* GetUses(uint32_t id) const;
  // Returns the annotation instrunctions which are a direct use of the given
  // |id|. This means when the decorations are applied through decoration
  // group(s), this function will just return the OpGroupDecorate
  // instrcution(s) which refer to the given id as an operand. The OpDecorate
  // instructions which decorate the decoration group will not be returned.
  std::vector<ir::Instruction*> GetAnnotations(uint32_t id) const;

  // Returns the map from ids to their def instructions.
  const IdToDefMap& id_to_defs() const { return id_to_def_; }
  // Returns the map from ids to their uses in instructions.
  const IdToUsesMap& id_to_uses() const { return id_to_uses_; }

  // Turns the instruction defining the given |id| into a Nop. Returns true on
  // success, false if the given |id| is not defined at all. This method also
  // erases both the uses of |id| and the information of this |id|-generating
  // instruction's uses of its operands.
  bool KillDef(uint32_t id);
  // Turns the given instruction |inst| to a Nop. This method erases the
  // information of the given instruction's uses of its operands. If |inst|
  // defines an result id, the uses of the result id will also be erased.
  void KillInst(ir::Instruction* inst);
  // Replaces all uses of |before| id with |after| id. Returns true if any
  // replacement happens. This method does not kill the definition of the
  // |before| id. If |after| is the same as |before|, does nothing and returns
  // false.
  bool ReplaceAllUsesWith(uint32_t before, uint32_t after);

 private:
  using InstToUsedIdsMap =
      std::unordered_map<const ir::Instruction*, std::vector<uint32_t>>;

  // Analyzes the defs and uses in the given |module| and populates data
  // structures in this class. Does nothing if |module| is nullptr.
  void AnalyzeDefUse(ir::Module* module);

  // Clear the internal def-use record of the given instruction |inst|. This
  // method will update the use information of the operand ids of |inst|. The
  // record: |inst| uses an |id|, will be removed from the use records of |id|.
  // If |inst| defines an result id, the use record of this result id will also
  // be removed. Does nothing if |inst| was not analyzed before.
  void ClearInst(ir::Instruction* inst);

  // Erases the records that a given instruction uses its operand ids.
  void EraseUseRecordsOfOperandIds(const ir::Instruction* inst);

  const MessageConsumer& consumer_;  // Message consumer.
  IdToDefMap id_to_def_;             // Mapping from ids to their definitions
  IdToUsesMap id_to_uses_;           // Mapping from ids to their uses
  // Mapping from instructions to the ids used in the instructions generating
  // the result ids.
  InstToUsedIdsMap inst_to_used_ids_;
};

}  // namespace analysis
}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_DEF_USE_MANAGER_H_
