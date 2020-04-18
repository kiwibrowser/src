// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/gpu_service_registry.h"

#include "content/browser/gpu/gpu_process_host.h"

namespace content {

void BindInterfaceInGpuProcess(const std::string& interface_name,
                               mojo::ScopedMessagePipeHandle interface_pipe) {
  GpuProcessHost* host = GpuProcessHost::Get();
  return host->BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace content
