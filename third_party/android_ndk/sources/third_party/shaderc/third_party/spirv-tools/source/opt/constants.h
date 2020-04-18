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

#ifndef LIBSPIRV_OPT_CONSTANTS_H_
#define LIBSPIRV_OPT_CONSTANTS_H_

#include <memory>
#include <utility>
#include <vector>

#include "make_unique.h"
#include "types.h"

namespace spvtools {
namespace opt {
namespace analysis {

// Class hierarchy to represent the normal constants defined through
// OpConstantTrue, OpConstantFalse, OpConstant, OpConstantNull and
// OpConstantComposite instructions.
// TODO(qining): Add class for constants defined with OpConstantSampler.
class Constant;
class ScalarConstant;
class IntConstant;
class FloatConstant;
class BoolConstant;
class CompositeConstant;
class StructConstant;
class VectorConstant;
class ArrayConstant;
class NullConstant;

// Abstract class for a SPIR-V constant. It has a bunch of As<subclass> methods,
// which is used as a way to probe the actual <subclass>
class Constant {
 public:
  Constant() = delete;
  virtual ~Constant() {}

  // Make a deep copy of this constant.
  virtual std::unique_ptr<Constant> Copy() const = 0;

  // reflections
  virtual ScalarConstant* AsScalarConstant() { return nullptr; }
  virtual IntConstant* AsIntConstant() { return nullptr; }
  virtual FloatConstant* AsFloatConstant() { return nullptr; }
  virtual BoolConstant* AsBoolConstant() { return nullptr; }
  virtual CompositeConstant* AsCompositeConstant() { return nullptr; }
  virtual StructConstant* AsStructConstant() { return nullptr; }
  virtual VectorConstant* AsVectorConstant() { return nullptr; }
  virtual ArrayConstant* AsArrayConstant() { return nullptr; }
  virtual NullConstant* AsNullConstant() { return nullptr; }

  virtual const ScalarConstant* AsScalarConstant() const { return nullptr; }
  virtual const IntConstant* AsIntConstant() const { return nullptr; }
  virtual const FloatConstant* AsFloatConstant() const { return nullptr; }
  virtual const BoolConstant* AsBoolConstant() const { return nullptr; }
  virtual const CompositeConstant* AsCompositeConstant() const {
    return nullptr;
  }
  virtual const StructConstant* AsStructConstant() const { return nullptr; }
  virtual const VectorConstant* AsVectorConstant() const { return nullptr; }
  virtual const ArrayConstant* AsArrayConstant() const { return nullptr; }
  virtual const NullConstant* AsNullConstant() const { return nullptr; }

  const analysis::Type* type() const { return type_; }

 protected:
  Constant(const analysis::Type* ty) : type_(ty) {}

  // The type of this constant.
  const analysis::Type* type_;
};

// Abstract class for scalar type constants.
class ScalarConstant : public Constant {
 public:
  ScalarConstant() = delete;
  ScalarConstant* AsScalarConstant() override { return this; }
  const ScalarConstant* AsScalarConstant() const override { return this; }

  // Returns a const reference of the value of this constant in 32-bit words.
  virtual const std::vector<uint32_t>& words() const { return words_; }

 protected:
  ScalarConstant(const analysis::Type* ty, const std::vector<uint32_t>& w)
      : Constant(ty), words_(w) {}
  ScalarConstant(const analysis::Type* ty, std::vector<uint32_t>&& w)
      : Constant(ty), words_(std::move(w)) {}
  std::vector<uint32_t> words_;
};

// Integer type constant.
class IntConstant : public ScalarConstant {
 public:
  IntConstant(const analysis::Integer* ty, const std::vector<uint32_t>& w)
      : ScalarConstant(ty, w) {}
  IntConstant(const analysis::Integer* ty, std::vector<uint32_t>&& w)
      : ScalarConstant(ty, std::move(w)) {}

  IntConstant* AsIntConstant() override { return this; }
  const IntConstant* AsIntConstant() const override { return this; }

  // Make a copy of this IntConstant instance.
  std::unique_ptr<IntConstant> CopyIntConstant() const {
    return MakeUnique<IntConstant>(type_->AsInteger(), words_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyIntConstant().release());
  }
};

// Float type constant.
class FloatConstant : public ScalarConstant {
 public:
  FloatConstant(const analysis::Float* ty, const std::vector<uint32_t>& w)
      : ScalarConstant(ty, w) {}
  FloatConstant(const analysis::Float* ty, std::vector<uint32_t>&& w)
      : ScalarConstant(ty, std::move(w)) {}

  FloatConstant* AsFloatConstant() override { return this; }
  const FloatConstant* AsFloatConstant() const override { return this; }

  // Make a copy of this FloatConstant instance.
  std::unique_ptr<FloatConstant> CopyFloatConstant() const {
    return MakeUnique<FloatConstant>(type_->AsFloat(), words_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyFloatConstant().release());
  }
};

// Bool type constant.
class BoolConstant : public ScalarConstant {
 public:
  BoolConstant(const analysis::Bool* ty, bool v)
      : ScalarConstant(ty, {static_cast<uint32_t>(v)}), value_(v) {}

  BoolConstant* AsBoolConstant() override { return this; }
  const BoolConstant* AsBoolConstant() const override { return this; }

  // Make a copy of this BoolConstant instance.
  std::unique_ptr<BoolConstant> CopyBoolConstant() const {
    return MakeUnique<BoolConstant>(type_->AsBool(), value_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyBoolConstant().release());
  }

  bool value() const { return value_; }

 private:
  bool value_;
};

// Abstract class for composite constants.
class CompositeConstant : public Constant {
 public:
  CompositeConstant() = delete;
  CompositeConstant* AsCompositeConstant() override { return this; }
  const CompositeConstant* AsCompositeConstant() const override { return this; }

  // Returns a const reference of the components holded in this composite
  // constant.
  virtual const std::vector<const Constant*>& GetComponents() const {
    return components_;
  }

 protected:
  CompositeConstant(const analysis::Type* ty) : Constant(ty), components_() {}
  CompositeConstant(const analysis::Type* ty,
                    const std::vector<const Constant*>& components)
      : Constant(ty), components_(components) {}
  CompositeConstant(const analysis::Type* ty,
                    std::vector<const Constant*>&& components)
      : Constant(ty), components_(std::move(components)) {}
  std::vector<const Constant*> components_;
};

// Struct type constant.
class StructConstant : public CompositeConstant {
 public:
  StructConstant(const analysis::Struct* ty) : CompositeConstant(ty) {}
  StructConstant(const analysis::Struct* ty,
                 const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components) {}
  StructConstant(const analysis::Struct* ty,
                 std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)) {}

  StructConstant* AsStructConstant() override { return this; }
  const StructConstant* AsStructConstant() const override { return this; }

  // Make a copy of this StructConstant instance.
  std::unique_ptr<StructConstant> CopyStructConstant() const {
    return MakeUnique<StructConstant>(type_->AsStruct(), components_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyStructConstant().release());
  }
};

// Vector type constant.
class VectorConstant : public CompositeConstant {
 public:
  VectorConstant(const analysis::Vector* ty)
      : CompositeConstant(ty), component_type_(ty->element_type()) {}
  VectorConstant(const analysis::Vector* ty,
                 const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components),
        component_type_(ty->element_type()) {}
  VectorConstant(const analysis::Vector* ty,
                 std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)),
        component_type_(ty->element_type()) {}

  VectorConstant* AsVectorConstant() override { return this; }
  const VectorConstant* AsVectorConstant() const override { return this; }

  // Make a copy of this VectorConstant instance.
  std::unique_ptr<VectorConstant> CopyVectorConstant() const {
    auto another = MakeUnique<VectorConstant>(type_->AsVector());
    another->components_.insert(another->components_.end(), components_.begin(),
                                components_.end());
    return another;
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyVectorConstant().release());
  }

  const analysis::Type* component_type() { return component_type_; }

 private:
  const analysis::Type* component_type_;
};

// Array type constant.
class ArrayConstant : public CompositeConstant {
 public:
  ArrayConstant(const analysis::Array* ty) : CompositeConstant(ty) {}
  ArrayConstant(const analysis::Array* ty,
                const std::vector<const Constant*>& components)
      : CompositeConstant(ty, components) {}
  ArrayConstant(const analysis::Array* ty,
                std::vector<const Constant*>&& components)
      : CompositeConstant(ty, std::move(components)) {}

  ArrayConstant* AsArrayConstant() override { return this; }
  const ArrayConstant* AsArrayConstant() const override { return this; }

  // Make a copy of this ArrayConstant instance.
  std::unique_ptr<ArrayConstant> CopyArrayConstant() const {
    return MakeUnique<ArrayConstant>(type_->AsArray(), components_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyArrayConstant().release());
  }
};

// Null type constant.
class NullConstant : public Constant {
 public:
  NullConstant(const analysis::Type* ty) : Constant(ty) {}
  NullConstant* AsNullConstant() override { return this; }
  const NullConstant* AsNullConstant() const override { return this; }

  // Make a copy of this NullConstant instance.
  std::unique_ptr<NullConstant> CopyNullConstant() const {
    return MakeUnique<NullConstant>(type_);
  }
  std::unique_ptr<Constant> Copy() const override {
    return std::unique_ptr<Constant>(CopyNullConstant().release());
  }
};

}  // namespace analysis
}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_CONSTANTS_H_
