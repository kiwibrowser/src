// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_CPP_PRIVATE_FLASH_DRM_H_
#define PPAPI_CPP_PRIVATE_FLASH_DRM_H_

#include <stdint.h>

#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/file_ref.h"
#include "ppapi/cpp/resource.h"

namespace pp {

class FileRef;

namespace flash {

class DRM : public Resource {
 public:
  DRM();
  explicit DRM(const InstanceHandle& instance);

  // On success, returns a string var.
  int32_t GetDeviceID(const CompletionCallbackWithOutput<Var>& callback);
  // Outputs the HMONITOR associated with the current plugin instance in
  // |hmonitor|. True is returned upon success.
  bool GetHmonitor(int64_t* hmonitor);
  // Returns the voucher file as a FileRef or an invalid resource on failure.
  int32_t GetVoucherFile(const CompletionCallbackWithOutput<FileRef>& callback);
  // On success, returns a value indicating if the monitor associated with the
  // current plugin instance is external.
  int32_t MonitorIsExternal(
      const CompletionCallbackWithOutput<PP_Bool>& callback);
};

}  // namespace flash
}  // namespace pp

#endif  // PPAPI_CPP_PRIVATE_FLASH_DRM_H_
