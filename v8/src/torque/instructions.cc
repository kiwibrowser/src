// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/torque/instructions.h"
#include "src/torque/cfg.h"
#include "src/torque/type-oracle.h"

namespace v8 {
namespace internal {
namespace torque {

#define TORQUE_INSTRUCTION_BOILERPLATE_DEFINITIONS(Name)        \
  const InstructionKind Name::kKind = InstructionKind::k##Name; \
  std::unique_ptr<InstructionBase> Name::Clone() const {        \
    return std::unique_ptr<InstructionBase>(new Name(*this));   \
  }                                                             \
  void Name::Assign(const InstructionBase& other) {             \
    *this = static_cast<const Name&>(other);                    \
  }
TORQUE_INSTRUCTION_LIST(TORQUE_INSTRUCTION_BOILERPLATE_DEFINITIONS)
#undef TORQUE_INSTRUCTION_BOILERPLATE_DEFINITIONS

void PeekInstruction::TypeInstruction(Stack<const Type*>* stack,
                                      ControlFlowGraph* cfg) const {
  const Type* type = stack->Peek(slot);
  if (widened_type) {
    if (type->IsTopType()) {
      const TopType* top_type = TopType::cast(type);
      ReportError("use of " + top_type->reason());
    }
    if (!type->IsSubtypeOf(*widened_type)) {
      ReportError("type ", *type, " is not a subtype of ", **widened_type);
    }
    type = *widened_type;
  }
  stack->Push(type);
}

void PokeInstruction::TypeInstruction(Stack<const Type*>* stack,
                                      ControlFlowGraph* cfg) const {
  const Type* type = stack->Top();
  if (widened_type) {
    if (!type->IsSubtypeOf(*widened_type)) {
      ReportError("type ", type, " is not a subtype of ", *widened_type);
    }
    type = *widened_type;
  }
  stack->Poke(slot, type);
  stack->Pop();
}

void DeleteRangeInstruction::TypeInstruction(Stack<const Type*>* stack,
                                             ControlFlowGraph* cfg) const {
  stack->DeleteRange(range);
}

void PushUninitializedInstruction::TypeInstruction(
    Stack<const Type*>* stack, ControlFlowGraph* cfg) const {
  stack->Push(type);
}

void PushBuiltinPointerInstruction::TypeInstruction(
    Stack<const Type*>* stack, ControlFlowGraph* cfg) const {
  stack->Push(type);
}

void NamespaceConstantInstruction::TypeInstruction(
    Stack<const Type*>* stack, ControlFlowGraph* cfg) const {
  stack->PushMany(LowerType(constant->type()));
}

void InstructionBase::InvalidateTransientTypes(
    Stack<const Type*>* stack) const {
  auto current = stack->begin();
  while (current != stack->end()) {
    if ((*current)->IsTransient()) {
      std::stringstream stream;
      stream << "type " << **current
             << " is made invalid by transitioning callable invocation at "
             << PositionAsString(pos);
      *current = TypeOracle::GetTopType(stream.str(), *current);
    }
    ++current;
  }
}

void CallIntrinsicInstruction::TypeInstruction(Stack<const Type*>* stack,
                                               ControlFlowGraph* cfg) const {
  std::vector<const Type*> parameter_types =
      LowerParameterTypes(intrinsic->signature().parameter_types);
  for (intptr_t i = parameter_types.size() - 1; i >= 0; --i) {
    const Type* arg_type = stack->Pop();
    const Type* parameter_type = parameter_types.back();
    parameter_types.pop_back();
    if (arg_type != parameter_type) {
      ReportError("parameter ", i, ": expected type ", *parameter_type,
                  " but found type ", *arg_type);
    }
  }
  if (intrinsic->IsTransitioning()) {
    InvalidateTransientTypes(stack);
  }
  stack->PushMany(LowerType(intrinsic->signature().return_type));
}

void CallCsaMacroInstruction::TypeInstruction(Stack<const Type*>* stack,
                                              ControlFlowGraph* cfg) const {
  std::vector<const Type*> parameter_types =
      LowerParameterTypes(macro->signature().parameter_types);
  for (intptr_t i = parameter_types.size() - 1; i >= 0; --i) {
    const Type* arg_type = stack->Pop();
    const Type* parameter_type = parameter_types.back();
    parameter_types.pop_back();
    if (arg_type != parameter_type) {
      ReportError("parameter ", i, ": expected type ", *parameter_type,
                  " but found type ", *arg_type);
    }
  }

  if (macro->IsTransitioning()) {
    InvalidateTransientTypes(stack);
  }

  if (catch_block) {
    Stack<const Type*> catch_stack = *stack;
    catch_stack.Push(TypeOracle::GetObjectType());
    (*catch_block)->SetInputTypes(catch_stack);
  }

  stack->PushMany(LowerType(macro->signature().return_type));
}

void CallCsaMacroAndBranchInstruction::TypeInstruction(
    Stack<const Type*>* stack, ControlFlowGraph* cfg) const {
  std::vector<const Type*> parameter_types =
      LowerParameterTypes(macro->signature().parameter_types);
  for (intptr_t i = parameter_types.size() - 1; i >= 0; --i) {
    const Type* arg_type = stack->Pop();
    const Type* parameter_type = parameter_types.back();
    parameter_types.pop_back();
    if (arg_type != parameter_type) {
      ReportError("parameter ", i, ": expected type ", *parameter_type,
                  " but found type ", *arg_type);
    }
  }

  if (label_blocks.size() != macro->signature().labels.size()) {
    ReportError("wrong number of labels");
  }
  for (size_t i = 0; i < label_blocks.size(); ++i) {
    Stack<const Type*> continuation_stack = *stack;
    continuation_stack.PushMany(
        LowerParameterTypes(macro->signature().labels[i].types));
    label_blocks[i]->SetInputTypes(std::move(continuation_stack));
  }

  if (macro->IsTransitioning()) {
    InvalidateTransientTypes(stack);
  }

  if (catch_block) {
    Stack<const Type*> catch_stack = *stack;
    catch_stack.Push(TypeOracle::GetObjectType());
    (*catch_block)->SetInputTypes(catch_stack);
  }

  if (macro->signature().return_type != TypeOracle::GetNeverType()) {
    Stack<const Type*> return_stack = *stack;
    return_stack.PushMany(LowerType(macro->signature().return_type));
    if (return_continuation == base::nullopt) {
      ReportError("missing return continuation.");
    }
    (*return_continuation)->SetInputTypes(return_stack);
  } else {
    if (return_continuation != base::nullopt) {
      ReportError("unreachable return continuation.");
    }
  }
}

void CallBuiltinInstruction::TypeInstruction(Stack<const Type*>* stack,
                                             ControlFlowGraph* cfg) const {
  std::vector<const Type*> argument_types = stack->PopMany(argc);
  if (argument_types !=
      LowerParameterTypes(builtin->signature().parameter_types)) {
    ReportError("wrong argument types");
  }
  if (builtin->IsTransitioning()) {
    InvalidateTransientTypes(stack);
  }

  if (catch_block) {
    Stack<const Type*> catch_stack = *stack;
    catch_stack.Push(TypeOracle::GetObjectType());
    (*catch_block)->SetInputTypes(catch_stack);
  }

  stack->PushMany(LowerType(builtin->signature().return_type));
}

void CallBuiltinPointerInstruction::TypeInstruction(
    Stack<const Type*>* stack, ControlFlowGraph* cfg) const {
  std::vector<const Type*> argument_types = stack->PopMany(argc);
  const BuiltinPointerType* f = BuiltinPointerType::DynamicCast(stack->Pop());
  if (!f) ReportError("expected function pointer type");
  if (argument_types != LowerParameterTypes(f->parameter_types())) {
    ReportError("wrong argument types");
  }
  // TODO(tebbi): Only invalidate transient types if the function pointer type
  // is transitioning.
  InvalidateTransientTypes(stack);
  stack->PushMany(LowerType(f->return_type()));
}

void CallRuntimeInstruction::TypeInstruction(Stack<const Type*>* stack,
                                             ControlFlowGraph* cfg) const {
  std::vector<const Type*> argument_types = stack->PopMany(argc);
  if (argument_types !=
      LowerParameterTypes(runtime_function->signature().parameter_types,
                          argc)) {
    ReportError("wrong argument types");
  }
  if (runtime_function->IsTransitioning()) {
    InvalidateTransientTypes(stack);
  }

  if (catch_block) {
    Stack<const Type*> catch_stack = *stack;
    catch_stack.Push(TypeOracle::GetObjectType());
    (*catch_block)->SetInputTypes(catch_stack);
  }

  const Type* return_type = runtime_function->signature().return_type;
  if (return_type != TypeOracle::GetNeverType()) {
    stack->PushMany(LowerType(return_type));
  }
}

void BranchInstruction::TypeInstruction(Stack<const Type*>* stack,
                                        ControlFlowGraph* cfg) const {
  const Type* condition_type = stack->Pop();
  if (condition_type != TypeOracle::GetBoolType()) {
    ReportError("condition has to have type bool");
  }
  if_true->SetInputTypes(*stack);
  if_false->SetInputTypes(*stack);
}

void ConstexprBranchInstruction::TypeInstruction(Stack<const Type*>* stack,
                                                 ControlFlowGraph* cfg) const {
  if_true->SetInputTypes(*stack);
  if_false->SetInputTypes(*stack);
}

void GotoInstruction::TypeInstruction(Stack<const Type*>* stack,
                                      ControlFlowGraph* cfg) const {
  destination->SetInputTypes(*stack);
}

void GotoExternalInstruction::TypeInstruction(Stack<const Type*>* stack,
                                              ControlFlowGraph* cfg) const {
  if (variable_names.size() != stack->Size()) {
    ReportError("goto external label with wrong parameter count.");
  }
}

void ReturnInstruction::TypeInstruction(Stack<const Type*>* stack,
                                        ControlFlowGraph* cfg) const {
  cfg->SetReturnType(stack->Pop());
}

void PrintConstantStringInstruction::TypeInstruction(
    Stack<const Type*>* stack, ControlFlowGraph* cfg) const {}

void AbortInstruction::TypeInstruction(Stack<const Type*>* stack,
                                       ControlFlowGraph* cfg) const {}

void UnsafeCastInstruction::TypeInstruction(Stack<const Type*>* stack,
                                            ControlFlowGraph* cfg) const {
  stack->Poke(stack->AboveTop() - 1, destination_type);
}

void LoadObjectFieldInstruction::TypeInstruction(Stack<const Type*>* stack,
                                                 ControlFlowGraph* cfg) const {
  const ClassType* stack_class_type = ClassType::DynamicCast(stack->Top());
  if (!stack_class_type) {
    ReportError(
        "first argument to a LoadObjectFieldInstruction instruction isn't a "
        "class");
  }
  if (stack_class_type != class_type) {
    ReportError(
        "first argument to a LoadObjectFieldInstruction doesn't match "
        "instruction's type");
  }
  const Field& field = class_type->LookupField(field_name);
  stack->Poke(stack->AboveTop() - 1, field.name_and_type.type);
}

void StoreObjectFieldInstruction::TypeInstruction(Stack<const Type*>* stack,
                                                  ControlFlowGraph* cfg) const {
  auto value = stack->Pop();
  const ClassType* stack_class_type = ClassType::DynamicCast(stack->Top());
  if (!stack_class_type) {
    ReportError(
        "first argument to a StoreObjectFieldInstruction instruction isn't a "
        "class");
  }
  if (stack_class_type != class_type) {
    ReportError(
        "first argument to a StoreObjectFieldInstruction doesn't match "
        "instruction's type");
  }
  stack->Pop();
  stack->Push(value);
}

bool CallRuntimeInstruction::IsBlockTerminator() const {
  return is_tailcall || runtime_function->signature().return_type ==
                            TypeOracle::GetNeverType();
}

}  // namespace torque
}  // namespace internal
}  // namespace v8
