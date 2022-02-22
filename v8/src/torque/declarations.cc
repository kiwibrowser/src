// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/torque/declarations.h"
#include "src/torque/declarable.h"
#include "src/torque/global-context.h"
#include "src/torque/type-oracle.h"

namespace v8 {
namespace internal {
namespace torque {

DEFINE_CONTEXTUAL_VARIABLE(GlobalContext)

namespace {

template <class T>
std::vector<T> EnsureNonempty(std::vector<T> list, const std::string& name,
                              const char* kind) {
  if (list.empty()) {
    ReportError("there is no ", kind, " named ", name);
  }
  return std::move(list);
}

template <class T, class Name>
T EnsureUnique(const std::vector<T>& list, const Name& name, const char* kind) {
  if (list.empty()) {
    ReportError("there is no ", kind, " named ", name);
  }
  if (list.size() >= 2) {
    ReportError("ambiguous reference to ", kind, " ", name);
  }
  return list.front();
}

template <class T>
void CheckAlreadyDeclared(const std::string& name, const char* new_type) {
  std::vector<T*> declarations =
      FilterDeclarables<T>(Declarations::TryLookupShallow(QualifiedName(name)));
  if (!declarations.empty()) {
    Scope* scope = CurrentScope::Get();
    ReportError("cannot redeclare ", name, " (type ", new_type, scope, ")");
  }
}

}  // namespace

std::vector<Declarable*> Declarations::LookupGlobalScope(
    const std::string& name) {
  std::vector<Declarable*> d =
      GlobalContext::GetDefaultNamespace()->Lookup(QualifiedName(name));
  if (d.empty()) {
    std::stringstream s;
    s << "cannot find \"" << name << "\" in global scope";
    ReportError(s.str());
  }
  return d;
}

const Type* Declarations::LookupType(const QualifiedName& name) {
  TypeAlias* declaration =
      EnsureUnique(FilterDeclarables<TypeAlias>(Lookup(name)), name, "type");
  return declaration->type();
}

const Type* Declarations::LookupType(std::string name) {
  return LookupType(QualifiedName(std::move(name)));
}

const Type* Declarations::LookupGlobalType(const std::string& name) {
  TypeAlias* declaration = EnsureUnique(
      FilterDeclarables<TypeAlias>(LookupGlobalScope(name)), name, "type");
  return declaration->type();
}

const Type* Declarations::GetType(TypeExpression* type_expression) {
  if (auto* basic = BasicTypeExpression::DynamicCast(type_expression)) {
    std::string name =
        (basic->is_constexpr ? CONSTEXPR_TYPE_PREFIX : "") + basic->name;
    return LookupType(QualifiedName{basic->namespace_qualification, name});
  } else if (auto* union_type = UnionTypeExpression::cast(type_expression)) {
    return TypeOracle::GetUnionType(GetType(union_type->a),
                                    GetType(union_type->b));
  } else {
    auto* function_type_exp = FunctionTypeExpression::cast(type_expression);
    TypeVector argument_types;
    for (TypeExpression* type_exp : function_type_exp->parameters) {
      argument_types.push_back(GetType(type_exp));
    }
    return TypeOracle::GetBuiltinPointerType(
        argument_types, GetType(function_type_exp->return_type));
  }
}

Builtin* Declarations::FindSomeInternalBuiltinWithType(
    const BuiltinPointerType* type) {
  for (auto& declarable : GlobalContext::AllDeclarables()) {
    if (Builtin* builtin = Builtin::DynamicCast(declarable.get())) {
      if (!builtin->IsExternal() && builtin->kind() == Builtin::kStub &&
          builtin->signature().return_type == type->return_type() &&
          builtin->signature().parameter_types.types ==
              type->parameter_types()) {
        return builtin;
      }
    }
  }
  return nullptr;
}

Value* Declarations::LookupValue(const QualifiedName& name) {
  return EnsureUnique(FilterDeclarables<Value>(Lookup(name)), name, "value");
}

Macro* Declarations::TryLookupMacro(const std::string& name,
                                    const TypeVector& types) {
  std::vector<Macro*> macros = TryLookup<Macro>(QualifiedName(name));
  for (auto& m : macros) {
    auto signature_types = m->signature().GetExplicitTypes();
    if (signature_types == types && !m->signature().parameter_types.var_args) {
      return m;
    }
  }
  return nullptr;
}

base::Optional<Builtin*> Declarations::TryLookupBuiltin(
    const QualifiedName& name) {
  std::vector<Builtin*> builtins = TryLookup<Builtin>(name);
  if (builtins.empty()) return base::nullopt;
  return EnsureUnique(builtins, name.name, "builtin");
}

std::vector<Generic*> Declarations::LookupGeneric(const std::string& name) {
  return EnsureNonempty(FilterDeclarables<Generic>(Lookup(QualifiedName(name))),
                        name, "generic");
}

Generic* Declarations::LookupUniqueGeneric(const QualifiedName& name) {
  return EnsureUnique(FilterDeclarables<Generic>(Lookup(name)), name,
                      "generic");
}

Namespace* Declarations::DeclareNamespace(const std::string& name) {
  return Declare(name, std::unique_ptr<Namespace>(new Namespace(name)));
}

const AbstractType* Declarations::DeclareAbstractType(
    const std::string& name, bool transient, const std::string& generated,
    base::Optional<const AbstractType*> non_constexpr_version,
    const base::Optional<std::string>& parent) {
  CheckAlreadyDeclared<TypeAlias>(name, "type");
  const Type* parent_type = nullptr;
  if (parent) {
    parent_type = LookupType(QualifiedName{*parent});
  }
  const AbstractType* type = TypeOracle::GetAbstractType(
      parent_type, name, transient, generated, non_constexpr_version);
  DeclareType(name, type, false);
  return type;
}

void Declarations::DeclareType(const std::string& name, const Type* type,
                               bool redeclaration) {
  CheckAlreadyDeclared<TypeAlias>(name, "type");
  Declare(name, std::unique_ptr<TypeAlias>(new TypeAlias(type, redeclaration)));
}

StructType* Declarations::DeclareStruct(const std::string& name,
                                        const std::vector<Field>& fields) {
  StructType* new_type = TypeOracle::GetStructType(name, fields);
  DeclareType(name, new_type, false);
  return new_type;
}

ClassType* Declarations::DeclareClass(const Type* super_type,
                                      const std::string& name, bool transient,
                                      const std::string& generates,
                                      std::vector<Field> fields, size_t size) {
  std::vector<Field> this_struct_fields;
  size_t struct_offset = 0;
  const StructType* super_struct_type = nullptr;
  // In order to ensure "atomicity" of object allocation, a class'
  // constructors operate on a per-class internal struct rather than the class
  // directly until the constructor has successfully completed and all class
  // members are available. Create the appropriate struct type for use in the
  // class' constructors, including a '_super' field in the struct that
  // contains the values constructed by calls to super constructors.
  if (const ClassType* super_class = ClassType::DynamicCast(super_type)) {
    super_struct_type = super_class->struct_type();
    this_struct_fields.push_back(
        {CurrentSourcePosition::Get(),
         {kConstructorStructSuperFieldName, super_struct_type},
         struct_offset,
         false});
    struct_offset += LoweredSlotCount(super_struct_type);
  }
  for (auto& field : fields) {
    const Type* field_type = field.name_and_type.type;
    this_struct_fields.push_back({field.pos,
                                  {field.name_and_type.name, field_type},
                                  struct_offset,
                                  false});
    struct_offset += LoweredSlotCount(field_type);
  }
  StructType* this_struct_type = DeclareStruct(
      kClassConstructorThisStructPrefix + name, this_struct_fields);

  ClassType* new_type =
      TypeOracle::GetClassType(super_type, name, transient, generates,
                               std::move(fields), this_struct_type, size);
  this_struct_type->SetDerivedFrom(new_type);
  DeclareType(name, new_type, false);
  return new_type;
}

Macro* Declarations::CreateMacro(
    std::string external_name, std::string readable_name,
    base::Optional<std::string> external_assembler_name, Signature signature,
    bool transitioning, base::Optional<Statement*> body) {
  if (!external_assembler_name) {
    external_assembler_name = CurrentNamespace()->ExternalName();
  }
  return RegisterDeclarable(std::unique_ptr<Macro>(
      new Macro(std::move(external_name), std::move(readable_name),
                std::move(*external_assembler_name), std::move(signature),
                transitioning, body)));
}

Macro* Declarations::DeclareMacro(
    const std::string& name,
    base::Optional<std::string> external_assembler_name,
    const Signature& signature, bool transitioning,
    base::Optional<Statement*> body, base::Optional<std::string> op) {
  if (TryLookupMacro(name, signature.GetExplicitTypes())) {
    ReportError("cannot redeclare macro ", name,
                " with identical explicit parameters");
  }
  Macro* macro = CreateMacro(name, name, std::move(external_assembler_name),
                             signature, transitioning, body);
  Declare(name, macro);
  if (op) {
    if (TryLookupMacro(*op, signature.GetExplicitTypes())) {
      ReportError("cannot redeclare operator ", name,
                  " with identical explicit parameters");
    }
    DeclareOperator(*op, macro);
  }
  return macro;
}

Method* Declarations::CreateMethod(AggregateType* container_type,
                                   const std::string& name, Signature signature,
                                   bool transitioning, Statement* body) {
  std::string generated_name{container_type->GetGeneratedMethodName(name)};
  Method* result = RegisterDeclarable(std::unique_ptr<Method>(
      new Method(container_type, container_type->GetGeneratedMethodName(name),
                 name, CurrentNamespace()->ExternalName(), std::move(signature),
                 transitioning, body)));
  container_type->RegisterMethod(result);
  return result;
}

Intrinsic* Declarations::CreateIntrinsic(const std::string& name,
                                         const Signature& signature) {
  Intrinsic* result = RegisterDeclarable(std::unique_ptr<Intrinsic>(
      new Intrinsic(std::move(name), std::move(signature))));
  return result;
}

Intrinsic* Declarations::DeclareIntrinsic(const std::string& name,
                                          const Signature& signature) {
  Intrinsic* result = CreateIntrinsic(std::move(name), std::move(signature));
  Declare(name, result);
  return result;
}

Builtin* Declarations::CreateBuiltin(std::string external_name,
                                     std::string readable_name,
                                     Builtin::Kind kind, Signature signature,
                                     bool transitioning,
                                     base::Optional<Statement*> body) {
  return RegisterDeclarable(std::unique_ptr<Builtin>(
      new Builtin(std::move(external_name), std::move(readable_name), kind,
                  std::move(signature), transitioning, body)));
}

Builtin* Declarations::DeclareBuiltin(const std::string& name,
                                      Builtin::Kind kind,
                                      const Signature& signature,
                                      bool transitioning,
                                      base::Optional<Statement*> body) {
  CheckAlreadyDeclared<Builtin>(name, "builtin");
  return Declare(
      name, CreateBuiltin(name, name, kind, signature, transitioning, body));
}

RuntimeFunction* Declarations::DeclareRuntimeFunction(
    const std::string& name, const Signature& signature, bool transitioning) {
  CheckAlreadyDeclared<RuntimeFunction>(name, "runtime function");
  return Declare(name,
                 RegisterDeclarable(std::unique_ptr<RuntimeFunction>(
                     new RuntimeFunction(name, signature, transitioning))));
}

void Declarations::DeclareExternConstant(const std::string& name,
                                         const Type* type, std::string value) {
  CheckAlreadyDeclared<Value>(name, "constant");
  ExternConstant* result = new ExternConstant(name, type, value);
  Declare(name, std::unique_ptr<Declarable>(result));
}

NamespaceConstant* Declarations::DeclareNamespaceConstant(
    const std::string& name, const Type* type, Expression* body) {
  CheckAlreadyDeclared<Value>(name, "constant");
  NamespaceConstant* result = new NamespaceConstant(name, type, body);
  Declare(name, std::unique_ptr<Declarable>(result));
  return result;
}

Generic* Declarations::DeclareGeneric(const std::string& name,
                                      GenericDeclaration* generic) {
  return Declare(name, std::unique_ptr<Generic>(new Generic(name, generic)));
}

std::string Declarations::GetGeneratedCallableName(
    const std::string& name, const TypeVector& specialized_types) {
  std::string result = name;
  for (auto type : specialized_types) {
    std::string type_string = type->MangledName();
    result += std::to_string(type_string.size()) + type_string;
  }
  return result;
}

Macro* Declarations::DeclareOperator(const std::string& name, Macro* m) {
  GlobalContext::GetDefaultNamespace()->AddDeclarable(name, m);
  return m;
}

}  // namespace torque
}  // namespace internal
}  // namespace v8
