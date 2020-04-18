// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_GPU_CLIENT_H_
#define CONTENT_PUBLIC_BROWSER_GPU_CLIENT_H_

#include <memory>

#include "base/callback_forward.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "services/ui/public/interfaces/gpu.mojom.h"

namespace content {

// GpuClient provides an implementation of ui::mojom::Gpu.
class CONTENT_EXPORT GpuClient {
 public:
  virtual ~GpuClient() {}

  using ConnectionErrorHandlerClosure =
      base::OnceCallback<void(GpuClient* client)>;
  static std::unique_ptr<GpuClient, BrowserThread::DeleteOnIOThread> Create(
      ui::mojom::GpuRequest request,
      ConnectionErrorHandlerClosure connection_error_handler);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_GPU_CLIENT_H_
