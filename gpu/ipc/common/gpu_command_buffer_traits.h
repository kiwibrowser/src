// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_GPU_COMMAND_BUFFER_TRAITS_H_
#define GPU_IPC_COMMON_GPU_COMMAND_BUFFER_TRAITS_H_

#include "gpu/command_buffer/common/id_type.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/common/gpu_command_buffer_traits_multi.h"
#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_param_traits.h"

namespace gpu {
struct Mailbox;
struct MailboxHolder;
struct SyncToken;
struct TextureInUseResponse;
}

namespace IPC {

template <>
struct GPU_EXPORT ParamTraits<gpu::SyncToken> {
  using param_type = gpu::SyncToken;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct GPU_EXPORT ParamTraits<gpu::TextureInUseResponse> {
  using param_type = gpu::TextureInUseResponse;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template<>
struct GPU_EXPORT ParamTraits<gpu::Mailbox> {
  using param_type = gpu::Mailbox;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct GPU_EXPORT ParamTraits<gpu::MailboxHolder> {
  using param_type = gpu::MailboxHolder;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template <typename TypeMarker, typename WrappedType, WrappedType kInvalidValue>
struct ParamTraits<gpu::IdType<TypeMarker, WrappedType, kInvalidValue>> {
  using param_type = gpu::IdType<TypeMarker, WrappedType, kInvalidValue>;
  static void Write(base::Pickle* m, const param_type& p) {
    WriteParam(m, p.GetUnsafeValue());
  }
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r) {
    WrappedType value;
    if (!ReadParam(m, iter, &value))
      return false;
    *r = param_type::FromUnsafeValue(value);
    return true;
  }
  static void Log(const param_type& p, std::string* l) {
    LogParam(p.GetUnsafeValue(), l);
  }
};

}  // namespace IPC

#endif  // GPU_IPC_COMMON_GPU_COMMAND_BUFFER_TRAITS_H_
