// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/patch/patch_service.h"

#include "build/build_config.h"
#include "components/services/patch/file_patcher_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace patch {

namespace {

void OnFilePatcherRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    patch::mojom::FilePatcherRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<FilePatcherImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

PatchService::PatchService() = default;

PatchService::~PatchService() = default;

std::unique_ptr<service_manager::Service> PatchService::CreateService() {
  return std::make_unique<PatchService>();
}

void PatchService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      context()->CreateQuitClosure()));
  registry_.AddInterface(base::Bind(&OnFilePatcherRequest, ref_factory_.get()));
}

void PatchService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  //  namespace patch
