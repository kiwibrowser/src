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

#include "function.h"

namespace spvtools {
namespace ir {

void Function::ForEachInst(const std::function<void(Instruction*)>& f,
                           bool run_on_debug_line_insts) {
  if (def_inst_) def_inst_->ForEachInst(f, run_on_debug_line_insts);
  for (auto& param : params_) param->ForEachInst(f, run_on_debug_line_insts);
  for (auto& bb : blocks_) bb->ForEachInst(f, run_on_debug_line_insts);
  if (end_inst_) end_inst_->ForEachInst(f, run_on_debug_line_insts);
}

void Function::ForEachInst(const std::function<void(const Instruction*)>& f,
                           bool run_on_debug_line_insts) const {
  if (def_inst_)
    static_cast<const Instruction*>(def_inst_.get())
        ->ForEachInst(f, run_on_debug_line_insts);

  for (const auto& param : params_)
    static_cast<const Instruction*>(param.get())
        ->ForEachInst(f, run_on_debug_line_insts);

  for (const auto& bb : blocks_)
    static_cast<const BasicBlock*>(bb.get())
        ->ForEachInst(f, run_on_debug_line_insts);

  if (end_inst_)
    static_cast<const Instruction*>(end_inst_.get())
        ->ForEachInst(f, run_on_debug_line_insts);
}

void Function::ForEachParam(const std::function<void(const Instruction*)>& f,
                            bool run_on_debug_line_insts) const {
  for (const auto& param : params_)
    static_cast<const Instruction*>(param.get())
        ->ForEachInst(f, run_on_debug_line_insts);
}

}  // namespace ir
}  // namespace spvtools
