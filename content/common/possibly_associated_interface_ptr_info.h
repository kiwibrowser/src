// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_POSSIBLY_ASSOCIATED_INTERFACE_PTR_INFO_H_
#define CONTENT_COMMON_POSSIBLY_ASSOCIATED_INTERFACE_PTR_INFO_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr_info.h"
#include "mojo/public/cpp/bindings/interface_ptr_info.h"

namespace content {

// PossiblyAssociatedInterfacePtrInfo<T> contains mojo::InterfacePtrInfo<T> or
// mojo::AssociatedInterfacePtrInfo<T>, but not both. Mojo-related functions in
// mojo::InterfacePtrInfo<T> and mojo::AssociatedInterfacePtrInfo<T> are not
// accessible, but a user can Bind this object into a
// PossiblyAssociatedInterfacePtr.
template <typename T>
class PossiblyAssociatedInterfacePtrInfo final {
 public:
  PossiblyAssociatedInterfacePtrInfo() {}
  PossiblyAssociatedInterfacePtrInfo(std::nullptr_t) {}
  PossiblyAssociatedInterfacePtrInfo(
      mojo::InterfacePtrInfo<T> independent_ptr_info)
      : independent_ptr_info_(std::move(independent_ptr_info)) {}
  PossiblyAssociatedInterfacePtrInfo(
      mojo::AssociatedInterfacePtrInfo<T> associated_ptr_info)
      : associated_ptr_info_(std::move(associated_ptr_info)) {}

  PossiblyAssociatedInterfacePtrInfo(
      PossiblyAssociatedInterfacePtrInfo&& other) = default;
  ~PossiblyAssociatedInterfacePtrInfo() {}

  PossiblyAssociatedInterfacePtrInfo& operator=(
      PossiblyAssociatedInterfacePtrInfo&& other) = default;

  mojo::InterfacePtrInfo<T> TakeIndependentPtrInfo() {
    return std::move(independent_ptr_info_);
  }

  mojo::AssociatedInterfacePtrInfo<T> TakeAssociatedPtrInfo() {
    return std::move(associated_ptr_info_);
  }

  explicit operator bool() const {
    return independent_ptr_info_.is_valid() || associated_ptr_info_.is_valid();
  }

 private:
  mojo::InterfacePtrInfo<T> independent_ptr_info_;
  mojo::AssociatedInterfacePtrInfo<T> associated_ptr_info_;

  DISALLOW_COPY_AND_ASSIGN(PossiblyAssociatedInterfacePtrInfo);
};

}  // namespace content

#endif  // CONTENT_COMMON_POSSIBLY_ASSOCIATED_INTERFACE_PTR_INFO_H_
