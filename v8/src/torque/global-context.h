// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_TORQUE_GLOBAL_CONTEXT_H_
#define V8_TORQUE_GLOBAL_CONTEXT_H_

#include <map>

#include "src/torque/declarable.h"
#include "src/torque/declarations.h"
#include "src/torque/type-oracle.h"

namespace v8 {
namespace internal {
namespace torque {

class GlobalContext : public ContextualClass<GlobalContext> {
 public:
  explicit GlobalContext(Ast ast) : verbose_(false), ast_(std::move(ast)) {
    CurrentScope::Scope current_scope(nullptr);
    CurrentSourcePosition::Scope current_source_position(
        SourcePosition{CurrentSourceFile::Get(), -1, -1});
    default_namespace_ =
        RegisterDeclarable(base::make_unique<Namespace>("base"));
  }
  static Namespace* GetDefaultNamespace() { return Get().default_namespace_; }
  template <class T>
  T* RegisterDeclarable(std::unique_ptr<T> d) {
    T* ptr = d.get();
    declarables_.push_back(std::move(d));
    return ptr;
  }

  static const std::vector<std::unique_ptr<Declarable>>& AllDeclarables() {
    return Get().declarables_;
  }

  static const std::vector<Namespace*> GetNamespaces() {
    std::vector<Namespace*> result;
    for (auto& declarable : AllDeclarables()) {
      if (Namespace* n = Namespace::DynamicCast(declarable.get())) {
        result.push_back(n);
      }
    }
    return result;
  }

  static void RegisterClass(const std::string& name,
                            const ClassType* new_class) {
    Get().classes_[name] = new_class;
  }

  static const std::map<std::string, const ClassType*>& GetClasses() {
    return Get().classes_;
  }

  static void AddCppInclude(std::string include_path) {
    Get().cpp_includes_.push_back(std::move(include_path));
  }
  static const std::vector<std::string>& CppIncludes() {
    return Get().cpp_includes_;
  }

  static void SetVerbose() { Get().verbose_ = true; }
  static bool verbose() { return Get().verbose_; }
  static Ast* ast() { return &Get().ast_; }

 private:
  bool verbose_;
  Namespace* default_namespace_;
  Ast ast_;
  std::vector<std::unique_ptr<Declarable>> declarables_;
  std::vector<std::string> cpp_includes_;
  std::map<std::string, const ClassType*> classes_;
};

template <class T>
T* RegisterDeclarable(std::unique_ptr<T> d) {
  return GlobalContext::Get().RegisterDeclarable(std::move(d));
}

}  // namespace torque
}  // namespace internal
}  // namespace v8

#endif  // V8_TORQUE_GLOBAL_CONTEXT_H_
