// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/util_win/util_win_service.h"

#include <memory>

#include "build/build_config.h"
#include "chrome/services/util_win/public/mojom/shell_util_win.mojom.h"
#include "chrome/services/util_win/shell_util_win_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace {

void OnShellUtilWinRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    chrome::mojom::ShellUtilWinRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<ShellUtilWinImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

UtilWinService::UtilWinService() = default;

UtilWinService::~UtilWinService() = default;

std::unique_ptr<service_manager::Service> UtilWinService::CreateService() {
  return std::unique_ptr<service_manager::Service>(new UtilWinService());
}

void UtilWinService::OnStart() {
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      context()->CreateQuitClosure());
  registry_.AddInterface(
      base::Bind(&OnShellUtilWinRequest, ref_factory_.get()));
}

void UtilWinService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}
