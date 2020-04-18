// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_PTR_INFO_H_
#define MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_PTR_INFO_H_

#include <cstddef>
#include <cstdint>
#include <utility>

#include "base/macros.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace mojo {

// InterfacePtrInfo stores necessary information to communicate with a remote
// interface implementation, which could be used to construct an InterfacePtr.
template <typename Interface>
class InterfacePtrInfo {
 public:
  InterfacePtrInfo() : version_(0u) {}
  InterfacePtrInfo(std::nullptr_t) : InterfacePtrInfo() {}

  InterfacePtrInfo(ScopedMessagePipeHandle handle, uint32_t version)
      : handle_(std::move(handle)), version_(version) {}

  InterfacePtrInfo(InterfacePtrInfo&& other)
      : handle_(std::move(other.handle_)), version_(other.version_) {
    other.version_ = 0u;
  }

  ~InterfacePtrInfo() {}

  InterfacePtrInfo& operator=(InterfacePtrInfo&& other) {
    if (this != &other) {
      handle_ = std::move(other.handle_);
      version_ = other.version_;
      other.version_ = 0u;
    }

    return *this;
  }

  bool is_valid() const { return handle_.is_valid(); }

  ScopedMessagePipeHandle PassHandle() { return std::move(handle_); }
  const ScopedMessagePipeHandle& handle() const { return handle_; }
  void set_handle(ScopedMessagePipeHandle handle) {
    handle_ = std::move(handle);
  }

  uint32_t version() const { return version_; }
  void set_version(uint32_t version) { version_ = version; }

  // Allow InterfacePtrInfo<> to be used in boolean expressions.
  explicit operator bool() const { return handle_.is_valid(); }

 private:
  ScopedMessagePipeHandle handle_;
  uint32_t version_;

  DISALLOW_COPY_AND_ASSIGN(InterfacePtrInfo);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_PTR_INFO_H_
