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

#ifndef LIBSPIRV_OPT_TYPE_MANAGER_H_
#define LIBSPIRV_OPT_TYPE_MANAGER_H_

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "module.h"
#include "spirv-tools/libspirv.hpp"
#include "types.h"

namespace spvtools {
namespace opt {
namespace analysis {

// A class for managing the SPIR-V type hierarchy.
class TypeManager {
 public:
  using IdToTypeMap = std::unordered_map<uint32_t, std::unique_ptr<Type>>;

  // Constructs a type manager from the given |module|. All internal messages
  // will be communicated to the outside via the given message |consumer|.
  // This instance only keeps a reference to the |consumer|, so the |consumer|
  // should outlive this instance.
  TypeManager(const MessageConsumer& consumer,
              const spvtools::ir::Module& module);

  TypeManager(const TypeManager&) = delete;
  TypeManager(TypeManager&&) = delete;
  TypeManager& operator=(const TypeManager&) = delete;
  TypeManager& operator=(TypeManager&&) = delete;

  // Returns the type for the given type |id|. Returns nullptr if the given |id|
  // does not define a type.
  Type* GetType(uint32_t id) const;
  // Returns the id for the given |type|. Returns 0 if can not find the given
  // |type|.
  uint32_t GetId(const Type* type) const;
  // Returns the number of types hold in this manager.
  size_t NumTypes() const { return id_to_type_.size(); }
  // Iterators for all types contained in this manager.
  IdToTypeMap::const_iterator begin() const { return id_to_type_.cbegin(); }
  IdToTypeMap::const_iterator end() const { return id_to_type_.cend(); }

  // Returns the forward pointer type at the given |index|.
  ForwardPointer* GetForwardPointer(uint32_t index) const;
  // Returns the number of forward pointer types hold in this manager.
  size_t NumForwardPointers() const { return forward_pointers_.size(); }

 private:
  using TypeToIdMap = std::unordered_map<const Type*, uint32_t>;
  using ForwardPointerVector = std::vector<std::unique_ptr<ForwardPointer>>;

  // Analyzes the types and decorations on types in the given |module|.
  void AnalyzeTypes(const spvtools::ir::Module& module);

  // Creates and returns a type from the given SPIR-V |inst|. Returns nullptr if
  // the given instruction is not for defining a type.
  Type* RecordIfTypeDefinition(const spvtools::ir::Instruction& inst);
  // Attaches the decoration encoded in |inst| to a type. Does nothing if the
  // given instruction is not a decoration instruction or not decorating a type.
  void AttachIfTypeDecoration(const spvtools::ir::Instruction& inst);

  const MessageConsumer& consumer_;  // Message consumer.
  IdToTypeMap id_to_type_;  // Mapping from ids to their type representations.
  TypeToIdMap type_to_id_;  // Mapping from types to their defining ids.
  ForwardPointerVector forward_pointers_;  // All forward pointer declarations.
  // All unresolved forward pointer declarations.
  // Refers the contents in the above vector.
  std::unordered_set<ForwardPointer*> unresolved_forward_pointers_;
};

inline TypeManager::TypeManager(const spvtools::MessageConsumer& consumer,
                                const spvtools::ir::Module& module)
    : consumer_(consumer) {
  AnalyzeTypes(module);
}

}  // namespace analysis
}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_TYPE_MANAGER_H_
