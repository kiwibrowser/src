// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/media_service.h"

#include "base/logging.h"
#include "media/media_buildflags.h"
#include "media/mojo/services/interface_factory_impl.h"
#include "media/mojo/services/mojo_media_client.h"
#include "services/service_manager/public/cpp/connector.h"

namespace media {

MediaService::MediaService(std::unique_ptr<MojoMediaClient> mojo_media_client)
    : mojo_media_client_(std::move(mojo_media_client)) {
  DCHECK(mojo_media_client_);
  registry_.AddInterface<mojom::MediaService>(
      base::Bind(&MediaService::Create, base::Unretained(this)));
}

MediaService::~MediaService() = default;

void MediaService::OnStart() {
  DVLOG(1) << __func__;

  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      context()->CreateQuitClosure()));
  mojo_media_client_->Initialize(context()->connector());
}

void MediaService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  DVLOG(1) << __func__ << ": interface_name = " << interface_name;

  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

bool MediaService::OnServiceManagerConnectionLost() {
  interface_factory_bindings_.CloseAllBindings();
  mojo_media_client_.reset();
  return true;
}

void MediaService::Create(mojom::MediaServiceRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void MediaService::CreateInterfaceFactory(
    mojom::InterfaceFactoryRequest request,
    service_manager::mojom::InterfaceProviderPtr host_interfaces) {
  // Ignore request if service has already stopped.
  if (!mojo_media_client_)
    return;

  interface_factory_bindings_.AddBinding(
      std::make_unique<InterfaceFactoryImpl>(
          std::move(host_interfaces), &media_log_, ref_factory_->CreateRef(),
          mojo_media_client_.get()),
      std::move(request));
}

}  // namespace media
