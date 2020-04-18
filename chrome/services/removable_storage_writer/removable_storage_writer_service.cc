// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/removable_storage_writer/removable_storage_writer_service.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "build/build_config.h"
#include "chrome/services/removable_storage_writer/public/mojom/removable_storage_writer.mojom.h"
#include "chrome/services/removable_storage_writer/removable_storage_writer.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace {

void OnRemovableStorageWriterGetterRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    chrome::mojom::RemovableStorageWriterRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<RemovableStorageWriter>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

RemovableStorageWriterService::RemovableStorageWriterService() = default;

RemovableStorageWriterService::~RemovableStorageWriterService() = default;

std::unique_ptr<service_manager::Service>
RemovableStorageWriterService::CreateService() {
  return std::make_unique<RemovableStorageWriterService>();
}

void RemovableStorageWriterService::OnStart() {
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      context()->CreateQuitClosure());
  registry_.AddInterface(
      base::Bind(&OnRemovableStorageWriterGetterRequest, ref_factory_.get()));
}

void RemovableStorageWriterService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}
