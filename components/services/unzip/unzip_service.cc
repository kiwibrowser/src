// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/unzip/unzip_service.h"

#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "components/services/unzip/unzipper_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace unzip {

namespace {

void OnUnzipRequest(service_manager::ServiceContextRefFactory* ref_factory,
                    unzip::mojom::UnzipperRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<UnzipperImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

UnzipService::UnzipService() = default;

UnzipService::~UnzipService() = default;

std::unique_ptr<service_manager::Service> UnzipService::CreateService() {
  return base::WrapUnique(new UnzipService());
}

void UnzipService::OnStart() {
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      context()->CreateQuitClosure());
  registry_.AddInterface(
      base::BindRepeating(&OnUnzipRequest, ref_factory_.get()));
}

void UnzipService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  //  namespace unzip
