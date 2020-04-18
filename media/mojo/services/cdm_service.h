// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_CDM_SERVICE_H_
#define MEDIA_MOJO_SERVICES_CDM_SERVICE_H_

#include <memory>

#include "build/build_config.h"
#include "media/media_buildflags.h"
#include "media/mojo/interfaces/cdm_service.mojom.h"
#include "media/mojo/interfaces/content_decryption_module.mojom.h"
#include "media/mojo/services/deferred_destroy_strong_binding_set.h"
#include "media/mojo/services/media_mojo_export.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

#if BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)
#include "media/cdm/cdm_host_file.h"
#endif

namespace media {

class CdmFactory;

class MEDIA_MOJO_EXPORT CdmService : public service_manager::Service,
                                     public mojom::CdmService {
 public:
  class Client {
   public:
    virtual ~Client() {}

    // Called by the MediaService to ensure the process is sandboxed. It could
    // be a no-op if the process is already sandboxed.
    virtual void EnsureSandboxed() = 0;

    // Returns the CdmFactory to be used by MojoCdmService. |host_interfaces|
    // can be used to request interfaces provided remotely by the host. It may
    // be a nullptr if the host chose not to bind the InterfacePtr.
    virtual std::unique_ptr<CdmFactory> CreateCdmFactory(
        service_manager::mojom::InterfaceProvider* host_interfaces) = 0;

#if BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)
    // Gets a list of CDM host file paths and put them in |cdm_host_file_paths|.
    virtual void AddCdmHostFilePaths(
        std::vector<CdmHostFilePath>* cdm_host_file_paths) = 0;
#endif  // BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)
  };

  explicit CdmService(std::unique_ptr<Client> client);
  ~CdmService() final;

  // By default CdmService release is delayed. Overrides the delay with |delay|.
  // If |delay| is 0, delayed service release will be disabled.
  void SetServiceReleaseDelayForTesting(base::TimeDelta delay) {
    service_release_delay_ = delay;
  }

  size_t BoundCdmFactorySizeForTesting() const {
    return cdm_factory_bindings_.size();
  }

  size_t UnboundCdmFactorySizeForTesting() const {
    return cdm_factory_bindings_.unbound_size();
  }

 private:
  // service_manager::Service implementation.
  void OnStart() final;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;
  bool OnServiceManagerConnectionLost() final;

  void Create(mojom::CdmServiceRequest request);

// mojom::CdmService implementation.
#if defined(OS_MACOSX)
  void LoadCdm(const base::FilePath& cdm_path,
               mojom::SeatbeltExtensionTokenProviderPtr token_provider) final;
#else
  void LoadCdm(const base::FilePath& cdm_path) final;
#endif  // defined(OS_MACOSX)
  void CreateCdmFactory(
      mojom::CdmFactoryRequest request,
      service_manager::mojom::InterfaceProviderPtr host_interfaces) final;

  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  std::unique_ptr<Client> client_;
  std::unique_ptr<CdmFactory> cdm_factory_;
  DeferredDestroyStrongBindingSet<mojom::CdmFactory> cdm_factory_bindings_;
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<mojom::CdmService> bindings_;
  base::TimeDelta service_release_delay_;
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_CDM_SERVICE_H_
