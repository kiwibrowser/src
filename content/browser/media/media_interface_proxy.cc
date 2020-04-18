// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/media_interface_proxy.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "content/browser/frame_host/render_frame_host_delegate.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/service_manager_connection.h"
#include "media/mojo/buildflags.h"
#include "media/mojo/interfaces/constants.mojom.h"
#include "media/mojo/interfaces/media_service.mojom.h"
#include "media/mojo/services/media_interface_provider.h"
#include "services/service_manager/public/cpp/connector.h"

#if BUILDFLAG(ENABLE_MOJO_CDM)
#include "content/public/browser/browser_context.h"
#include "content/public/browser/provision_fetcher_impl.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#endif

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "base/guid.h"
#include "content/browser/media/cdm_storage_impl.h"
#include "content/browser/media/key_system_support_impl.h"
#include "content/public/common/cdm_info.h"
#include "media/base/key_system_names.h"
#include "media/mojo/interfaces/cdm_service.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#if defined(OS_MACOSX)
#include "sandbox/mac/seatbelt_extension.h"
#endif  // defined(OS_MACOSX)
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

#if defined(OS_ANDROID)
#include "content/browser/media/android/media_player_renderer.h"
#include "content/browser/media/flinging_renderer.h"
#include "media/mojo/services/mojo_renderer_service.h"  // nogncheck
#endif

namespace content {

#if BUILDFLAG(ENABLE_LIBRARY_CDMS) && defined(OS_MACOSX)

namespace {

#if BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)
// TODO(xhwang): Move this to a common place.
const base::FilePath::CharType kSignatureFileExtension[] =
    FILE_PATH_LITERAL(".sig");

// Returns the signature file path given the |file_path|. This function should
// only be used when the signature file and the file are located in the same
// directory, which is the case for the CDM and CDM adapter.
base::FilePath GetSigFilePath(const base::FilePath& file_path) {
  return file_path.AddExtension(kSignatureFileExtension);
}
#endif  // BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)

class SeatbeltExtensionTokenProviderImpl
    : public media::mojom::SeatbeltExtensionTokenProvider {
 public:
  explicit SeatbeltExtensionTokenProviderImpl(const base::FilePath& cdm_path)
      : cdm_path_(cdm_path) {}
  void GetTokens(GetTokensCallback callback) final {
    std::vector<sandbox::SeatbeltExtensionToken> tokens;

    // Allow the CDM to be loaded in the CDM service process.
    auto cdm_token = sandbox::SeatbeltExtension::Issue(
        sandbox::SeatbeltExtension::FILE_READ, cdm_path_.value());
    if (cdm_token) {
      tokens.push_back(std::move(*cdm_token));
    } else {
      std::move(callback).Run({});
      return;
    }

#if BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)
    // If CDM host verification is enabled, also allow to open the CDM signature
    // file.
    auto cdm_sig_token =
        sandbox::SeatbeltExtension::Issue(sandbox::SeatbeltExtension::FILE_READ,
                                          GetSigFilePath(cdm_path_).value());
    if (cdm_sig_token) {
      tokens.push_back(std::move(*cdm_sig_token));
    } else {
      std::move(callback).Run({});
      return;
    }
#endif  // BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)

    std::move(callback).Run(std::move(tokens));
  }

 private:
  base::FilePath cdm_path_;

  DISALLOW_COPY_AND_ASSIGN(SeatbeltExtensionTokenProviderImpl);
};

}  // namespace

#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS) && defined(OS_MACOSX)

MediaInterfaceProxy::MediaInterfaceProxy(
    RenderFrameHost* render_frame_host,
    media::mojom::InterfaceFactoryRequest request,
    const base::Closure& error_handler)
    : render_frame_host_(render_frame_host),
      binding_(this, std::move(request)) {
  DVLOG(1) << __func__;
  DCHECK(render_frame_host_);
  DCHECK(!error_handler.is_null());

  binding_.set_connection_error_handler(error_handler);

  // |interface_factory_ptr_| and |cdm_factory_map_| will be lazily
  // connected in GetMediaInterfaceFactory() and GetCdmFactory().
}

MediaInterfaceProxy::~MediaInterfaceProxy() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());
}

void MediaInterfaceProxy::CreateAudioDecoder(
    media::mojom::AudioDecoderRequest request) {
  DCHECK(thread_checker_.CalledOnValidThread());
  InterfaceFactory* factory = GetMediaInterfaceFactory();
  if (factory)
    factory->CreateAudioDecoder(std::move(request));
}

void MediaInterfaceProxy::CreateVideoDecoder(
    media::mojom::VideoDecoderRequest request) {
  DCHECK(thread_checker_.CalledOnValidThread());
  InterfaceFactory* factory = GetMediaInterfaceFactory();
  if (factory)
    factory->CreateVideoDecoder(std::move(request));
}

void MediaInterfaceProxy::CreateRenderer(
    media::mojom::HostedRendererType type,
    const std::string& type_specific_id,
    media::mojom::RendererRequest request) {
  DCHECK(thread_checker_.CalledOnValidThread());

#if defined(OS_ANDROID)
  if (type == media::mojom::HostedRendererType::kMediaPlayer) {
    CreateMediaPlayerRenderer(std::move(request));
    return;
  }

  if (type == media::mojom::HostedRendererType::kFlinging) {
    std::unique_ptr<FlingingRenderer> renderer =
        FlingingRenderer::Create(render_frame_host_, type_specific_id);

    media::MojoRendererService::Create(
        nullptr, std::move(renderer),
        media::MojoRendererService::InitiateSurfaceRequestCB(),
        std::move(request));
    return;
  }
#endif

  InterfaceFactory* factory = GetMediaInterfaceFactory();
  if (factory)
    factory->CreateRenderer(type, type_specific_id, std::move(request));
}

void MediaInterfaceProxy::CreateCdm(
    const std::string& key_system,
    media::mojom::ContentDecryptionModuleRequest request) {
  DCHECK(thread_checker_.CalledOnValidThread());
#if !BUILDFLAG(ENABLE_LIBRARY_CDMS)
  auto* factory = GetMediaInterfaceFactory();
  if (factory)
    factory->CreateCdm(key_system, std::move(request));
#else
  auto* factory = GetCdmFactory(key_system);
  if (factory)
    factory->CreateCdm(key_system, std::move(request));
#endif
}

void MediaInterfaceProxy::CreateDecryptor(
    int cdm_id,
    media::mojom::DecryptorRequest request) {
  InterfaceFactory* factory = GetMediaInterfaceFactory();
  if (factory)
    factory->CreateDecryptor(cdm_id, std::move(request));
}

void MediaInterfaceProxy::CreateCdmProxy(
    const std::string& cdm_guid,
    media::mojom::CdmProxyRequest request) {
  NOTREACHED() << "The CdmProxy should only be created by a CDM.";
}

service_manager::mojom::InterfaceProviderPtr
MediaInterfaceProxy::GetFrameServices(const std::string& cdm_guid,
                                      const std::string& cdm_file_system_id) {
  // Register frame services.
  service_manager::mojom::InterfaceProviderPtr interfaces;

  // TODO(xhwang): Replace this InterfaceProvider with a dedicated media host
  // interface. See http://crbug.com/660573
  auto provider = std::make_unique<media::MediaInterfaceProvider>(
      mojo::MakeRequest(&interfaces));

#if BUILDFLAG(ENABLE_MOJO_CDM)
  // TODO(slan): Wrap these into a RenderFrame specific ProvisionFetcher impl.
  provider->registry()->AddInterface(base::BindRepeating(
      &ProvisionFetcherImpl::Create,
      base::RetainedRef(
          BrowserContext::GetDefaultStoragePartition(
              render_frame_host_->GetProcess()->GetBrowserContext())
              ->GetURLLoaderFactoryForBrowserProcess())));

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  // Only provide CdmStorageImpl when we have a valid |cdm_file_system_id|,
  // which is currently only set for the CdmService (not the MediaService).
  if (!cdm_file_system_id.empty()) {
    provider->registry()->AddInterface(base::BindRepeating(
        &CdmStorageImpl::Create, render_frame_host_, cdm_file_system_id));
  }

  provider->registry()->AddInterface(
      base::BindRepeating(&MediaInterfaceProxy::CreateCdmProxyInternal,
                          base::Unretained(this), cdm_guid));
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)
#endif  // BUILDFLAG(ENABLE_MOJO_CDM)

  GetContentClient()->browser()->ExposeInterfacesToMediaService(
      provider->registry(), render_frame_host_);

  media_registries_.push_back(std::move(provider));

  return interfaces;
}

media::mojom::InterfaceFactory*
MediaInterfaceProxy::GetMediaInterfaceFactory() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!interface_factory_ptr_)
    ConnectToMediaService();

  return interface_factory_ptr_.get();
}

void MediaInterfaceProxy::ConnectToMediaService() {
  DVLOG(1) << __func__;
  DCHECK(!interface_factory_ptr_);

  media::mojom::MediaServicePtr media_service;

  // TODO(slan): Use the BrowserContext Connector instead. See crbug.com/638950.
  service_manager::Connector* connector =
      ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface(media::mojom::kMediaServiceName, &media_service);

  media_service->CreateInterfaceFactory(
      MakeRequest(&interface_factory_ptr_),
      GetFrameServices(std::string(), std::string()));

  interface_factory_ptr_.set_connection_error_handler(
      base::BindOnce(&MediaInterfaceProxy::OnMediaServiceConnectionError,
                     base::Unretained(this)));
}

void MediaInterfaceProxy::OnMediaServiceConnectionError() {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  interface_factory_ptr_.reset();
}

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)

media::mojom::CdmFactory* MediaInterfaceProxy::GetCdmFactory(
    const std::string& key_system) {
  DCHECK(thread_checker_.CalledOnValidThread());

  std::string cdm_guid;
  base::FilePath cdm_path;
  std::string cdm_file_system_id;

  std::unique_ptr<CdmInfo> cdm_info =
      KeySystemSupportImpl::GetCdmInfoForKeySystem(key_system);
  if (!cdm_info) {
    NOTREACHED() << "No valid CdmInfo for " << key_system;
    return nullptr;
  }
  if (cdm_info->path.empty()) {
    NOTREACHED() << "CDM path for " << key_system << " is empty.";
    return nullptr;
  }
  if (!base::IsValidGUID(cdm_info->guid)) {
    NOTREACHED() << "Invalid CDM GUID " << cdm_info->guid;
    return nullptr;
  }
  if (!CdmStorageImpl::IsValidCdmFileSystemId(cdm_info->file_system_id)) {
    NOTREACHED() << "Invalid file system ID " << cdm_info->file_system_id;
    return nullptr;
  }
  cdm_guid = cdm_info->guid;
  cdm_path = cdm_info->path;
  cdm_file_system_id = cdm_info->file_system_id;

  auto found = cdm_factory_map_.find(cdm_guid);
  if (found != cdm_factory_map_.end())
    return found->second.get();

  return ConnectToCdmService(cdm_guid, cdm_path, cdm_file_system_id);
}

media::mojom::CdmFactory* MediaInterfaceProxy::ConnectToCdmService(
    const std::string& cdm_guid,
    const base::FilePath& cdm_path,
    const std::string& cdm_file_system_id) {
  DVLOG(1) << __func__ << ": cdm_guid = " << cdm_guid;

  DCHECK(!cdm_factory_map_.count(cdm_guid));
  service_manager::Identity identity(media::mojom::kCdmServiceName,
                                     service_manager::mojom::kInheritUserID,
                                     cdm_guid);

  // TODO(slan): Use the BrowserContext Connector instead. See crbug.com/638950.
  service_manager::Connector* connector =
      ServiceManagerConnection::GetForProcess()->GetConnector();

  media::mojom::CdmServicePtr cdm_service;
  connector->BindInterface(identity, &cdm_service);

#if defined(OS_MACOSX)
  // LoadCdm() should always be called before CreateInterfaceFactory().
  media::mojom::SeatbeltExtensionTokenProviderPtr token_provider_ptr;
  mojo::MakeStrongBinding(
      std::make_unique<SeatbeltExtensionTokenProviderImpl>(cdm_path),
      mojo::MakeRequest(&token_provider_ptr));

  cdm_service->LoadCdm(cdm_path, std::move(token_provider_ptr));
#else
  cdm_service->LoadCdm(cdm_path);
#endif  // defined(OS_MACOSX)

  media::mojom::CdmFactoryPtr cdm_factory_ptr;
  cdm_service->CreateCdmFactory(MakeRequest(&cdm_factory_ptr),
                                GetFrameServices(cdm_guid, cdm_file_system_id));
  cdm_factory_ptr.set_connection_error_handler(
      base::BindOnce(&MediaInterfaceProxy::OnCdmServiceConnectionError,
                     base::Unretained(this), cdm_guid));

  auto* cdm_factory = cdm_factory_ptr.get();
  cdm_factory_map_.emplace(cdm_guid, std::move(cdm_factory_ptr));
  return cdm_factory;
}

void MediaInterfaceProxy::OnCdmServiceConnectionError(
    const std::string& cdm_guid) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  DCHECK(cdm_factory_map_.count(cdm_guid));
  cdm_factory_map_.erase(cdm_guid);
}

void MediaInterfaceProxy::CreateCdmProxyInternal(
    const std::string& cdm_guid,
    media::mojom::CdmProxyRequest request) {
  DVLOG(1) << __func__;
  DCHECK(thread_checker_.CalledOnValidThread());

  InterfaceFactory* factory = GetMediaInterfaceFactory();
  if (factory)
    factory->CreateCdmProxy(cdm_guid, std::move(request));
}
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

#if defined(OS_ANDROID)
void MediaInterfaceProxy::CreateMediaPlayerRenderer(
    media::mojom::RendererRequest request) {
  auto renderer = std::make_unique<MediaPlayerRenderer>(
      render_frame_host_->GetProcess()->GetID(),
      render_frame_host_->GetRoutingID(),
      static_cast<RenderFrameHostImpl*>(render_frame_host_)
          ->delegate()
          ->GetAsWebContents());

  // base::Unretained is safe here because the lifetime of the MediaPlayerRender
  // is tied to the lifetime of the MojoRendererService.
  media::MojoRendererService::InitiateSurfaceRequestCB surface_request_cb =
      base::BindRepeating(&MediaPlayerRenderer::InitiateScopedSurfaceRequest,
                          base::Unretained(renderer.get()));

  media::MojoRendererService::Create(nullptr, std::move(renderer),
                                     surface_request_cb, std::move(request));
}
#endif  // defined(OS_ANDROID)

}  // namespace content
