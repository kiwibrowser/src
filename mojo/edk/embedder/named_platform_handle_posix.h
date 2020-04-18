// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_POSIX_H_
#define MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_POSIX_H_

#include <string>

#include "base/strings/string_piece.h"
#include "mojo/edk/system/system_impl_export.h"

namespace mojo {
namespace edk {

struct MOJO_SYSTEM_IMPL_EXPORT NamedPlatformHandle {
  NamedPlatformHandle() {}
  explicit NamedPlatformHandle(const base::StringPiece& name)
      : name(name.as_string()) {}

  bool is_valid() const { return !name.empty(); }

  std::string name;
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_POSIX_H_
