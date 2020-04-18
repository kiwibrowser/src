// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/mojo/blink_interface_provider_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/service_names.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace content {

namespace {

void BindNamedInterface(base::WeakPtr<service_manager::Connector> connector,
                        const std::string& name,
                        mojo::ScopedMessagePipeHandle handle) {
  if (!connector)
    return;

  connector->BindInterface(
      service_manager::Identity(mojom::kBrowserServiceName,
                                service_manager::mojom::kInheritUserID),
      name, std::move(handle));
}

}  // namespace

BlinkInterfaceProviderImpl::BlinkInterfaceProviderImpl(
    service_manager::Connector* connector)
    : connector_(connector->GetWeakPtr()),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

BlinkInterfaceProviderImpl::~BlinkInterfaceProviderImpl() = default;

void BlinkInterfaceProviderImpl::GetInterface(
    const char* name,
    mojo::ScopedMessagePipeHandle handle) {
  // Construct a closure that can safely be passed across threads if necessary.
  base::OnceClosure closure = base::BindOnce(
      &BindNamedInterface, connector_, std::string(name), std::move(handle));

  if (main_thread_task_runner_->BelongsToCurrentThread()) {
    std::move(closure).Run();
    return;
  }

  main_thread_task_runner_->PostTask(FROM_HERE, std::move(closure));
}

}  // namespace content
