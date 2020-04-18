// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/ipc/capture_param_traits.h"

#include "base/strings/stringprintf.h"
#include "ipc/ipc_message_utils.h"
#include "media/base/ipc/media_param_traits.h"
#include "media/base/limits.h"
#include "media/capture/video_capture_types.h"
#include "ui/gfx/ipc/geometry/gfx_param_traits.h"
#include "ui/gfx/ipc/gfx_param_traits.h"
#include "ui/gfx/ipc/skia/gfx_skia_param_traits.h"

using media::VideoCaptureFormat;

namespace IPC {

void ParamTraits<VideoCaptureFormat>::Write(base::Pickle* m,
                                            const VideoCaptureFormat& p) {
  WriteParam(m, p.frame_size);
  WriteParam(m, p.frame_rate);
  WriteParam(m, p.pixel_format);
}

bool ParamTraits<VideoCaptureFormat>::Read(const base::Pickle* m,
                                           base::PickleIterator* iter,
                                           VideoCaptureFormat* r) {
  if (!ReadParam(m, iter, &r->frame_size) ||
      !ReadParam(m, iter, &r->frame_rate) ||
      !ReadParam(m, iter, &r->pixel_format)) {
    return false;
  }
  return r->IsValid();
}

void ParamTraits<VideoCaptureFormat>::Log(const VideoCaptureFormat& p,
                                          std::string* l) {
  l->append(base::StringPrintf("<VideoCaptureFormat> %s",
                               media::VideoCaptureFormat::ToString(p).c_str()));
}

}  // namespace IPC

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#undef MEDIA_CAPTURE_IPC_CAPTURE_PARAM_TRAITS_MACROS_H_
#include "media/capture/ipc/capture_param_traits_macros.h"
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#undef MEDIA_CAPTURE_IPC_CAPTURE_PARAM_TRAITS_MACROS_H_
#include "media/capture/ipc/capture_param_traits_macros.h"
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#undef MEDIA_CAPTURE_IPC_CAPTURE_PARAM_TRAITS_MACROS_H_
#include "media/capture/ipc/capture_param_traits_macros.h"
}  // namespace IPC
