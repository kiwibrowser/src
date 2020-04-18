// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/media_interface_factory.h"

#include <string>

#include "base/bind.h"
#include "media/mojo/interfaces/content_decryption_module.mojom.h"
#include "media/mojo/interfaces/renderer.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace content {

MediaInterfaceFactory::MediaInterfaceFactory(
    service_manager::InterfaceProvider* remote_interfaces)
    : remote_interfaces_(remote_interfaces), weak_factory_(this) {
  task_runner_ = base::ThreadTaskRunnerHandle::Get();
  weak_this_ = weak_factory_.GetWeakPtr();
}

MediaInterfaceFactory::~MediaInterfaceFactory() {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

void MediaInterfaceFactory::CreateAudioDecoder(
    media::mojom::AudioDecoderRequest request) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&MediaInterfaceFactory::CreateAudioDecoder,
                                  weak_this_, std::move(request)));
    return;
  }

  DVLOG(1) << __func__;
  GetMediaInterfaceFactory()->CreateAudioDecoder(std::move(request));
}

void MediaInterfaceFactory::CreateVideoDecoder(
    media::mojom::VideoDecoderRequest request) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&MediaInterfaceFactory::CreateVideoDecoder,
                                  weak_this_, std::move(request)));
    return;
  }

  DVLOG(1) << __func__;
  GetMediaInterfaceFactory()->CreateVideoDecoder(std::move(request));
}

void MediaInterfaceFactory::CreateRenderer(
    media::mojom::HostedRendererType type,
    const std::string& type_specific_id,
    media::mojom::RendererRequest request) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&MediaInterfaceFactory::CreateRenderer, weak_this_, type,
                       type_specific_id, std::move(request)));
    return;
  }

  DVLOG(1) << __func__;
  GetMediaInterfaceFactory()->CreateRenderer(type, type_specific_id,
                                             std::move(request));
}

void MediaInterfaceFactory::CreateCdm(
    const std::string& key_system,
    media::mojom::ContentDecryptionModuleRequest request) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&MediaInterfaceFactory::CreateCdm, weak_this_,
                                  key_system, std::move(request)));
    return;
  }

  DVLOG(1) << __func__ << ": key_system = " << key_system;
  GetMediaInterfaceFactory()->CreateCdm(key_system, std::move(request));
}

void MediaInterfaceFactory::CreateDecryptor(
    int cdm_id,
    media::mojom::DecryptorRequest request) {
  if (!task_runner_->BelongsToCurrentThread()) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&MediaInterfaceFactory::CreateDecryptor,
                                  weak_this_, cdm_id, std::move(request)));
    return;
  }

  DVLOG(1) << __func__;
  GetMediaInterfaceFactory()->CreateDecryptor(cdm_id, std::move(request));
}

void MediaInterfaceFactory::CreateCdmProxy(
    const std::string& cdm_guid,
    media::mojom::CdmProxyRequest request) {
  NOTREACHED() << "CdmProxy should only be connected from a library CDM";
}

media::mojom::InterfaceFactory*
MediaInterfaceFactory::GetMediaInterfaceFactory() {
  DVLOG(1) << __func__;
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (!media_interface_factory_) {
    remote_interfaces_->GetInterface(&media_interface_factory_);
    media_interface_factory_.set_connection_error_handler(base::BindOnce(
        &MediaInterfaceFactory::OnConnectionError, base::Unretained(this)));
  }

  return media_interface_factory_.get();
}

void MediaInterfaceFactory::OnConnectionError() {
  DVLOG(1) << __func__;
  DCHECK(task_runner_->BelongsToCurrentThread());

  media_interface_factory_.reset();
}

}  // namespace content
