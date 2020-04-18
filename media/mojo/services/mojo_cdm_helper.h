// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_CDM_HELPER_H_
#define MEDIA_MOJO_SERVICES_MOJO_CDM_HELPER_H_

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/cdm/cdm_auxiliary_helper.h"
#include "media/mojo/interfaces/cdm_proxy.mojom.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "media/mojo/interfaces/output_protection.mojom.h"
#include "media/mojo/interfaces/platform_verification.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/mojo/services/mojo_cdm_file_io.h"
#include "media/mojo/services/mojo_cdm_proxy.h"

namespace service_manager {
namespace mojom {
class InterfaceProvider;
}
}  // namespace service_manager

namespace media {

// Helper class that connects the CDM to various auxiliary services. All
// additional services (FileIO, memory allocation, output protection, and
// platform verification) are lazily created.
class MEDIA_MOJO_EXPORT MojoCdmHelper final : public CdmAuxiliaryHelper,
                                              public MojoCdmFileIO::Delegate {
 public:
  explicit MojoCdmHelper(
      service_manager::mojom::InterfaceProvider* interface_provider);
  ~MojoCdmHelper() final;

  // CdmAuxiliaryHelper implementation.
  void SetFileReadCB(FileReadCB file_read_cb) final;
  cdm::FileIO* CreateCdmFileIO(cdm::FileIOClient* client) final;
  cdm::CdmProxy* CreateCdmProxy(cdm::CdmProxyClient* client) final;
  int GetCdmProxyCdmId() final;
  cdm::Buffer* CreateCdmBuffer(size_t capacity) final;
  std::unique_ptr<VideoFrameImpl> CreateCdmVideoFrame() final;
  void QueryStatus(QueryStatusCB callback) final;
  void EnableProtection(uint32_t desired_protection_mask,
                        EnableProtectionCB callback) final;
  void ChallengePlatform(const std::string& service_id,
                         const std::string& challenge,
                         ChallengePlatformCB callback) final;
  void GetStorageId(uint32_t version, StorageIdCB callback) final;

  // MojoCdmFileIO::Delegate implementation.
  void CloseCdmFileIO(MojoCdmFileIO* cdm_file_io) final;
  void ReportFileReadSize(int file_size_bytes) final;

 private:
  // All services are created lazily.
  void ConnectToCdmStorage();
  CdmAllocator* GetAllocator();
  void ConnectToOutputProtection();
  void ConnectToPlatformVerification();

  // Provides interfaces when needed.
  service_manager::mojom::InterfaceProvider* interface_provider_;

  // Connections to the additional services. For the mojom classes, if a
  // connection error occurs, we will not be able to reconnect to the
  // service as the document has been destroyed (see FrameServiceBase) or
  // the browser crashed, so there's no point in trying to reconnect.
  mojom::CdmStoragePtr cdm_storage_ptr_;
  std::unique_ptr<CdmAllocator> allocator_;
  mojom::OutputProtectionPtr output_protection_ptr_;
  mojom::PlatformVerificationPtr platform_verification_ptr_;

  FileReadCB file_read_cb_;

  // A list of open cdm::FileIO objects.
  // TODO(xhwang): Switch to use UniquePtrComparator.
  std::vector<std::unique_ptr<MojoCdmFileIO>> cdm_file_io_set_;

  std::unique_ptr<MojoCdmProxy> cdm_proxy_;

  base::WeakPtrFactory<MojoCdmHelper> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(MojoCdmHelper);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_CDM_HELPER_H_
