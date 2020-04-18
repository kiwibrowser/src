// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/interface_factory_impl.h"

#include <memory>
#include "base/guid.h"

#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/base/media_log.h"
#include "media/mojo/services/mojo_decryptor_service.h"
#include "media/mojo/services/mojo_media_client.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/mojom/interface_provider.mojom.h"

#if BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)
#include "media/mojo/services/mojo_audio_decoder_service.h"
#endif  // BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
#include "media/mojo/services/mojo_video_decoder_service.h"
#endif  // BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_RENDERER)
#include "base/bind_helpers.h"
#include "media/base/renderer.h"
#include "media/mojo/services/mojo_renderer_service.h"
#endif  // BUILDFLAG(ENABLE_MOJO_RENDERER)

#if BUILDFLAG(ENABLE_MOJO_CDM)
#include "media/base/cdm_factory.h"
#include "media/mojo/services/mojo_cdm_service.h"
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "media/mojo/services/mojo_cdm_proxy_service.h"
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

namespace media {

InterfaceFactoryImpl::InterfaceFactoryImpl(
    service_manager::mojom::InterfaceProviderPtr interfaces,
    MediaLog* media_log,
    std::unique_ptr<service_manager::ServiceContextRef> connection_ref,
    MojoMediaClient* mojo_media_client)
    :
#if BUILDFLAG(ENABLE_MOJO_RENDERER)
      media_log_(media_log),
#endif
#if BUILDFLAG(ENABLE_MOJO_CDM)
      interfaces_(std::move(interfaces)),
#endif
      connection_ref_(std::move(connection_ref)),
      mojo_media_client_(mojo_media_client) {
  DVLOG(1) << __func__;
  DCHECK(mojo_media_client_);

  SetBindingConnectionErrorHandler();
}

InterfaceFactoryImpl::~InterfaceFactoryImpl() {
  DVLOG(1) << __func__;
}

// mojom::InterfaceFactory implementation.

void InterfaceFactoryImpl::CreateAudioDecoder(
    mojo::InterfaceRequest<mojom::AudioDecoder> request) {
  DVLOG(2) << __func__;
#if BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)
  scoped_refptr<base::SingleThreadTaskRunner> task_runner(
      base::ThreadTaskRunnerHandle::Get());

  std::unique_ptr<AudioDecoder> audio_decoder =
      mojo_media_client_->CreateAudioDecoder(task_runner);
  if (!audio_decoder) {
    DLOG(ERROR) << "AudioDecoder creation failed.";
    return;
  }

  audio_decoder_bindings_.AddBinding(
      std::make_unique<MojoAudioDecoderService>(&cdm_service_context_,
                                                std::move(audio_decoder)),
      std::move(request));
#endif  // BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)
}

void InterfaceFactoryImpl::CreateVideoDecoder(
    mojom::VideoDecoderRequest request) {
  DVLOG(2) << __func__;
#if BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
  video_decoder_bindings_.AddBinding(
      std::make_unique<MojoVideoDecoderService>(mojo_media_client_,
                                                &cdm_service_context_),
      std::move(request));
#endif  // BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
}

void InterfaceFactoryImpl::CreateRenderer(
    media::mojom::HostedRendererType type,
    const std::string& type_specific_id,
    mojo::InterfaceRequest<mojom::Renderer> request) {
  DVLOG(2) << __func__;
#if BUILDFLAG(ENABLE_MOJO_RENDERER)
  // Creation requests for non default renderers should have already been
  // handled by now, in a different layer.
  if (type != media::mojom::HostedRendererType::kDefault) {
    DLOG(ERROR) << "Creation of specialized renderers is not supported.";
    return;
  }

  // For HostedRendererType::kDefault type, |type_specific_id| represents an
  // audio device ID. See interface_factory.mojom.
  const std::string& audio_device_id = type_specific_id;
  auto renderer = mojo_media_client_->CreateRenderer(
      base::ThreadTaskRunnerHandle::Get(), media_log_, audio_device_id);
  if (!renderer) {
    DLOG(ERROR) << "Renderer creation failed.";
    return;
  }

  std::unique_ptr<MojoRendererService> mojo_renderer_service =
      std::make_unique<MojoRendererService>(
          &cdm_service_context_, std::move(renderer),
          MojoRendererService::InitiateSurfaceRequestCB());

  MojoRendererService* mojo_renderer_service_ptr = mojo_renderer_service.get();

  mojo::BindingId binding_id = renderer_bindings_.AddBinding(
      std::move(mojo_renderer_service), std::move(request));

  // base::Unretained() is safe because the callback will be fired by
  // |mojo_renderer_service|, which is owned by |renderer_bindings_|.
  mojo_renderer_service_ptr->set_bad_message_cb(
      base::Bind(base::IgnoreResult(
                     &mojo::StrongBindingSet<mojom::Renderer>::RemoveBinding),
                 base::Unretained(&renderer_bindings_), binding_id));
#endif  // BUILDFLAG(ENABLE_MOJO_RENDERER)
}

void InterfaceFactoryImpl::CreateCdm(
    const std::string& /* key_system */,
    mojo::InterfaceRequest<mojom::ContentDecryptionModule> request) {
  DVLOG(2) << __func__;
#if BUILDFLAG(ENABLE_MOJO_CDM)
  CdmFactory* cdm_factory = GetCdmFactory();
  if (!cdm_factory)
    return;

  cdm_bindings_.AddBinding(
      std::make_unique<MojoCdmService>(cdm_factory, &cdm_service_context_),
      std::move(request));
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)
}

void InterfaceFactoryImpl::CreateDecryptor(int cdm_id,
                                           mojom::DecryptorRequest request) {
  DVLOG(2) << __func__;
  auto mojo_decryptor_service =
      MojoDecryptorService::Create(cdm_id, &cdm_service_context_);
  if (!mojo_decryptor_service) {
    DLOG(ERROR) << "MojoDecryptorService creation failed.";
    return;
  }

  decryptor_bindings_.AddBinding(std::move(mojo_decryptor_service),
                                 std::move(request));
}

void InterfaceFactoryImpl::CreateCdmProxy(const std::string& cdm_guid,
                                          mojom::CdmProxyRequest request) {
  DVLOG(2) << __func__;
#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  if (!base::IsValidGUID(cdm_guid)) {
    DLOG(ERROR) << "Invalid CDM GUID: " << cdm_guid;
    return;
  }

  auto cdm_proxy = mojo_media_client_->CreateCdmProxy(cdm_guid);
  if (!cdm_proxy) {
    DLOG(ERROR) << "CdmProxy creation failed.";
    return;
  }

  cdm_proxy_bindings_.AddBinding(
      std::make_unique<MojoCdmProxyService>(std::move(cdm_proxy),
                                            &cdm_service_context_),
      std::move(request));
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)
}

void InterfaceFactoryImpl::OnDestroyPending(base::OnceClosure destroy_cb) {
  DVLOG(1) << __func__;
  destroy_cb_ = std::move(destroy_cb);
  if (IsEmpty())
    std::move(destroy_cb_).Run();
  // else the callback will be called when IsEmpty() becomes true.
}

bool InterfaceFactoryImpl::IsEmpty() {
#if BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)
  if (!audio_decoder_bindings_.empty())
    return false;
#endif  // BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
  if (!video_decoder_bindings_.empty())
    return false;
#endif  // BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_RENDERER)
  if (!renderer_bindings_.empty())
    return false;
#endif  // BUILDFLAG(ENABLE_MOJO_RENDERER)

#if BUILDFLAG(ENABLE_MOJO_CDM)
  if (!cdm_bindings_.empty())
    return false;
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  if (!cdm_proxy_bindings_.empty())
    return false;
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

  if (!decryptor_bindings_.empty())
    return false;

  return true;
}

void InterfaceFactoryImpl::SetBindingConnectionErrorHandler() {
  // base::Unretained is safe because all bindings are owned by |this|. If
  // |this| is destructed, the bindings will be destructed as well and the
  // connection error handler should never be called.
  auto connection_error_cb = base::BindRepeating(
      &InterfaceFactoryImpl::OnBindingConnectionError, base::Unretained(this));

#if BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)
  audio_decoder_bindings_.set_connection_error_handler(connection_error_cb);
#endif  // BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
  video_decoder_bindings_.set_connection_error_handler(connection_error_cb);
#endif  // BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_RENDERER)
  renderer_bindings_.set_connection_error_handler(connection_error_cb);
#endif  // BUILDFLAG(ENABLE_MOJO_RENDERER)

#if BUILDFLAG(ENABLE_MOJO_CDM)
  cdm_bindings_.set_connection_error_handler(connection_error_cb);
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  cdm_proxy_bindings_.set_connection_error_handler(connection_error_cb);
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

  decryptor_bindings_.set_connection_error_handler(connection_error_cb);
}

void InterfaceFactoryImpl::OnBindingConnectionError() {
  DVLOG(2) << __func__;
  if (destroy_cb_ && IsEmpty())
    std::move(destroy_cb_).Run();
}

#if BUILDFLAG(ENABLE_MOJO_CDM)
CdmFactory* InterfaceFactoryImpl::GetCdmFactory() {
  if (!cdm_factory_) {
    cdm_factory_ = mojo_media_client_->CreateCdmFactory(interfaces_.get());
    LOG_IF(ERROR, !cdm_factory_) << "CdmFactory not available.";
  }
  return cdm_factory_.get();
}
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

}  // namespace media
