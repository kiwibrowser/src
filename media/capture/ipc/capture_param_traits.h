// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_IPC_CAPTURE_PARAM_TRAITS_H_
#define MEDIA_CAPTURE_IPC_CAPTURE_PARAM_TRAITS_H_

#include "ipc/ipc_message.h"
#include "ipc/ipc_param_traits.h"
#include "media/capture/ipc/capture_param_traits_macros.h"

namespace media {
struct VideoCaptureFormat;
}

namespace IPC {

template <>
struct ParamTraits<media::VideoCaptureFormat> {
  typedef media::VideoCaptureFormat param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

}  // namespace IPC

#endif  // MEDIA_CAPTURE_IPC_CAPTURE_PARAM_TRAITS_H_
