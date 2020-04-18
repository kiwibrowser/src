// Copyright (c) 2015-2016 The Khronos Group Inc.
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

#include "val/validation_state.h"

#include <cassert>

#include "opcode.h"
#include "val/basic_block.h"
#include "val/construct.h"
#include "val/function.h"

using std::deque;
using std::make_pair;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

namespace libspirv {

namespace {
bool IsInstructionInLayoutSection(ModuleLayoutSection layout, SpvOp op) {
  // See Section 2.4
  bool out = false;
  // clang-format off
  switch (layout) {
    case kLayoutCapabilities:  out = op == SpvOpCapability;    break;
    case kLayoutExtensions:    out = op == SpvOpExtension;     break;
    case kLayoutExtInstImport: out = op == SpvOpExtInstImport; break;
    case kLayoutMemoryModel:   out = op == SpvOpMemoryModel;   break;
    case kLayoutEntryPoint:    out = op == SpvOpEntryPoint;    break;
    case kLayoutExecutionMode: out = op == SpvOpExecutionMode; break;
    case kLayoutDebug1:
      switch (op) {
        case SpvOpSourceContinued:
        case SpvOpSource:
        case SpvOpSourceExtension:
        case SpvOpString:
          out = true;
          break;
        default: break;
      }
      break;
    case kLayoutDebug2:
      switch (op) {
        case SpvOpName:
        case SpvOpMemberName:
          out = true;
          break;
        default: break;
      }
      break;
    case kLayoutAnnotations:
      switch (op) {
        case SpvOpDecorate:
        case SpvOpMemberDecorate:
        case SpvOpGroupDecorate:
        case SpvOpGroupMemberDecorate:
        case SpvOpDecorationGroup:
          out = true;
          break;
        default: break;
      }
      break;
    case kLayoutTypes:
      if (spvOpcodeGeneratesType(op) || spvOpcodeIsConstant(op)) {
        out = true;
        break;
      }
      switch (op) {
        case SpvOpTypeForwardPointer:
        case SpvOpVariable:
        case SpvOpLine:
        case SpvOpNoLine:
        case SpvOpUndef:
          out = true;
          break;
        default: break;
      }
      break;
    case kLayoutFunctionDeclarations:
    case kLayoutFunctionDefinitions:
      // NOTE: These instructions should NOT be in these layout sections
      if (spvOpcodeGeneratesType(op) || spvOpcodeIsConstant(op)) {
        out = false;
        break;
      }
      switch (op) {
        case SpvOpCapability:
        case SpvOpExtension:
        case SpvOpExtInstImport:
        case SpvOpMemoryModel:
        case SpvOpEntryPoint:
        case SpvOpExecutionMode:
        case SpvOpSourceContinued:
        case SpvOpSource:
        case SpvOpSourceExtension:
        case SpvOpString:
        case SpvOpName:
        case SpvOpMemberName:
        case SpvOpDecorate:
        case SpvOpMemberDecorate:
        case SpvOpGroupDecorate:
        case SpvOpGroupMemberDecorate:
        case SpvOpDecorationGroup:
        case SpvOpTypeForwardPointer:
          out = false;
          break;
      default:
        out = true;
        break;
      }
  }
  // clang-format on
  return out;
}

}  // anonymous namespace

ValidationState_t::ValidationState_t(const spv_const_context ctx,
                                     const spv_const_validator_options opt)
    : context_(ctx),
      options_(opt),
      instruction_counter_(0),
      unresolved_forward_ids_{},
      operand_names_{},
      current_layout_section_(kLayoutCapabilities),
      module_functions_(),
      module_capabilities_(),
      module_extensions_(),
      ordered_instructions_(),
      all_definitions_(),
      global_vars_(),
      local_vars_(),
      struct_nesting_depth_(),
      grammar_(ctx),
      addressing_model_(SpvAddressingModelLogical),
      memory_model_(SpvMemoryModelSimple),
      in_function_(false) {
  assert(opt && "Validator options may not be Null.");
}

spv_result_t ValidationState_t::ForwardDeclareId(uint32_t id) {
  unresolved_forward_ids_.insert(id);
  return SPV_SUCCESS;
}

spv_result_t ValidationState_t::RemoveIfForwardDeclared(uint32_t id) {
  unresolved_forward_ids_.erase(id);
  return SPV_SUCCESS;
}

spv_result_t ValidationState_t::RegisterForwardPointer(uint32_t id) {
  forward_pointer_ids_.insert(id);
  return SPV_SUCCESS;
}

bool ValidationState_t::IsForwardPointer(uint32_t id) const {
  return (forward_pointer_ids_.find(id) != forward_pointer_ids_.end());
}

void ValidationState_t::AssignNameToId(uint32_t id, string name) {
  operand_names_[id] = name;
}

string ValidationState_t::getIdName(uint32_t id) const {
  std::stringstream out;
  out << id;
  if (operand_names_.find(id) != end(operand_names_)) {
    out << "[" << operand_names_.at(id) << "]";
  }
  return out.str();
}

string ValidationState_t::getIdOrName(uint32_t id) const {
  std::stringstream out;
  if (operand_names_.find(id) != end(operand_names_)) {
    out << operand_names_.at(id);
  } else {
    out << id;
  }
  return out.str();
}

size_t ValidationState_t::unresolved_forward_id_count() const {
  return unresolved_forward_ids_.size();
}

vector<uint32_t> ValidationState_t::UnresolvedForwardIds() const {
  vector<uint32_t> out(begin(unresolved_forward_ids_),
                       end(unresolved_forward_ids_));
  return out;
}

bool ValidationState_t::IsDefinedId(uint32_t id) const {
  return all_definitions_.find(id) != end(all_definitions_);
}

const Instruction* ValidationState_t::FindDef(uint32_t id) const {
  auto it = all_definitions_.find(id);
  if (it == all_definitions_.end())
    return nullptr;
  return it->second;
}

Instruction* ValidationState_t::FindDef(uint32_t id) {
  auto it = all_definitions_.find(id);
  if (it == all_definitions_.end())
    return nullptr;
  return it->second;
}

// Increments the instruction count. Used for diagnostic
int ValidationState_t::increment_instruction_count() {
  return instruction_counter_++;
}

ModuleLayoutSection ValidationState_t::current_layout_section() const {
  return current_layout_section_;
}

void ValidationState_t::ProgressToNextLayoutSectionOrder() {
  // Guard against going past the last element(kLayoutFunctionDefinitions)
  if (current_layout_section_ <= kLayoutFunctionDefinitions) {
    current_layout_section_ =
        static_cast<ModuleLayoutSection>(current_layout_section_ + 1);
  }
}

bool ValidationState_t::IsOpcodeInCurrentLayoutSection(SpvOp op) {
  return IsInstructionInLayoutSection(current_layout_section_, op);
}

DiagnosticStream ValidationState_t::diag(spv_result_t error_code) const {
  return libspirv::DiagnosticStream(
      {0, 0, static_cast<size_t>(instruction_counter_)}, context_->consumer,
      error_code);
}

deque<Function>& ValidationState_t::functions() { return module_functions_; }

Function& ValidationState_t::current_function() {
  assert(in_function_body());
  return module_functions_.back();
}

bool ValidationState_t::in_function_body() const { return in_function_; }

bool ValidationState_t::in_block() const {
  return module_functions_.empty() == false &&
         module_functions_.back().current_block() != nullptr;
}

void ValidationState_t::RegisterCapability(SpvCapability cap) {
  // Avoid redundant work.  Otherwise the recursion could induce work
  // quadrdatic in the capability dependency depth. (Ok, not much, but
  // it's something.)
  if (module_capabilities_.Contains(cap)) return;

  module_capabilities_.Add(cap);
  spv_operand_desc desc;
  if (SPV_SUCCESS ==
      grammar_.lookupOperand(SPV_OPERAND_TYPE_CAPABILITY, cap, &desc)) {
    desc->capabilities.ForEach(
        [this](SpvCapability c) { RegisterCapability(c); });
  }

  switch (cap) {
    case SpvCapabilityInt16:
      features_.declare_int16_type = true;
      break;
    case SpvCapabilityFloat16:
    case SpvCapabilityFloat16Buffer:
      features_.declare_float16_type = true;
      break;
    case SpvCapabilityStorageUniformBufferBlock16:
    case SpvCapabilityStorageUniform16:
    case SpvCapabilityStoragePushConstant16:
    case SpvCapabilityStorageInputOutput16:
      features_.declare_int16_type = true;
      features_.declare_float16_type = true;
      features_.free_fp_rounding_mode = true;
      break;
    case SpvCapabilityVariablePointers:
      features_.variable_pointers = true;
      features_.variable_pointers_storage_buffer = true;
      break;
    case SpvCapabilityVariablePointersStorageBuffer:
      features_.variable_pointers_storage_buffer = true;
      break;
    default:
      break;
  }
}

void ValidationState_t::RegisterExtension(Extension ext) {
  if (module_extensions_.Contains(ext)) return;

  module_extensions_.Add(ext);
}

bool ValidationState_t::HasAnyOfCapabilities(
    const CapabilitySet& capabilities) const {
  return module_capabilities_.HasAnyOf(capabilities);
}

bool ValidationState_t::HasAnyOfExtensions(
    const ExtensionSet& extensions) const {
  return module_extensions_.HasAnyOf(extensions);
}

void ValidationState_t::set_addressing_model(SpvAddressingModel am) {
  addressing_model_ = am;
}

SpvAddressingModel ValidationState_t::addressing_model() const {
  return addressing_model_;
}

void ValidationState_t::set_memory_model(SpvMemoryModel mm) {
  memory_model_ = mm;
}

SpvMemoryModel ValidationState_t::memory_model() const { return memory_model_; }

spv_result_t ValidationState_t::RegisterFunction(
    uint32_t id, uint32_t ret_type_id, SpvFunctionControlMask function_control,
    uint32_t function_type_id) {
  assert(in_function_body() == false &&
         "RegisterFunction can only be called when parsing the binary outside "
         "of another function");
  in_function_ = true;
  module_functions_.emplace_back(id, ret_type_id, function_control,
                                 function_type_id);

  // TODO(umar): validate function type and type_id

  return SPV_SUCCESS;
}

spv_result_t ValidationState_t::RegisterFunctionEnd() {
  assert(in_function_body() == true &&
         "RegisterFunctionEnd can only be called when parsing the binary "
         "inside of another function");
  assert(in_block() == false &&
         "RegisterFunctionParameter can only be called when parsing the binary "
         "ouside of a block");
  current_function().RegisterFunctionEnd();
  in_function_ = false;
  return SPV_SUCCESS;
}

void ValidationState_t::RegisterInstruction(
    const spv_parsed_instruction_t& inst) {
  if (in_function_body()) {
    ordered_instructions_.emplace_back(&inst, &current_function(),
                                       current_function().current_block());
  } else {
    ordered_instructions_.emplace_back(&inst, nullptr, nullptr);
  }
  uint32_t id = ordered_instructions_.back().id();
  if (id) {
    all_definitions_.insert(make_pair(id, &ordered_instructions_.back()));
  }

  // If the instruction is using an OpTypeSampledImage as an operand, it should
  // be recorded. The validator will ensure that all usages of an
  // OpTypeSampledImage and its definition are in the same basic block.
  for (uint16_t i = 0; i < inst.num_operands; ++i) {
    const spv_parsed_operand_t& operand = inst.operands[i];
    if (SPV_OPERAND_TYPE_ID == operand.type) {
      const uint32_t operand_word = inst.words[operand.offset];
      Instruction* operand_inst = FindDef(operand_word);
      if (operand_inst && SpvOpSampledImage == operand_inst->opcode()) {
        RegisterSampledImageConsumer(operand_word, inst.result_id);
      }
    }
  }
}

std::vector<uint32_t> ValidationState_t::getSampledImageConsumers(
    uint32_t sampled_image_id) const {
  std::vector<uint32_t> result;
  auto iter = sampled_image_consumers_.find(sampled_image_id);
  if (iter != sampled_image_consumers_.end()) {
    result = iter->second;
  }
  return result;
}

void ValidationState_t::RegisterSampledImageConsumer(uint32_t sampled_image_id,
                                                     uint32_t consumer_id) {
  sampled_image_consumers_[sampled_image_id].push_back(consumer_id);
}

uint32_t ValidationState_t::getIdBound() const { return id_bound_; }

void ValidationState_t::setIdBound(const uint32_t bound) { id_bound_ = bound; }

bool ValidationState_t::RegisterUniqueTypeDeclaration(
    const spv_parsed_instruction_t& inst) {
  std::vector<uint32_t> key;
  key.push_back(static_cast<uint32_t>(inst.opcode));
  for (int index = 0; index < inst.num_operands; ++index) {
    const spv_parsed_operand_t& operand = inst.operands[index];

    if (operand.type == SPV_OPERAND_TYPE_RESULT_ID) continue;

    const int words_begin = operand.offset;
    const int words_end = words_begin + operand.num_words;
    assert(words_end <= static_cast<int>(inst.num_words));

    key.insert(key.end(), inst.words + words_begin, inst.words + words_end);
  }

  return unique_type_declarations_.insert(std::move(key)).second;
}
}  /// namespace libspirv
