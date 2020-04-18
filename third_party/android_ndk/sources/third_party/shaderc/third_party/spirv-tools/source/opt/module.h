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

#ifndef LIBSPIRV_OPT_MODULE_H_
#define LIBSPIRV_OPT_MODULE_H_

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "function.h"
#include "instruction.h"
#include "iterator.h"

namespace spvtools {
namespace ir {

// A struct for containing the module header information.
struct ModuleHeader {
  uint32_t magic_number;
  uint32_t version;
  uint32_t generator;
  uint32_t bound;
  uint32_t reserved;
};

// A SPIR-V module. It contains all the information for a SPIR-V module and
// serves as the backbone of optimization transformations.
class Module {
 public:
  using iterator = UptrVectorIterator<Function>;
  using const_iterator = UptrVectorIterator<Function, true>;
  using inst_iterator = UptrVectorIterator<Instruction>;
  using const_inst_iterator = UptrVectorIterator<Instruction, true>;

  // Creates an empty module with zero'd header.
  Module() : header_({}) {}

  // Sets the header to the given |header|.
  void SetHeader(const ModuleHeader& header) { header_ = header; }
  // Sets the Id bound.
  void SetIdBound(uint32_t bound) { header_.bound = bound; }
  // Returns the Id bound.
  uint32_t IdBound() { return header_.bound; }
  // Appends a capability instruction to this module.
  inline void AddCapability(std::unique_ptr<Instruction> c);
  // Appends an extension instruction to this module.
  inline void AddExtension(std::unique_ptr<Instruction> e);
  // Appends an extended instruction set instruction to this module.
  inline void AddExtInstImport(std::unique_ptr<Instruction> e);
  // Set the memory model for this module.
  inline void SetMemoryModel(std::unique_ptr<Instruction> m);
  // Appends an entry point instruction to this module.
  inline void AddEntryPoint(std::unique_ptr<Instruction> e);
  // Appends an execution mode instruction to this module.
  inline void AddExecutionMode(std::unique_ptr<Instruction> e);
  // Appends a debug instruction (excluding OpLine & OpNoLine) to this module.
  inline void AddDebugInst(std::unique_ptr<Instruction> d);
  // Appends an annotation instruction to this module.
  inline void AddAnnotationInst(std::unique_ptr<Instruction> a);
  // Appends a type-declaration instruction to this module.
  inline void AddType(std::unique_ptr<Instruction> t);
  // Appends a constant, global variable, or OpUndef instruction to this module.
  inline void AddGlobalValue(std::unique_ptr<Instruction> v);
  // Appends a function to this module.
  inline void AddFunction(std::unique_ptr<Function> f);

  // Returns a vector of pointers to type-declaration instructions in this
  // module.
  std::vector<Instruction*> GetTypes();
  std::vector<const Instruction*> GetTypes() const;
  // Returns a vector of pointers to constant-creation instructions in this
  // module.
  std::vector<Instruction*> GetConstants();
  std::vector<const Instruction*> GetConstants() const;

  // Return result id of global value with |opcode|, 0 if not present.
  uint32_t GetGlobalValue(SpvOp opcode) const;

  // Add global value with |opcode|, |result_id| and |type_id|
  void AddGlobalValue(SpvOp opcode, uint32_t result_id, uint32_t type_id);

  inline uint32_t id_bound() const { return header_.bound; }

  // Iterators for debug instructions (excluding OpLine & OpNoLine) contained in
  // this module.
  inline inst_iterator debug_begin();
  inline inst_iterator debug_end();
  inline IteratorRange<inst_iterator> debugs();
  inline IteratorRange<const_inst_iterator> debugs() const;

  // Iterators for entry point instructions contained in this module
  inline IteratorRange<inst_iterator> entry_points();
  inline IteratorRange<const_inst_iterator> entry_points() const;

  // Clears all debug instructions (excluding OpLine & OpNoLine).
  void debug_clear() { debugs_.clear(); }

  // Iterators for annotation instructions contained in this module.
  IteratorRange<inst_iterator> annotations();
  IteratorRange<const_inst_iterator> annotations() const;

  // Iterators for extension instructions contained in this module.
  IteratorRange<inst_iterator> extensions();
  IteratorRange<const_inst_iterator> extensions() const;

  // Iterators for types, constants and global variables instructions.
  inline inst_iterator types_values_begin();
  inline inst_iterator types_values_end();
  inline IteratorRange<inst_iterator> types_values();
  inline IteratorRange<const_inst_iterator> types_values() const;

  // Iterators for functions contained in this module.
  iterator begin() { return iterator(&functions_, functions_.begin()); }
  iterator end() { return iterator(&functions_, functions_.end()); }
  inline const_iterator cbegin() const;
  inline const_iterator cend() const;

  // Invokes function |f| on all instructions in this module, and optionally on
  // the debug line instructions that precede them.
  void ForEachInst(const std::function<void(Instruction*)>& f,
                   bool run_on_debug_line_insts = false);
  void ForEachInst(const std::function<void(const Instruction*)>& f,
                   bool run_on_debug_line_insts = false) const;

  // Pushes the binary segments for this instruction into the back of *|binary|.
  // If |skip_nop| is true and this is a OpNop, do nothing.
  void ToBinary(std::vector<uint32_t>* binary, bool skip_nop) const;

  // Returns 1 more than the maximum Id value mentioned in the module.
  uint32_t ComputeIdBound() const;

  // Returns true if module has capability |cap|
  bool HasCapability(uint32_t cap);

  // Returns id for OpExtInst instruction for extension |extstr|.
  // Returns 0 if not found.
  uint32_t GetExtInstImportId(const char* extstr);

 private:
  ModuleHeader header_;  // Module header

  // The following fields respect the "Logical Layout of a Module" in
  // Section 2.4 of the SPIR-V specification.
  std::vector<std::unique_ptr<Instruction>> capabilities_;
  std::vector<std::unique_ptr<Instruction>> extensions_;
  std::vector<std::unique_ptr<Instruction>> ext_inst_imports_;
  // A module only has one memory model instruction.
  std::unique_ptr<Instruction> memory_model_;
  std::vector<std::unique_ptr<Instruction>> entry_points_;
  std::vector<std::unique_ptr<Instruction>> execution_modes_;
  std::vector<std::unique_ptr<Instruction>> debugs_;
  std::vector<std::unique_ptr<Instruction>> annotations_;
  // Type declarations, constants, and global variable declarations.
  std::vector<std::unique_ptr<Instruction>> types_values_;
  std::vector<std::unique_ptr<Function>> functions_;
};

inline void Module::AddCapability(std::unique_ptr<Instruction> c) {
  capabilities_.emplace_back(std::move(c));
}

inline void Module::AddExtension(std::unique_ptr<Instruction> e) {
  extensions_.emplace_back(std::move(e));
}

inline void Module::AddExtInstImport(std::unique_ptr<Instruction> e) {
  ext_inst_imports_.emplace_back(std::move(e));
}

inline void Module::SetMemoryModel(std::unique_ptr<Instruction> m) {
  memory_model_ = std::move(m);
}

inline void Module::AddEntryPoint(std::unique_ptr<Instruction> e) {
  entry_points_.emplace_back(std::move(e));
}

inline void Module::AddExecutionMode(std::unique_ptr<Instruction> e) {
  execution_modes_.emplace_back(std::move(e));
}

inline void Module::AddDebugInst(std::unique_ptr<Instruction> d) {
  debugs_.emplace_back(std::move(d));
}

inline void Module::AddAnnotationInst(std::unique_ptr<Instruction> a) {
  annotations_.emplace_back(std::move(a));
}

inline void Module::AddType(std::unique_ptr<Instruction> t) {
  types_values_.emplace_back(std::move(t));
}

inline void Module::AddGlobalValue(std::unique_ptr<Instruction> v) {
  types_values_.emplace_back(std::move(v));
}

inline void Module::AddFunction(std::unique_ptr<Function> f) {
  functions_.emplace_back(std::move(f));
}

inline Module::inst_iterator Module::debug_begin() {
  return inst_iterator(&debugs_, debugs_.begin());
}
inline Module::inst_iterator Module::debug_end() {
  return inst_iterator(&debugs_, debugs_.end());
}

inline IteratorRange<Module::inst_iterator> Module::debugs() {
  return make_range(debugs_);
}

inline IteratorRange<Module::const_inst_iterator> Module::debugs() const {
  return make_const_range(debugs_);
}

inline IteratorRange<Module::inst_iterator> Module::entry_points() {
  return make_range(entry_points_);
}

inline IteratorRange<Module::const_inst_iterator> Module::entry_points() const {
  return make_const_range(entry_points_);
}

inline IteratorRange<Module::inst_iterator> Module::annotations() {
  return make_range(annotations_);
}

inline IteratorRange<Module::const_inst_iterator> Module::annotations() const {
  return make_const_range(annotations_);
}

inline IteratorRange<Module::inst_iterator> Module::extensions() {
  return make_range(extensions_);
}

inline IteratorRange<Module::const_inst_iterator> Module::extensions() const {
  return make_const_range(extensions_);
}

inline Module::inst_iterator Module::types_values_begin() {
  return inst_iterator(&types_values_, types_values_.begin());
}

inline Module::inst_iterator Module::types_values_end() {
  return inst_iterator(&types_values_, types_values_.end());
}

inline IteratorRange<Module::inst_iterator> Module::types_values() {
  return make_range(types_values_);
}

inline IteratorRange<Module::const_inst_iterator> Module::types_values() const {
  return make_const_range(types_values_);
}

inline Module::const_iterator Module::cbegin() const {
  return const_iterator(&functions_, functions_.cbegin());
}

inline Module::const_iterator Module::cend() const {
  return const_iterator(&functions_, functions_.cend());
}

}  // namespace ir
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_MODULE_H_
