// Copyright (c) 2015-2016 The Khronos Group Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#include <algorithm>
#include <cassert>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "spirv/spirv.h"

#include "spirv_definition.h"
#include "validate.h"

using std::find;
using std::string;
using std::unordered_set;
using std::vector;

using libspirv::kLayoutCapabilities;
using libspirv::kLayoutExtensions;
using libspirv::kLayoutExtInstImport;
using libspirv::kLayoutMemoryModel;
using libspirv::kLayoutEntryPoint;
using libspirv::kLayoutExecutionMode;
using libspirv::kLayoutDebug1;
using libspirv::kLayoutDebug2;
using libspirv::kLayoutAnnotations;
using libspirv::kLayoutTypes;
using libspirv::kLayoutFunctionDeclarations;
using libspirv::kLayoutFunctionDefinitions;
using libspirv::ModuleLayoutSection;

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
      switch (op) {
        case SpvOpTypeVoid:
        case SpvOpTypeBool:
        case SpvOpTypeInt:
        case SpvOpTypeFloat:
        case SpvOpTypeVector:
        case SpvOpTypeMatrix:
        case SpvOpTypeImage:
        case SpvOpTypeSampler:
        case SpvOpTypeSampledImage:
        case SpvOpTypeArray:
        case SpvOpTypeRuntimeArray:
        case SpvOpTypeStruct:
        case SpvOpTypeOpaque:
        case SpvOpTypePointer:
        case SpvOpTypeFunction:
        case SpvOpTypeEvent:
        case SpvOpTypeDeviceEvent:
        case SpvOpTypeReserveId:
        case SpvOpTypeQueue:
        case SpvOpTypePipe:
        case SpvOpTypeForwardPointer:
        case SpvOpConstantTrue:
        case SpvOpConstantFalse:
        case SpvOpConstant:
        case SpvOpConstantComposite:
        case SpvOpConstantSampler:
        case SpvOpConstantNull:
        case SpvOpSpecConstantTrue:
        case SpvOpSpecConstantFalse:
        case SpvOpSpecConstant:
        case SpvOpSpecConstantComposite:
        case SpvOpSpecConstantOp:
        case SpvOpVariable:
        case SpvOpLine:
        case SpvOpNoLine:
          out = true;
          break;
        default: break;
      }
      break;
    case kLayoutFunctionDeclarations:
    case kLayoutFunctionDefinitions:
      // NOTE: These instructions should NOT be in these layout sections
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
        case SpvOpTypeVoid:
        case SpvOpTypeBool:
        case SpvOpTypeInt:
        case SpvOpTypeFloat:
        case SpvOpTypeVector:
        case SpvOpTypeMatrix:
        case SpvOpTypeImage:
        case SpvOpTypeSampler:
        case SpvOpTypeSampledImage:
        case SpvOpTypeArray:
        case SpvOpTypeRuntimeArray:
        case SpvOpTypeStruct:
        case SpvOpTypeOpaque:
        case SpvOpTypePointer:
        case SpvOpTypeFunction:
        case SpvOpTypeEvent:
        case SpvOpTypeDeviceEvent:
        case SpvOpTypeReserveId:
        case SpvOpTypeQueue:
        case SpvOpTypePipe:
        case SpvOpTypeForwardPointer:
        case SpvOpConstantTrue:
        case SpvOpConstantFalse:
        case SpvOpConstant:
        case SpvOpConstantComposite:
        case SpvOpConstantSampler:
        case SpvOpConstantNull:
        case SpvOpSpecConstantTrue:
        case SpvOpSpecConstantFalse:
        case SpvOpSpecConstant:
        case SpvOpSpecConstantComposite:
        case SpvOpSpecConstantOp:
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

namespace libspirv {

ValidationState_t::ValidationState_t(spv_diagnostic* diagnostic,
                                     const spv_const_context context)
    : diagnostic_(diagnostic),
      instruction_counter_(0),
      unresolved_forward_ids_{},
      operand_names_{},
      current_layout_section_(kLayoutCapabilities),
      module_functions_(*this),
      module_capabilities_(0u),
      grammar_(context) {}

spv_result_t ValidationState_t::forwardDeclareId(uint32_t id) {
  unresolved_forward_ids_.insert(id);
  return SPV_SUCCESS;
}

spv_result_t ValidationState_t::removeIfForwardDeclared(uint32_t id) {
  unresolved_forward_ids_.erase(id);
  return SPV_SUCCESS;
}

void ValidationState_t::assignNameToId(uint32_t id, string name) {
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

size_t ValidationState_t::unresolvedForwardIdCount() const {
  return unresolved_forward_ids_.size();
}

vector<uint32_t> ValidationState_t::unresolvedForwardIds() const {
  vector<uint32_t> out(begin(unresolved_forward_ids_),
                       end(unresolved_forward_ids_));
  return out;
}

bool ValidationState_t::isDefinedId(uint32_t id) const {
  return usedefs_.FindDef(id).first;
}

// Increments the instruction count. Used for diagnostic
int ValidationState_t::incrementInstructionCount() {
  return instruction_counter_++;
}

ModuleLayoutSection ValidationState_t::getLayoutSection() const {
  return current_layout_section_;
}

void ValidationState_t::progressToNextLayoutSectionOrder() {
  // Guard against going past the last element(kLayoutFunctionDefinitions)
  if (current_layout_section_ <= kLayoutFunctionDefinitions) {
    current_layout_section_ =
        static_cast<ModuleLayoutSection>(current_layout_section_ + 1);
  }
}

bool ValidationState_t::isOpcodeInCurrentLayoutSection(SpvOp op) {
  return IsInstructionInLayoutSection(current_layout_section_, op);
}

DiagnosticStream ValidationState_t::diag(spv_result_t error_code) const {
  return libspirv::DiagnosticStream(
      {0, 0, static_cast<size_t>(instruction_counter_)}, diagnostic_,
      error_code);
}

Functions& ValidationState_t::get_functions() { return module_functions_; }

bool ValidationState_t::in_function_body() const {
  return module_functions_.in_function_body();
}

bool ValidationState_t::in_block() const {
  return module_functions_.in_block();
}

void ValidationState_t::registerCapability(SpvCapability cap) {
  module_capabilities_ |= SPV_CAPABILITY_AS_MASK(cap);
  spv_operand_desc desc;
  if (SPV_SUCCESS ==
      grammar_.lookupOperand(SPV_OPERAND_TYPE_CAPABILITY, cap, &desc))
    libspirv::ForEach(desc->capabilities,
                      [this](SpvCapability c) { registerCapability(c); });
}

bool ValidationState_t::hasCapability(SpvCapability cap) const {
  return module_capabilities_ & SPV_CAPABILITY_AS_MASK(cap);
}

bool ValidationState_t::HasAnyOf(spv_capability_mask_t capabilities) const {
  if (!capabilities)
    return true;  // No capabilities requested: trivially satisfied.
  bool found = false;
  libspirv::ForEach(capabilities, [&found, this](SpvCapability c) {
    found |= hasCapability(c);
  });
  return found;
}

Functions::Functions(ValidationState_t& module)
    : module_(module), in_function_(false), in_block_(false) {}

bool Functions::in_function_body() const { return in_function_; }

bool Functions::in_block() const { return in_block_; }

spv_result_t Functions::RegisterFunction(uint32_t id, uint32_t ret_type_id,
                                         uint32_t function_control,
                                         uint32_t function_type_id) {
  assert(in_function_ == false &&
         "Function instructions can not be declared in a function");
  in_function_ = true;
  id_.emplace_back(id);
  type_id_.emplace_back(function_type_id);
  declaration_type_.emplace_back(FunctionDecl::kFunctionDeclUnknown);
  block_ids_.emplace_back();
  variable_ids_.emplace_back();
  parameter_ids_.emplace_back();

  // TODO(umar): validate function type and type_id
  (void)ret_type_id;
  (void)function_control;

  return SPV_SUCCESS;
}

spv_result_t Functions::RegisterFunctionParameter(uint32_t id,
                                                  uint32_t type_id) {
  assert(in_function_ == true &&
         "Function parameter instructions cannot be declared outside of a "
         "function");
  assert(in_block() == false &&
         "Function parameters cannot be called in blocks");
  // TODO(umar): Validate function parameter type order and count
  // TODO(umar): Use these variables to validate parameter type
  (void)id;
  (void)type_id;
  return SPV_SUCCESS;
}

spv_result_t Functions::RegisterSetFunctionDeclType(FunctionDecl type) {
  assert(declaration_type_.back() == FunctionDecl::kFunctionDeclUnknown);
  declaration_type_.back() = type;
  return SPV_SUCCESS;
}

spv_result_t Functions::RegisterBlock(uint32_t id) {
  assert(in_function_ == true && "Blocks can only exsist in functions");
  assert(in_block_ == false && "Blocks cannot be nested");
  assert(module_.getLayoutSection() !=
             ModuleLayoutSection::kLayoutFunctionDeclarations &&
         "Function declartions must appear before function definitions");
  assert(declaration_type_.back() == FunctionDecl::kFunctionDeclDefinition &&
         "Function declaration type should have already been defined");

  block_ids_.back().push_back(id);
  in_block_ = true;
  return SPV_SUCCESS;
}

spv_result_t Functions::RegisterFunctionEnd() {
  assert(in_function_ == true &&
         "Function end can only be called in functions");
  assert(in_block_ == false && "Function end cannot be called inside a block");
  in_function_ = false;
  return SPV_SUCCESS;
}

spv_result_t Functions::RegisterBlockEnd() {
  assert(in_function_ == true &&
         "Branch instruction can only be called in a function");
  assert(in_block_ == true &&
         "Branch instruction can only be called in a block");
  in_block_ = false;
  return SPV_SUCCESS;
}

size_t Functions::get_block_count() const { return block_ids_.back().size(); }
}
