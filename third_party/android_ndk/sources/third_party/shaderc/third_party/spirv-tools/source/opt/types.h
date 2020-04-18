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

// This file provides a class hierarchy for representing SPIR-V types.

#ifndef LIBSPIRV_OPT_TYPES_H_
#define LIBSPIRV_OPT_TYPES_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "spirv-tools/libspirv.h"
#include "spirv/1.2/spirv.h"

namespace spvtools {
namespace opt {
namespace analysis {

class Void;
class Bool;
class Integer;
class Float;
class Vector;
class Matrix;
class Image;
class Sampler;
class SampledImage;
class Array;
class RuntimeArray;
class Struct;
class Opaque;
class Pointer;
class Function;
class Event;
class DeviceEvent;
class ReserveId;
class Queue;
class Pipe;
class ForwardPointer;
class PipeStorage;
class NamedBarrier;

// Abstract class for a SPIR-V type. It has a bunch of As<sublcass>() methods,
// which is used as a way to probe the actual <subclass>.
class Type {
 public:
  virtual ~Type() {}

  // Attaches a decoration directly on this type.
  void AddDecoration(std::vector<uint32_t>&& d) {
    decorations_.push_back(std::move(d));
  }
  // Returns the decorations on this type as a string.
  std::string GetDecorationStr() const;
  // Returns true if this type has exactly the same decorations as |that| type.
  bool HasSameDecorations(const Type* that) const;
  // Returns true if this type is exactly the same as |that| type, including
  // decorations.
  virtual bool IsSame(Type* that) const = 0;
  // Returns a human-readable string to represent this type.
  virtual std::string str() const = 0;

  // Returns true if there is no decoration on this type. For struct types,
  // returns true only when there is no decoration for both the struct type
  // and the struct members.
  virtual bool decoration_empty() const { return decorations_.empty(); }

// A bunch of methods for casting this type to a given type. Returns this if the
// cast can be done, nullptr otherwise.
#define DeclareCastMethod(target)                  \
  virtual target* As##target() { return nullptr; } \
  virtual const target* As##target() const { return nullptr; }
  DeclareCastMethod(Void);
  DeclareCastMethod(Bool);
  DeclareCastMethod(Integer);
  DeclareCastMethod(Float);
  DeclareCastMethod(Vector);
  DeclareCastMethod(Matrix);
  DeclareCastMethod(Image);
  DeclareCastMethod(Sampler);
  DeclareCastMethod(SampledImage);
  DeclareCastMethod(Array);
  DeclareCastMethod(RuntimeArray);
  DeclareCastMethod(Struct);
  DeclareCastMethod(Opaque);
  DeclareCastMethod(Pointer);
  DeclareCastMethod(Function);
  DeclareCastMethod(Event);
  DeclareCastMethod(DeviceEvent);
  DeclareCastMethod(ReserveId);
  DeclareCastMethod(Queue);
  DeclareCastMethod(Pipe);
  DeclareCastMethod(ForwardPointer);
  DeclareCastMethod(PipeStorage);
  DeclareCastMethod(NamedBarrier);
#undef DeclareCastMethod

 protected:
  // Decorations attached to this type. Each decoration is encoded as a vector
  // of uint32_t numbers. The first uint32_t number is the decoration value,
  // and the rest are the parameters to the decoration (if exists).
  std::vector<std::vector<uint32_t>> decorations_;
};

class Integer : public Type {
 public:
  Integer(uint32_t w, bool is_signed) : width_(w), signed_(is_signed) {}
  Integer(const Integer&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;

  Integer* AsInteger() override { return this; }
  const Integer* AsInteger() const override { return this; }
  uint32_t width() const { return width_; }
  bool IsSigned() const { return signed_; }

 private:
  uint32_t width_;  // bit width
  bool signed_;     // true if this integer is signed
};

class Float : public Type {
 public:
  Float(uint32_t w) : width_(w) {}
  Float(const Float&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;

  Float* AsFloat() override { return this; }
  const Float* AsFloat() const override { return this; }
  uint32_t width() const { return width_; }

 private:
  uint32_t width_;  // bit width
};

class Vector : public Type {
 public:
  Vector(Type* element_type, uint32_t count);
  Vector(const Vector&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;
  const Type* element_type() const { return element_type_; }
  uint32_t element_count() const { return count_; }

  Vector* AsVector() override { return this; }
  const Vector* AsVector() const override { return this; }

 private:
  Type* element_type_;
  uint32_t count_;
};

class Matrix : public Type {
 public:
  Matrix(Type* element_type, uint32_t count);
  Matrix(const Matrix&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;
  const Type* element_type() const { return element_type_; }
  uint32_t element_count() const { return count_; }

  Matrix* AsMatrix() override { return this; }
  const Matrix* AsMatrix() const override { return this; }

 private:
  Type* element_type_;
  uint32_t count_;
};

class Image : public Type {
 public:
  Image(Type* sampled_type, SpvDim dim, uint32_t depth, uint32_t arrayed,
        uint32_t ms, uint32_t sampled, SpvImageFormat format,
        SpvAccessQualifier access_qualifier = SpvAccessQualifierReadOnly);
  Image(const Image&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;

  Image* AsImage() override { return this; }
  const Image* AsImage() const override { return this; }

 private:
  Type* sampled_type_;
  SpvDim dim_;
  uint32_t depth_;
  uint32_t arrayed_;
  uint32_t ms_;
  uint32_t sampled_;
  SpvImageFormat format_;
  SpvAccessQualifier access_qualifier_;
};

class SampledImage : public Type {
 public:
  SampledImage(Type* image_type) : image_type_(image_type) {}
  SampledImage(const SampledImage&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;

  SampledImage* AsSampledImage() override { return this; }
  const SampledImage* AsSampledImage() const override { return this; }

 private:
  Type* image_type_;
};

class Array : public Type {
 public:
  Array(Type* element_type, uint32_t length_id);
  Array(const Array&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;
  const Type* element_type() const { return element_type_; }
  uint32_t LengthId() const { return length_id_; }

  Array* AsArray() override { return this; }
  const Array* AsArray() const override { return this; }

 private:
  Type* element_type_;
  uint32_t length_id_;
};

class RuntimeArray : public Type {
 public:
  RuntimeArray(Type* element_type);
  RuntimeArray(const RuntimeArray&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;
  const Type* element_type() const { return element_type_; }

  RuntimeArray* AsRuntimeArray() override { return this; }
  const RuntimeArray* AsRuntimeArray() const override { return this; }

 private:
  Type* element_type_;
};

class Struct : public Type {
 public:
  Struct(const std::vector<Type*>& element_types);
  Struct(const Struct&) = default;

  // Adds a decoration to the member at the given index.  The first word is the
  // decoration enum, and the remaining words, if any, are its operands.
  void AddMemberDecoration(uint32_t index, std::vector<uint32_t>&& decoration);

  bool IsSame(Type* that) const override;
  std::string str() const override;
  const std::vector<Type*>& element_types() const { return element_types_; }
  bool decoration_empty() const override {
    return decorations_.empty() && element_decorations_.empty();
  }

  Struct* AsStruct() override { return this; }
  const Struct* AsStruct() const override { return this; }

 private:
  std::vector<Type*> element_types_;
  // We can attach decorations to struct members and that should not affect the
  // underlying element type. So we need an extra data structure here to keep
  // track of element type decorations.
  std::unordered_map<uint32_t, std::vector<std::vector<uint32_t>>>
      element_decorations_;
};

class Opaque : public Type {
 public:
  Opaque(std::string name) : name_(std::move(name)) {}
  Opaque(const Opaque&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;

  Opaque* AsOpaque() override { return this; }
  const Opaque* AsOpaque() const override { return this; }

 private:
  std::string name_;
};

class Pointer : public Type {
 public:
  Pointer(Type* pointee_type, SpvStorageClass storage_class);
  Pointer(const Pointer&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;
  const Type* pointee_type() const { return pointee_type_; }

  Pointer* AsPointer() override { return this; }
  const Pointer* AsPointer() const override { return this; }

 private:
  Type* pointee_type_;
  SpvStorageClass storage_class_;
};

class Function : public Type {
 public:
  Function(Type* return_type, const std::vector<Type*>& param_types);
  Function(const Function&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;

  Function* AsFunction() override { return this; }
  const Function* AsFunction() const override { return this; }

 private:
  Type* return_type_;
  std::vector<Type*> param_types_;
};

class Pipe : public Type {
 public:
  Pipe(SpvAccessQualifier access_qualifier)
      : access_qualifier_(access_qualifier) {}
  Pipe(const Pipe&) = default;

  bool IsSame(Type* that) const override;
  std::string str() const override;

  Pipe* AsPipe() override { return this; }
  const Pipe* AsPipe() const override { return this; }

 private:
  SpvAccessQualifier access_qualifier_;
};

class ForwardPointer : public Type {
 public:
  ForwardPointer(uint32_t id, SpvStorageClass storage_class)
      : target_id_(id), storage_class_(storage_class), pointer_(nullptr) {}
  ForwardPointer(const ForwardPointer&) = default;

  uint32_t target_id() const { return target_id_; }
  void SetTargetPointer(Pointer* pointer) { pointer_ = pointer; }

  bool IsSame(Type* that) const override;
  std::string str() const override;

  ForwardPointer* AsForwardPointer() override { return this; }
  const ForwardPointer* AsForwardPointer() const override { return this; }

 private:
  uint32_t target_id_;
  SpvStorageClass storage_class_;
  Pointer* pointer_;
};

#define DefineParameterlessType(type, name)                \
  class type : public Type {                               \
   public:                                                 \
    type() = default;                                      \
    type(const type&) = default;                           \
                                                           \
    bool IsSame(Type* that) const override {               \
      return that->As##type() && HasSameDecorations(that); \
    }                                                      \
    std::string str() const override { return #name; }     \
                                                           \
    type* As##type() override { return this; }             \
    const type* As##type() const override { return this; } \
  };
DefineParameterlessType(Void, void);
DefineParameterlessType(Bool, bool);
DefineParameterlessType(Sampler, sampler);
DefineParameterlessType(Event, event);
DefineParameterlessType(DeviceEvent, device_event);
DefineParameterlessType(ReserveId, reserve_id);
DefineParameterlessType(Queue, queue);
DefineParameterlessType(PipeStorage, pipe_storage);
DefineParameterlessType(NamedBarrier, named_barrier);
#undef DefineParameterlessType

}  // namespace analysis
}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_TYPES_H_
