// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/gpu/shell_content_gpu_client.h"

#include "content/shell/common/power_monitor_test_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {

ShellContentGpuClient::ShellContentGpuClient() = default;

ShellContentGpuClient::~ShellContentGpuClient() = default;

void ShellContentGpuClient::InitializeRegistry(
    service_manager::BinderRegistry* registry) {
  registry->AddInterface<mojom::PowerMonitorTest>(
      base::Bind(&PowerMonitorTestImpl::MakeStrongBinding,
                 base::Passed(std::make_unique<PowerMonitorTestImpl>())),
      base::ThreadTaskRunnerHandle::Get());
}

}  // namespace content
