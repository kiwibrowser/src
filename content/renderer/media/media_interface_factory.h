// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MEDIA_INTERFACE_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_MEDIA_INTERFACE_FACTORY_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "content/common/content_export.h"
#include "media/mojo/interfaces/interface_factory.mojom.h"
#include "url/gurl.h"

namespace service_manager {
class InterfaceProvider;
}

namespace content {

// MediaInterfaceFactory is an implementation of media::mojom::InterfaceFactory
// that provides thread safety and handles disconnection error automatically.
// The Create* methods can be called on any thread.
class CONTENT_EXPORT MediaInterfaceFactory
    : public media::mojom::InterfaceFactory {
 public:
  explicit MediaInterfaceFactory(
      service_manager::InterfaceProvider* remote_interfaces);
  ~MediaInterfaceFactory() final;

  // media::mojom::InterfaceFactory implementation.
  void CreateAudioDecoder(media::mojom::AudioDecoderRequest request) final;
  void CreateVideoDecoder(media::mojom::VideoDecoderRequest request) final;
  void CreateRenderer(media::mojom::HostedRendererType type,
                      const std::string& type_specific_id,
                      media::mojom::RendererRequest request) final;
  void CreateCdm(const std::string& key_system,
                 media::mojom::ContentDecryptionModuleRequest request) final;
  void CreateDecryptor(int cdm_id,
                       media::mojom::DecryptorRequest request) final;
  // TODO(xhwang): We should not expose this here.
  void CreateCdmProxy(const std::string& cdm_guid,
                      media::mojom::CdmProxyRequest request) final;

 private:
  media::mojom::InterfaceFactory* GetMediaInterfaceFactory();
  void OnConnectionError();

  service_manager::InterfaceProvider* remote_interfaces_;
  media::mojom::InterfaceFactoryPtr media_interface_factory_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::WeakPtr<MediaInterfaceFactory> weak_this_;
  base::WeakPtrFactory<MediaInterfaceFactory> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaInterfaceFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MEDIA_INTERFACE_FACTORY_H_
