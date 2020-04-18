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

#ifndef LIBSPIRV_OPT_CONSTRUCTS_H_
#define LIBSPIRV_OPT_CONSTRUCTS_H_

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "basic_block.h"
#include "instruction.h"
#include "iterator.h"

namespace spvtools {
namespace ir {

class Module;

// A SPIR-V function.
class Function {
 public:
  using iterator = UptrVectorIterator<BasicBlock>;
  using const_iterator = UptrVectorIterator<BasicBlock, true>;

  // Creates a function instance declared by the given OpFunction instruction
  // |def_inst|.
  inline explicit Function(std::unique_ptr<Instruction> def_inst);
  // The OpFunction instruction that begins the definition of this function.
  Instruction& DefInst() { return *def_inst_; }

  // Sets the enclosing module for this function.
  void SetParent(Module* module) { module_ = module; }
  // Appends a parameter to this function.
  inline void AddParameter(std::unique_ptr<Instruction> p);
  // Appends a basic block to this function.
  inline void AddBasicBlock(std::unique_ptr<BasicBlock> b);

  // Saves the given function end instruction.
  inline void SetFunctionEnd(std::unique_ptr<Instruction> end_inst);

  // Returns function's id
  inline uint32_t result_id() const { return def_inst_->result_id(); }

  // Returns function's type id
  inline uint32_t type_id() const { return def_inst_->type_id(); }

  iterator begin() { return iterator(&blocks_, blocks_.begin()); }
  iterator end() { return iterator(&blocks_, blocks_.end()); }
  const_iterator cbegin() const {
    return const_iterator(&blocks_, blocks_.cbegin());
  }
  const_iterator cend() const {
    return const_iterator(&blocks_, blocks_.cend());
  }

  // Runs the given function |f| on each instruction in this function, and
  // optionally on debug line instructions that might precede them.
  void ForEachInst(const std::function<void(Instruction*)>& f,
                   bool run_on_debug_line_insts = false);
  void ForEachInst(const std::function<void(const Instruction*)>& f,
                   bool run_on_debug_line_insts = false) const;

  // Runs the given function |f| on each parameter instruction in this function,
  // and optionally on debug line instructions that might precede them.
  void ForEachParam(const std::function<void(const Instruction*)>& f,
                    bool run_on_debug_line_insts = false) const;

 private:
  // The enclosing module.
  Module* module_;
  // The OpFunction instruction that begins the definition of this function.
  std::unique_ptr<Instruction> def_inst_;
  // All parameters to this function.
  std::vector<std::unique_ptr<Instruction>> params_;
  // All basic blocks inside this function in specification order
  std::vector<std::unique_ptr<BasicBlock>> blocks_;
  // The OpFunctionEnd instruction.
  std::unique_ptr<Instruction> end_inst_;
};

inline Function::Function(std::unique_ptr<Instruction> def_inst)
    : module_(nullptr), def_inst_(std::move(def_inst)), end_inst_() {}

inline void Function::AddParameter(std::unique_ptr<Instruction> p) {
  params_.emplace_back(std::move(p));
}

inline void Function::AddBasicBlock(std::unique_ptr<BasicBlock> b) {
  blocks_.emplace_back(std::move(b));
}

inline void Function::SetFunctionEnd(std::unique_ptr<Instruction> end_inst) {
  end_inst_ = std::move(end_inst);
}

}  // namespace ir
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_CONSTRUCTS_H_
