// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_POSSIBLY_ASSOCIATED_INTERFACE_PTR_H_
#define CONTENT_COMMON_POSSIBLY_ASSOCIATED_INTERFACE_PTR_H_

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "content/common/possibly_associated_interface_ptr_info.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"

namespace content {

// PossiblyAssociatedInterfacePtr<T> contains mojo::InterfacePtr<T> or
// mojo::AssociatedInterfacePtr<T>, but not both. Mojo-related functions in
// mojo::InterfacePtr<T> and mojo::AssociatedInterfacePtr<T> are not accessible,
// but a user can access the raw pointer to the interface.
template <typename T>
class PossiblyAssociatedInterfacePtr final {
 public:
  using InterfaceType = T;
  using PtrInfoType = PossiblyAssociatedInterfacePtrInfo<T>;

  PossiblyAssociatedInterfacePtr() {}
  PossiblyAssociatedInterfacePtr(std::nullptr_t) {}
  PossiblyAssociatedInterfacePtr(mojo::InterfacePtr<T> independent_ptr)
      : independent_ptr_(std::move(independent_ptr)) {}
  PossiblyAssociatedInterfacePtr(mojo::AssociatedInterfacePtr<T> associated_ptr)
      : associated_ptr_(std::move(associated_ptr)) {}

  PossiblyAssociatedInterfacePtr(PossiblyAssociatedInterfacePtr&& other) =
      default;
  ~PossiblyAssociatedInterfacePtr() {}

  PossiblyAssociatedInterfacePtr& operator=(
      PossiblyAssociatedInterfacePtr&& other) = default;

  void Bind(PossiblyAssociatedInterfacePtrInfo<T> info,
            scoped_refptr<base::SingleThreadTaskRunner> runner = nullptr) {
    independent_ptr_.reset();
    associated_ptr_.reset();

    // A PossiblyAssociatedInterfacePtrInfo is guaranteed to only have either a
    // valid InterfacePtrInfo or valid AssociatedInterfacePtrInfo, never both.
    auto independent_ptr_info = info.TakeIndependentPtrInfo();
    if (independent_ptr_info.is_valid())
      independent_ptr_.Bind(std::move(independent_ptr_info), std::move(runner));

    auto associated_ptr_info = info.TakeAssociatedPtrInfo();
    if (associated_ptr_info.is_valid())
      associated_ptr_.Bind(std::move(associated_ptr_info), std::move(runner));
  }

  PossiblyAssociatedInterfacePtrInfo<T> PassInterface() {
    if (independent_ptr_)
      return independent_ptr_.PassInterface();
    if (associated_ptr_)
      return associated_ptr_.PassInterface();
    return nullptr;
  }

  T* get() const {
    return independent_ptr_ ? independent_ptr_.get() : associated_ptr_.get();
  }
  T* operator->() const { return get(); }
  T& operator*() const { return *get(); }
  explicit operator bool() const { return get(); }

 private:
  mojo::InterfacePtr<T> independent_ptr_;
  mojo::AssociatedInterfacePtr<T> associated_ptr_;

  DISALLOW_COPY_AND_ASSIGN(PossiblyAssociatedInterfacePtr);
};

}  // namespace content

#endif  // CONTENT_COMMON_POSSIBLY_ASSOCIATED_INTERFACE_PTR_H_
