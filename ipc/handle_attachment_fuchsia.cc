// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipc/handle_attachment_fuchsia.h"

#include <zircon/syscalls.h>
#include <zircon/types.h>

namespace IPC {
namespace internal {

HandleAttachmentFuchsia::HandleAttachmentFuchsia(const zx_handle_t& handle) {
  zx_status_t result =
      zx_handle_duplicate(handle, ZX_RIGHT_SAME_RIGHTS, handle_.receive());
  DLOG_IF(ERROR, result != ZX_OK)
      << "zx_handle_duplicate: " << zx_status_get_string(result);
}

HandleAttachmentFuchsia::HandleAttachmentFuchsia(base::ScopedZxHandle handle)
    : handle_(std::move(handle)) {}

HandleAttachmentFuchsia::~HandleAttachmentFuchsia() {}

MessageAttachment::Type HandleAttachmentFuchsia::GetType() const {
  return Type::FUCHSIA_HANDLE;
}

}  // namespace internal
}  // namespace IPC
