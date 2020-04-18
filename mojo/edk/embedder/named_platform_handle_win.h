// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_WIN_H_
#define MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_WIN_H_

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "mojo/edk/system/system_impl_export.h"

namespace mojo {
namespace edk {

struct MOJO_SYSTEM_IMPL_EXPORT NamedPlatformHandle {
  NamedPlatformHandle() {}
  explicit NamedPlatformHandle(const base::StringPiece& name)
      : name(base::UTF8ToUTF16(name)) {}

  explicit NamedPlatformHandle(const base::StringPiece16& name)
      : name(name.as_string()) {}

  bool is_valid() const { return !name.empty(); }

  base::string16 pipe_name() const { return L"\\\\.\\pipe\\mojo." + name; }

  base::string16 name;
};

}  // namespace edk
}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_NAMED_PLATFORM_HANDLE_WIN_H_
