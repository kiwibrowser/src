// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipc/handle_fuchsia.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "ipc/handle_attachment_fuchsia.h"
#include "ipc/ipc_message.h"

namespace IPC {

HandleFuchsia::HandleFuchsia() : handle_(ZX_HANDLE_INVALID) {}

HandleFuchsia::HandleFuchsia(const zx_handle_t& handle) : handle_(handle) {}

// static
void ParamTraits<HandleFuchsia>::Write(base::Pickle* m, const param_type& p) {
  scoped_refptr<IPC::internal::HandleAttachmentFuchsia> attachment(
      new IPC::internal::HandleAttachmentFuchsia(p.get_handle()));
  if (!m->WriteAttachment(std::move(attachment)))
    NOTREACHED();
}

// static
bool ParamTraits<HandleFuchsia>::Read(const base::Pickle* m,
                                      base::PickleIterator* iter,
                                      param_type* r) {
  scoped_refptr<base::Pickle::Attachment> base_attachment;
  if (!m->ReadAttachment(iter, &base_attachment))
    return false;
  MessageAttachment* attachment =
      static_cast<MessageAttachment*>(base_attachment.get());
  if (attachment->GetType() != MessageAttachment::Type::FUCHSIA_HANDLE)
    return false;
  IPC::internal::HandleAttachmentFuchsia* handle_attachment =
      static_cast<IPC::internal::HandleAttachmentFuchsia*>(attachment);
  r->set_handle(handle_attachment->Take());
  return true;
}

// static
void ParamTraits<HandleFuchsia>::Log(const param_type& p, std::string* l) {
  l->append(base::StringPrintf("0x%x", p.get_handle()));
}

}  // namespace IPC
