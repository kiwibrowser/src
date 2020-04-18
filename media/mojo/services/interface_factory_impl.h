// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_INTERFACE_FACTORY_IMPL_H_
#define MEDIA_MOJO_SERVICES_INTERFACE_FACTORY_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "media/mojo/buildflags.h"
#include "media/mojo/interfaces/interface_factory.mojom.h"
#include "media/mojo/services/deferred_destroy_strong_binding_set.h"
#include "media/mojo/services/mojo_cdm_service_context.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace media {

class CdmFactory;
class MediaLog;
class MojoMediaClient;

class InterfaceFactoryImpl : public DeferredDestroy<mojom::InterfaceFactory> {
 public:
  InterfaceFactoryImpl(
      service_manager::mojom::InterfaceProviderPtr interfaces,
      MediaLog* media_log,
      std::unique_ptr<service_manager::ServiceContextRef> connection_ref,
      MojoMediaClient* mojo_media_client);
  ~InterfaceFactoryImpl() final;

  // mojom::InterfaceFactory implementation.
  void CreateAudioDecoder(mojom::AudioDecoderRequest request) final;
  void CreateVideoDecoder(mojom::VideoDecoderRequest request) final;
  void CreateRenderer(media::mojom::HostedRendererType type,
                      const std::string& type_specific_id,
                      mojom::RendererRequest request) final;
  void CreateCdm(const std::string& key_system,
                 mojom::ContentDecryptionModuleRequest request) final;
  void CreateDecryptor(int cdm_id, mojom::DecryptorRequest request) final;
  void CreateCdmProxy(const std::string& cdm_guid,
                      mojom::CdmProxyRequest request) final;

  // DeferredDestroy<mojom::InterfaceFactory> implemenation.
  void OnDestroyPending(base::OnceClosure destroy_cb) final;

 private:
  // Returns true when there is no media component (audio/video decoder,
  // renderer, cdm and cdm proxy) bindings exist.
  bool IsEmpty();

  void SetBindingConnectionErrorHandler();
  void OnBindingConnectionError();

#if BUILDFLAG(ENABLE_MOJO_CDM)
  CdmFactory* GetCdmFactory();
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

  // Must be declared before the bindings below because the bound objects might
  // take a raw pointer of |cdm_service_context_| and assume it's always
  // available.
  MojoCdmServiceContext cdm_service_context_;

#if BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)
  mojo::StrongBindingSet<mojom::AudioDecoder> audio_decoder_bindings_;
#endif  // BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
  mojo::StrongBindingSet<mojom::VideoDecoder> video_decoder_bindings_;
#endif  // BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)

#if BUILDFLAG(ENABLE_MOJO_RENDERER)
  MediaLog* media_log_;
  mojo::StrongBindingSet<mojom::Renderer> renderer_bindings_;
#endif  // BUILDFLAG(ENABLE_MOJO_RENDERER)

#if BUILDFLAG(ENABLE_MOJO_CDM)
  std::unique_ptr<CdmFactory> cdm_factory_;
  service_manager::mojom::InterfaceProviderPtr interfaces_;
  mojo::StrongBindingSet<mojom::ContentDecryptionModule> cdm_bindings_;
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  mojo::StrongBindingSet<mojom::CdmProxy> cdm_proxy_bindings_;
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

  mojo::StrongBindingSet<mojom::Decryptor> decryptor_bindings_;

  std::unique_ptr<service_manager::ServiceContextRef> connection_ref_;
  MojoMediaClient* mojo_media_client_;
  base::OnceClosure destroy_cb_;

  DISALLOW_COPY_AND_ASSIGN(InterfaceFactoryImpl);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_INTERFACE_FACTORY_IMPL_H_
