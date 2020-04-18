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

#include "basic_block.h"

namespace spvtools {
namespace ir {

const Instruction* BasicBlock::GetMergeInst() const {
  const Instruction* result = nullptr;
  // If it exists, the merge instruction immediately precedes the
  // terminator.
  auto iter = ctail();
  if (iter != cbegin()) {
    --iter;
    const auto opcode = iter->opcode();
    if (opcode == SpvOpLoopMerge || opcode  == SpvOpSelectionMerge) {
      result = &*iter;
    }
  }
  return result;
}

Instruction* BasicBlock::GetMergeInst() {
  Instruction* result = nullptr;
  // If it exists, the merge instruction immediately precedes the
  // terminator.
  auto iter = tail();
  if (iter != begin()) {
    --iter;
    const auto opcode = iter->opcode();
    if (opcode == SpvOpLoopMerge || opcode  == SpvOpSelectionMerge) {
      result = &*iter;
    }
  }
  return result;
}

const Instruction* BasicBlock::GetLoopMergeInst() const {
  if (auto* merge = GetMergeInst()) {
    if (merge->opcode() == SpvOpLoopMerge) {
      return merge;
    }
  }
  return nullptr;
}

Instruction* BasicBlock::GetLoopMergeInst() {
  if (auto* merge = GetMergeInst()) {
    if (merge->opcode() == SpvOpLoopMerge) {
      return merge;
    }
  }
  return nullptr;
}

void BasicBlock::ForEachSuccessorLabel(
    const std::function<void(const uint32_t)>& f) {
  const auto br = &*insts_.back();
  switch (br->opcode()) {
    case SpvOpBranch: {
      f(br->GetOperand(0).words[0]);
    } break;
    case SpvOpBranchConditional:
    case SpvOpSwitch: {
      bool is_first = true;
      br->ForEachInId([&is_first, &f](const uint32_t* idp) {
        if (!is_first) f(*idp);
        is_first = false;
      });
    } break;
    default:
      break;
  }
}

void BasicBlock::ForMergeAndContinueLabel(
    const std::function<void(const uint32_t)>& f) {
  auto ii = insts_.end();
  --ii;
  if (ii == insts_.begin()) return;
  --ii;
  if ((*ii)->opcode() == SpvOpSelectionMerge || 
      (*ii)->opcode() == SpvOpLoopMerge)
    (*ii)->ForEachInId([&f](const uint32_t* idp) {
      f(*idp);
    });
}

}  // namespace ir
}  // namespace spvtools

