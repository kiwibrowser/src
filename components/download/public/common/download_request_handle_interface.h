// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_REQUEST_HANDLE_INTERFACE_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_REQUEST_HANDLE_INTERFACE_H_

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "components/download/public/common/download_export.h"

namespace download {

// A handle used by the download system for operations on the network request.
class COMPONENTS_DOWNLOAD_EXPORT DownloadRequestHandleInterface {
 public:
  virtual ~DownloadRequestHandleInterface() = default;

  // Pauses or resumes the network request.
  virtual void PauseRequest() = 0;
  virtual void ResumeRequest() = 0;

  // Cancels the request.
  virtual void CancelRequest(bool user_cancel) = 0;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_COMMON_DOWNLOAD_REQUEST_HANDLE_INTERFACE_H_
