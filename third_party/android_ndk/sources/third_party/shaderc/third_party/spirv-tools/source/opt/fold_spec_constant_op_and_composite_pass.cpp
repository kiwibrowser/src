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

#include "fold_spec_constant_op_and_composite_pass.h"

#include <algorithm>
#include <initializer_list>
#include <tuple>

#include "constants.h"
#include "make_unique.h"

namespace spvtools {
namespace opt {

namespace {
// Returns the single-word result from performing the given unary operation on
// the operand value which is passed in as a 32-bit word.
uint32_t UnaryOperate(SpvOp opcode, uint32_t operand) {
  switch (opcode) {
    // Arthimetics
    case SpvOp::SpvOpSNegate:
      return -static_cast<int32_t>(operand);
    case SpvOp::SpvOpNot:
      return ~operand;
    case SpvOp::SpvOpLogicalNot:
      return !static_cast<bool>(operand);
    default:
      assert(false &&
             "Unsupported unary operation for OpSpecConstantOp instruction");
      return 0u;
  }
}

// Returns the single-word result from performing the given binary operation on
// the operand values which are passed in as two 32-bit word.
uint32_t BinaryOperate(SpvOp opcode, uint32_t a, uint32_t b) {
  switch (opcode) {
    // Arthimetics
    case SpvOp::SpvOpIAdd:
      return a + b;
    case SpvOp::SpvOpISub:
      return a - b;
    case SpvOp::SpvOpIMul:
      return a * b;
    case SpvOp::SpvOpUDiv:
      assert(b != 0);
      return a / b;
    case SpvOp::SpvOpSDiv:
      assert(b != 0u);
      return (static_cast<int32_t>(a)) / (static_cast<int32_t>(b));
    case SpvOp::SpvOpSRem: {
      // The sign of non-zero result comes from the first operand: a. This is
      // guaranteed by C++11 rules for integer division operator. The division
      // result is rounded toward zero, so the result of '%' has the sign of
      // the first operand.
      assert(b != 0u);
      return static_cast<int32_t>(a) % static_cast<int32_t>(b);
    }
    case SpvOp::SpvOpSMod: {
      // The sign of non-zero result comes from the second operand: b
      assert(b != 0u);
      int32_t rem = BinaryOperate(SpvOp::SpvOpSRem, a, b);
      int32_t b_prim = static_cast<int32_t>(b);
      return (rem + b_prim) % b_prim;
    }
    case SpvOp::SpvOpUMod:
      assert(b != 0u);
      return (a % b);

    // Shifting
    case SpvOp::SpvOpShiftRightLogical: {
      return a >> b;
    }
    case SpvOp::SpvOpShiftRightArithmetic:
      return (static_cast<int32_t>(a)) >> b;
    case SpvOp::SpvOpShiftLeftLogical:
      return a << b;

    // Bitwise operations
    case SpvOp::SpvOpBitwiseOr:
      return a | b;
    case SpvOp::SpvOpBitwiseAnd:
      return a & b;
    case SpvOp::SpvOpBitwiseXor:
      return a ^ b;

    // Logical
    case SpvOp::SpvOpLogicalEqual:
      return (static_cast<bool>(a)) == (static_cast<bool>(b));
    case SpvOp::SpvOpLogicalNotEqual:
      return (static_cast<bool>(a)) != (static_cast<bool>(b));
    case SpvOp::SpvOpLogicalOr:
      return (static_cast<bool>(a)) || (static_cast<bool>(b));
    case SpvOp::SpvOpLogicalAnd:
      return (static_cast<bool>(a)) && (static_cast<bool>(b));

    // Comparison
    case SpvOp::SpvOpIEqual:
      return a == b;
    case SpvOp::SpvOpINotEqual:
      return a != b;
    case SpvOp::SpvOpULessThan:
      return a < b;
    case SpvOp::SpvOpSLessThan:
      return (static_cast<int32_t>(a)) < (static_cast<int32_t>(b));
    case SpvOp::SpvOpUGreaterThan:
      return a > b;
    case SpvOp::SpvOpSGreaterThan:
      return (static_cast<int32_t>(a)) > (static_cast<int32_t>(b));
    case SpvOp::SpvOpULessThanEqual:
      return a <= b;
    case SpvOp::SpvOpSLessThanEqual:
      return (static_cast<int32_t>(a)) <= (static_cast<int32_t>(b));
    case SpvOp::SpvOpUGreaterThanEqual:
      return a >= b;
    case SpvOp::SpvOpSGreaterThanEqual:
      return (static_cast<int32_t>(a)) >= (static_cast<int32_t>(b));
    default:
      assert(false &&
             "Unsupported binary operation for OpSpecConstantOp instruction");
      return 0u;
  }
}

// Returns the single-word result from performing the given ternary operation
// on the operand values which are passed in as three 32-bit word.
uint32_t TernaryOperate(SpvOp opcode, uint32_t a, uint32_t b, uint32_t c) {
  switch (opcode) {
    case SpvOp::SpvOpSelect:
      return (static_cast<bool>(a)) ? b : c;
    default:
      assert(false &&
             "Unsupported ternary operation for OpSpecConstantOp instruction");
      return 0u;
  }
}

// Returns the single-word result from performing the given operation on the
// operand words. This only works with 32-bit operations and uses boolean
// convention that 0u is false, and anything else is boolean true.
// TODO(qining): Support operands other than 32-bit wide.
uint32_t OperateWords(SpvOp opcode,
                      const std::vector<uint32_t>& operand_words) {
  switch (operand_words.size()) {
    case 1:
      return UnaryOperate(opcode, operand_words.front());
    case 2:
      return BinaryOperate(opcode, operand_words.front(), operand_words.back());
    case 3:
      return TernaryOperate(opcode, operand_words[0], operand_words[1],
                            operand_words[2]);
    default:
      assert(false && "Invalid number of operands");
      return 0;
  }
}

// Returns the result of performing an operation on scalar constant operands.
// This function extracts the operand values as 32 bit words and returns the
// result in 32 bit word. Scalar constants with longer than 32-bit width are
// not accepted in this function.
uint32_t OperateScalars(SpvOp opcode,
                        const std::vector<analysis::Constant*>& operands) {
  std::vector<uint32_t> operand_values_in_raw_words;
  for (analysis::Constant* operand : operands) {
    if (analysis::ScalarConstant* scalar = operand->AsScalarConstant()) {
      const auto& scalar_words = scalar->words();
      assert(scalar_words.size() == 1 &&
             "Scalar constants with longer than 32-bit width are not allowed "
             "in OperateScalars()");
      operand_values_in_raw_words.push_back(scalar_words.front());
    } else if (operand->AsNullConstant()) {
      operand_values_in_raw_words.push_back(0u);
    } else {
      assert(false &&
             "OperateScalars() only accepts ScalarConst or NullConst type of "
             "constant");
    }
  }
  return OperateWords(opcode, operand_values_in_raw_words);
}

// Returns the result of performing an operation over constant vectors. This
// function iterates through the given vector type constant operands and
// calculates the result for each element of the result vector to return.
// Vectors with longer than 32-bit scalar components are not accepted in this
// function.
std::vector<uint32_t> OperateVectors(
    SpvOp opcode, uint32_t num_dims,
    const std::vector<analysis::Constant*>& operands) {
  std::vector<uint32_t> result;
  for (uint32_t d = 0; d < num_dims; d++) {
    std::vector<uint32_t> operand_values_for_one_dimension;
    for (analysis::Constant* operand : operands) {
      if (analysis::VectorConstant* vector_operand =
              operand->AsVectorConstant()) {
        // Extract the raw value of the scalar component constants
        // in 32-bit words here. The reason of not using OperateScalars() here
        // is that we do not create temporary null constants as components
        // when the vector operand is a NullConstant because Constant creation
        // may need extra checks for the validity and that is not manageed in
        // here.
        if (const analysis::ScalarConstant* scalar_component =
                vector_operand->GetComponents().at(d)->AsScalarConstant()) {
          const auto& scalar_words = scalar_component->words();
          assert(
              scalar_words.size() == 1 &&
              "Vector components with longer than 32-bit width are not allowed "
              "in OperateVectors()");
          operand_values_for_one_dimension.push_back(scalar_words.front());
        } else if (operand->AsNullConstant()) {
          operand_values_for_one_dimension.push_back(0u);
        } else {
          assert(false &&
                 "VectorConst should only has ScalarConst or NullConst as "
                 "components");
        }
      } else if (operand->AsNullConstant()) {
        operand_values_for_one_dimension.push_back(0u);
      } else {
        assert(false &&
               "OperateVectors() only accepts VectorConst or NullConst type of "
               "constant");
      }
    }
    result.push_back(OperateWords(opcode, operand_values_for_one_dimension));
  }
  return result;
}
}  // anonymous namespace

FoldSpecConstantOpAndCompositePass::FoldSpecConstantOpAndCompositePass()
    : max_id_(0),
      module_(nullptr),
      def_use_mgr_(nullptr),
      type_mgr_(nullptr),
      id_to_const_val_() {}

Pass::Status FoldSpecConstantOpAndCompositePass::Process(ir::Module* module) {
  Initialize(module);
  return ProcessImpl(module);
}

void FoldSpecConstantOpAndCompositePass::Initialize(ir::Module* module) {
  type_mgr_.reset(new analysis::TypeManager(consumer(), *module));
  def_use_mgr_.reset(new analysis::DefUseManager(consumer(), module));
  for (const auto& id_def : def_use_mgr_->id_to_defs()) {
    max_id_ = std::max(max_id_, id_def.first);
  }
  module_ = module;
};

Pass::Status FoldSpecConstantOpAndCompositePass::ProcessImpl(
    ir::Module* module) {
  bool modified = false;
  // Traverse through all the constant defining instructions. For Normal
  // Constants whose values are determined and do not depend on OpUndef
  // instructions, records their values in two internal maps: id_to_const_val_
  // and const_val_to_id_ so that we can use them to infer the value of Spec
  // Constants later.
  // For Spec Constants defined with OpSpecConstantComposite instructions, if
  // all of their components are Normal Constants, they will be turned into
  // Normal Constants too. For Spec Constants defined with OpSpecConstantOp
  // instructions, we check if they only depends on Normal Constants and fold
  // them when possible. The two maps for Normal Constants: id_to_const_val_
  // and const_val_to_id_ will be updated along the traversal so that the new
  // Normal Constants generated from folding can be used to fold following Spec
  // Constants.
  // This algorithm depends on the SSA property of SPIR-V when
  // defining constants. The dependent constants must be defined before the
  // dependee constants. So a dependent Spec Constant must be defined and
  // will be processed before its dependee Spec Constant. When we encounter
  // the dependee Spec Constants, all its dependent constants must have been
  // processed and all its dependent Spec Constants should have been folded if
  // possible.
  for (ir::Module::inst_iterator inst_iter = module->types_values_begin();
       // Need to re-evaluate the end iterator since we may modify the list of
       // instructions in this section of the module as the process goes.
       inst_iter != module->types_values_end(); ++inst_iter) {
    ir::Instruction* inst = &*inst_iter;
    // Collect constant values of normal constants and process the
    // OpSpecConstantOp and OpSpecConstantComposite instructions if possible.
    // The constant values will be stored in analysis::Constant instances.
    // OpConstantSampler instruction is not collected here because it cannot be
    // used in OpSpecConstant{Composite|Op} instructions.
    // TODO(qining): If the constant or its type has decoration, we may need
    // to skip it.
    if (GetType(inst) && !GetType(inst)->decoration_empty()) continue;
    switch (SpvOp opcode = inst->opcode()) {
      // Records the values of Normal Constants.
      case SpvOp::SpvOpConstantTrue:
      case SpvOp::SpvOpConstantFalse:
      case SpvOp::SpvOpConstant:
      case SpvOp::SpvOpConstantNull:
      case SpvOp::SpvOpConstantComposite:
      case SpvOp::SpvOpSpecConstantComposite: {
        // A Constant instance will be created if the given instruction is a
        // Normal Constant whose value(s) are fixed. Note that for a composite
        // Spec Constant defined with OpSpecConstantComposite instruction, if
        // all of its components are Normal Constants already, the Spec
        // Constant will be turned in to a Normal Constant. In that case, a
        // Constant instance should also be created successfully and recorded
        // in the id_to_const_val_ and const_val_to_id_ mapps.
        if (auto const_value = CreateConstFromInst(inst)) {
          // Need to replace the OpSpecConstantComposite instruction with a
          // corresponding OpConstantComposite instruction.
          if (opcode == SpvOp::SpvOpSpecConstantComposite) {
            inst->SetOpcode(SpvOp::SpvOpConstantComposite);
            modified = true;
          }
          const_val_to_id_[const_value.get()] = inst->result_id();
          id_to_const_val_[inst->result_id()] = std::move(const_value);
        }
        break;
      }
      // For a Spec Constants defined with OpSpecConstantOp instruction, check
      // if it only depends on Normal Constants. If so, the Spec Constant will
      // be folded. The original Spec Constant defining instruction will be
      // replaced by Normal Constant defining instructions, and the new Normal
      // Constants will be added to id_to_const_val_ and const_val_to_id_ so
      // that we can use the new Normal Constants when folding following Spec
      // Constants.
      case SpvOp::SpvOpSpecConstantOp:
        modified |= ProcessOpSpecConstantOp(&inst_iter);
        break;
      default:
        break;
    }
  }
  return modified ? Status::SuccessWithChange : Status::SuccessWithoutChange;
}

bool FoldSpecConstantOpAndCompositePass::ProcessOpSpecConstantOp(
    ir::Module::inst_iterator* pos) {
  ir::Instruction* inst = &**pos;
  ir::Instruction* folded_inst = nullptr;
  assert(inst->GetInOperand(0).type ==
             SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER &&
         "The first in-operand of OpSpecContantOp instruction must be of "
         "SPV_OPERAND_TYPE_SPEC_CONSTANT_OP_NUMBER type");

  switch (static_cast<SpvOp>(inst->GetSingleWordInOperand(0))) {
    case SpvOp::SpvOpCompositeExtract:
      folded_inst = DoCompositeExtract(pos);
      break;
    case SpvOp::SpvOpVectorShuffle:
      folded_inst = DoVectorShuffle(pos);
      break;

    case SpvOp::SpvOpCompositeInsert:
      // Current Glslang does not generate code with OpSpecConstantOp
      // CompositeInsert instruction, so this is not implmented so far.
      // TODO(qining): Implement CompositeInsert case.
      return false;

    default:
      // Component-wise operations.
      folded_inst = DoComponentWiseOperation(pos);
      break;
  }
  if (!folded_inst) return false;

  // Replace the original constant with the new folded constant, kill the
  // original constant.
  uint32_t new_id = folded_inst->result_id();
  uint32_t old_id = inst->result_id();
  def_use_mgr_->ReplaceAllUsesWith(old_id, new_id);
  def_use_mgr_->KillDef(old_id);
  return true;
}

ir::Instruction* FoldSpecConstantOpAndCompositePass::DoCompositeExtract(
    ir::Module::inst_iterator* pos) {
  ir::Instruction* inst = &**pos;
  assert(inst->NumInOperands() - 1 >= 2 &&
         "OpSpecConstantOp CompositeExtract requires at least two non-type "
         "non-opcode operands.");
  assert(inst->GetInOperand(1).type == SPV_OPERAND_TYPE_ID &&
         "The vector operand must have a SPV_OPERAND_TYPE_ID type");
  assert(
      inst->GetInOperand(2).type == SPV_OPERAND_TYPE_LITERAL_INTEGER &&
      "The literal operand must have a SPV_OPERAND_TYPE_LITERAL_INTEGER type");

  // Note that for OpSpecConstantOp, the second in-operand is the first id
  // operand. The first in-operand is the spec opcode.
  analysis::Constant* first_operand_const =
      FindRecordedConst(inst->GetSingleWordInOperand(1));
  if (!first_operand_const) return nullptr;

  const analysis::Constant* current_const = first_operand_const;
  for (uint32_t i = 2; i < inst->NumInOperands(); i++) {
    uint32_t literal = inst->GetSingleWordInOperand(i);
    if (const analysis::CompositeConstant* composite_const =
            current_const->AsCompositeConstant()) {
      // Case 1: current constant is a non-null composite type constant.
      assert(literal < composite_const->GetComponents().size() &&
             "Literal index out of bound of the composite constant");
      current_const = composite_const->GetComponents().at(literal);
    } else if (current_const->AsNullConstant()) {
      // Case 2: current constant is a constant created with OpConstantNull.
      // Because components of a NullConstant are always NullConstants, we can
      // return early with a NullConstant in the result type.
      return BuildInstructionAndAddToModule(CreateConst(GetType(inst), {}),
                                            pos);
    } else {
      // Dereferencing a non-composite constant. Invalid case.
      return nullptr;
    }
  }
  return BuildInstructionAndAddToModule(current_const->Copy(), pos);
}

ir::Instruction* FoldSpecConstantOpAndCompositePass::DoVectorShuffle(
    ir::Module::inst_iterator* pos) {
  ir::Instruction* inst = &**pos;
  analysis::Vector* result_vec_type = GetType(inst)->AsVector();
  assert(inst->NumInOperands() - 1 > 2 &&
         "OpSpecConstantOp DoVectorShuffle instruction requires more than 2 "
         "operands (2 vector ids and at least one literal operand");
  assert(result_vec_type &&
         "The result of VectorShuffle must be of type vector");

  // A temporary null constants that can be used as the components fo the
  // result vector. This is needed when any one of the vector operands are null
  // constant.
  std::unique_ptr<analysis::Constant> null_component_constants;

  // Get a concatenated vector of scalar constants. The vector should be built
  // with the components from the first and the second operand of VectorShuffle.
  std::vector<const analysis::Constant*> concatenated_components;
  // Note that for OpSpecConstantOp, the second in-operand is the first id
  // operand. The first in-operand is the spec opcode.
  for (uint32_t i : {1, 2}) {
    assert(inst->GetInOperand(i).type == SPV_OPERAND_TYPE_ID &&
           "The vector operand must have a SPV_OPERAND_TYPE_ID type");
    uint32_t operand_id = inst->GetSingleWordInOperand(i);
    analysis::Constant* operand_const = FindRecordedConst(operand_id);
    if (!operand_const) return nullptr;
    const analysis::Type* operand_type = operand_const->type();
    assert(operand_type->AsVector() &&
           "The first two operand of VectorShuffle must be of vector type");
    if (analysis::VectorConstant* vec_const =
            operand_const->AsVectorConstant()) {
      // case 1: current operand is a non-null vector constant.
      concatenated_components.insert(concatenated_components.end(),
                                     vec_const->GetComponents().begin(),
                                     vec_const->GetComponents().end());
    } else if (operand_const->AsNullConstant()) {
      // case 2: current operand is a null vector constant. Create a temporary
      // null scalar constant as the component.
      if (!null_component_constants) {
        const analysis::Type* component_type =
            operand_type->AsVector()->element_type();
        null_component_constants = CreateConst(component_type, {});
      }
      // Append the null scalar consts to the concatenated components
      // vector.
      concatenated_components.insert(concatenated_components.end(),
                                     operand_type->AsVector()->element_count(),
                                     null_component_constants.get());
    } else {
      // no other valid cases
      return nullptr;
    }
  }
  // Create null component constants if there are any. The component constants
  // must be added to the module before the dependee composite constants to
  // satisfy SSA def-use dominance.
  if (null_component_constants) {
    BuildInstructionAndAddToModule(std::move(null_component_constants), pos);
  }
  // Create the new vector constant with the selected components.
  std::vector<const analysis::Constant*> selected_components;
  for (uint32_t i = 3; i < inst->NumInOperands(); i++) {
    assert(inst->GetInOperand(i).type == SPV_OPERAND_TYPE_LITERAL_INTEGER &&
           "The literal operand must of type SPV_OPERAND_TYPE_LITERAL_INTEGER");
    uint32_t literal = inst->GetSingleWordInOperand(i);
    assert(literal < concatenated_components.size() &&
           "Literal index out of bound of the concatenated vector");
    selected_components.push_back(concatenated_components[literal]);
  }
  auto new_vec_const = MakeUnique<analysis::VectorConstant>(
      result_vec_type, selected_components);
  return BuildInstructionAndAddToModule(std::move(new_vec_const), pos);
}

namespace {
// A helper function to check the type for component wise operations. Returns
// true if the type:
//  1) is bool type;
//  2) is 32-bit int type;
//  3) is vector of bool type;
//  4) is vector of 32-bit integer type.
// Otherwise returns false.
bool IsValidTypeForComponentWiseOperation(const analysis::Type* type) {
  if (type->AsBool()) {
    return true;
  } else if (auto* it = type->AsInteger()) {
    if (it->width() == 32) return true;
  } else if (auto* vt = type->AsVector()) {
    if (vt->element_type()->AsBool())
      return true;
    else if (auto* vit = vt->element_type()->AsInteger()) {
      if (vit->width() == 32) return true;
    }
  }
  return false;
}
}

ir::Instruction* FoldSpecConstantOpAndCompositePass::DoComponentWiseOperation(
    ir::Module::inst_iterator* pos) {
  const ir::Instruction* inst = &**pos;
  const analysis::Type* result_type = GetType(inst);
  SpvOp spec_opcode = static_cast<SpvOp>(inst->GetSingleWordInOperand(0));
  // Check and collect operands.
  std::vector<analysis::Constant*> operands;

  if (!std::all_of(inst->cbegin(), inst->cend(),
                   [&operands, this](const ir::Operand& o) {
                     // skip the operands that is not an id.
                     if (o.type != spv_operand_type_t::SPV_OPERAND_TYPE_ID)
                       return true;
                     uint32_t id = o.words.front();
                     if (analysis::Constant* c = FindRecordedConst(id)) {
                       if (IsValidTypeForComponentWiseOperation(c->type())) {
                         operands.push_back(c);
                         return true;
                       }
                     }
                     return false;
                   }))
    return nullptr;

  if (result_type->AsInteger() || result_type->AsBool()) {
    // Scalar operation
    uint32_t result_val = OperateScalars(spec_opcode, operands);
    auto result_const = CreateConst(result_type, {result_val});
    return BuildInstructionAndAddToModule(std::move(result_const), pos);
  } else if (result_type->AsVector()) {
    // Vector operation
    const analysis::Type* element_type =
        result_type->AsVector()->element_type();
    uint32_t num_dims = result_type->AsVector()->element_count();
    std::vector<uint32_t> result_vec =
        OperateVectors(spec_opcode, num_dims, operands);
    std::vector<const analysis::Constant*> result_vector_components;
    for (uint32_t r : result_vec) {
      if (auto rc = CreateConst(element_type, {r})) {
        result_vector_components.push_back(rc.get());
        if (!BuildInstructionAndAddToModule(std::move(rc), pos)) {
          assert(false &&
                 "Failed to build and insert constant declaring instruction "
                 "for the given vector component constant");
        }
      } else {
        assert(false && "Failed to create constants with 32-bit word");
      }
    }
    auto new_vec_const = MakeUnique<analysis::VectorConstant>(
        result_type->AsVector(), result_vector_components);
    return BuildInstructionAndAddToModule(std::move(new_vec_const), pos);
  } else {
    // Cannot process invalid component wise operation. The result of component
    // wise operation must be of integer or bool scalar or vector of
    // integer/bool type.
    return nullptr;
  }
}

ir::Instruction*
FoldSpecConstantOpAndCompositePass::BuildInstructionAndAddToModule(
    std::unique_ptr<analysis::Constant> c, ir::Module::inst_iterator* pos) {
  analysis::Constant* new_const = c.get();
  uint32_t new_id = ++max_id_;
  module_->SetIdBound(new_id + 1);
  const_val_to_id_[new_const] = new_id;
  id_to_const_val_[new_id] = std::move(c);
  auto new_inst = CreateInstruction(new_id, new_const);
  if (!new_inst) return nullptr;
  auto* new_inst_ptr = new_inst.get();
  *pos = pos->InsertBefore(std::move(new_inst));
  (*pos)++;
  def_use_mgr_->AnalyzeInstDefUse(new_inst_ptr);
  return new_inst_ptr;
}

std::unique_ptr<analysis::Constant>
FoldSpecConstantOpAndCompositePass::CreateConstFromInst(ir::Instruction* inst) {
  std::vector<uint32_t> literal_words_or_ids;
  std::unique_ptr<analysis::Constant> new_const;
  // Collect the constant defining literals or component ids.
  for (uint32_t i = 0; i < inst->NumInOperands(); i++) {
    literal_words_or_ids.insert(literal_words_or_ids.end(),
                                inst->GetInOperand(i).words.begin(),
                                inst->GetInOperand(i).words.end());
  }
  switch (inst->opcode()) {
    // OpConstant{True|Flase} have the value embedded in the opcode. So they
    // are not handled by the for-loop above. Here we add the value explicitly.
    case SpvOp::SpvOpConstantTrue:
      literal_words_or_ids.push_back(true);
      break;
    case SpvOp::SpvOpConstantFalse:
      literal_words_or_ids.push_back(false);
      break;
    case SpvOp::SpvOpConstantNull:
    case SpvOp::SpvOpConstant:
    case SpvOp::SpvOpConstantComposite:
    case SpvOp::SpvOpSpecConstantComposite:
      break;
    default:
      return nullptr;
  }
  return CreateConst(GetType(inst), literal_words_or_ids);
}

analysis::Constant* FoldSpecConstantOpAndCompositePass::FindRecordedConst(
    uint32_t id) {
  auto iter = id_to_const_val_.find(id);
  if (iter == id_to_const_val_.end()) {
    return nullptr;
  } else {
    return iter->second.get();
  }
}

uint32_t FoldSpecConstantOpAndCompositePass::FindRecordedConst(
    const analysis::Constant* c) {
  auto iter = const_val_to_id_.find(c);
  if (iter == const_val_to_id_.end()) {
    return 0;
  } else {
    return iter->second;
  }
}

std::vector<const analysis::Constant*>
FoldSpecConstantOpAndCompositePass::GetConstsFromIds(
    const std::vector<uint32_t>& ids) {
  std::vector<const analysis::Constant*> constants;
  for (uint32_t id : ids) {
    if (analysis::Constant* c = FindRecordedConst(id)) {
      constants.push_back(c);
    } else {
      return {};
    }
  }
  return constants;
}

std::unique_ptr<analysis::Constant>
FoldSpecConstantOpAndCompositePass::CreateConst(
    const analysis::Type* type,
    const std::vector<uint32_t>& literal_words_or_ids) {
  std::unique_ptr<analysis::Constant> new_const;
  if (literal_words_or_ids.size() == 0) {
    // Constant declared with OpConstantNull
    return MakeUnique<analysis::NullConstant>(type);
  } else if (auto* bt = type->AsBool()) {
    assert(literal_words_or_ids.size() == 1 &&
           "Bool constant should be declared with one operand");
    return MakeUnique<analysis::BoolConstant>(bt, literal_words_or_ids.front());
  } else if (auto* it = type->AsInteger()) {
    return MakeUnique<analysis::IntConstant>(it, literal_words_or_ids);
  } else if (auto* ft = type->AsFloat()) {
    return MakeUnique<analysis::FloatConstant>(ft, literal_words_or_ids);
  } else if (auto* vt = type->AsVector()) {
    auto components = GetConstsFromIds(literal_words_or_ids);
    if (components.empty()) return nullptr;
    // All components of VectorConstant must be of type Bool, Integer or Float.
    if (!std::all_of(components.begin(), components.end(),
                     [](const analysis::Constant* c) {
                       if (c->type()->AsBool() || c->type()->AsInteger() ||
                           c->type()->AsFloat()) {
                         return true;
                       } else {
                         return false;
                       }
                     }))
      return nullptr;
    // All components of VectorConstant must be in the same type.
    const auto* component_type = components.front()->type();
    if (!std::all_of(components.begin(), components.end(),
                     [&component_type](const analysis::Constant* c) {
                       if (c->type() == component_type) return true;
                       return false;
                     }))
      return nullptr;
    return MakeUnique<analysis::VectorConstant>(vt, components);
  } else if (auto* st = type->AsStruct()) {
    auto components = GetConstsFromIds(literal_words_or_ids);
    if (components.empty()) return nullptr;
    return MakeUnique<analysis::StructConstant>(st, components);
  } else if (auto* at = type->AsArray()) {
    auto components = GetConstsFromIds(literal_words_or_ids);
    if (components.empty()) return nullptr;
    return MakeUnique<analysis::ArrayConstant>(at, components);
  } else {
    return nullptr;
  }
}

std::vector<ir::Operand> BuildOperandsFromIds(
    const std::vector<uint32_t>& ids) {
  std::vector<ir::Operand> operands;
  for (uint32_t id : ids) {
    operands.emplace_back(spv_operand_type_t::SPV_OPERAND_TYPE_ID,
                          std::initializer_list<uint32_t>{id});
  }
  return operands;
}

std::unique_ptr<ir::Instruction>
FoldSpecConstantOpAndCompositePass::CreateInstruction(uint32_t id,
                                                      analysis::Constant* c) {
  if (c->AsNullConstant()) {
    return MakeUnique<ir::Instruction>(SpvOp::SpvOpConstantNull,
                                       type_mgr_->GetId(c->type()), id,
                                       std::initializer_list<ir::Operand>{});
  } else if (analysis::BoolConstant* bc = c->AsBoolConstant()) {
    return MakeUnique<ir::Instruction>(
        bc->value() ? SpvOp::SpvOpConstantTrue : SpvOp::SpvOpConstantFalse,
        type_mgr_->GetId(c->type()), id, std::initializer_list<ir::Operand>{});
  } else if (analysis::IntConstant* ic = c->AsIntConstant()) {
    return MakeUnique<ir::Instruction>(
        SpvOp::SpvOpConstant, type_mgr_->GetId(c->type()), id,
        std::initializer_list<ir::Operand>{ir::Operand(
            spv_operand_type_t::SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER,
            ic->words())});
  } else if (analysis::FloatConstant* fc = c->AsFloatConstant()) {
    return MakeUnique<ir::Instruction>(
        SpvOp::SpvOpConstant, type_mgr_->GetId(c->type()), id,
        std::initializer_list<ir::Operand>{ir::Operand(
            spv_operand_type_t::SPV_OPERAND_TYPE_TYPED_LITERAL_NUMBER,
            fc->words())});
  } else if (analysis::CompositeConstant* cc = c->AsCompositeConstant()) {
    return CreateCompositeInstruction(id, cc);
  } else {
    return nullptr;
  }
}

std::unique_ptr<ir::Instruction>
FoldSpecConstantOpAndCompositePass::CreateCompositeInstruction(
    uint32_t result_id, analysis::CompositeConstant* cc) {
  std::vector<ir::Operand> operands;
  for (const analysis::Constant* component_const : cc->GetComponents()) {
    uint32_t id = FindRecordedConst(component_const);
    if (id == 0) {
      // Cannot get the id of the component constant, while all components
      // should have been added to the module prior to the composite constant.
      // Cannot create OpConstantComposite instruction in this case.
      return nullptr;
    }
    operands.emplace_back(spv_operand_type_t::SPV_OPERAND_TYPE_ID,
                          std::initializer_list<uint32_t>{id});
  }
  return MakeUnique<ir::Instruction>(SpvOp::SpvOpConstantComposite,
                                     type_mgr_->GetId(cc->type()), result_id,
                                     std::move(operands));
}

}  // namespace opt
}  // namespace spvtools
