// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_MEDIA_TRANSFER_PROTOCOL_MTP_DEVICE_MANAGER_H_
#define SERVICES_DEVICE_MEDIA_TRANSFER_PROTOCOL_MTP_DEVICE_MANAGER_H_

#include <memory>
#include <string>

#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/device/media_transfer_protocol/media_transfer_protocol_manager.h"
#include "services/device/public/mojom/mtp_manager.mojom.h"

#if !defined(OS_CHROMEOS)
#error "Only used on ChromeOS"
#endif

namespace device {

// This is the implementation of device::mojom::MtpManager which provides
// various methods to get information of MTP (Media Transfer Protocol) devices.
class MtpDeviceManager : public mojom::MtpManager {
 public:
  MtpDeviceManager();
  ~MtpDeviceManager() override;

  void AddBinding(mojom::MtpManagerRequest request);

  // Implements mojom::MtpManager.
  void EnumerateStoragesAndSetClient(
      mojom::MtpManagerClientAssociatedPtrInfo client,
      EnumerateStoragesAndSetClientCallback callback) override;
  void GetStorageInfo(const std::string& storage_name,
                      GetStorageInfoCallback callback) override;
  void GetStorageInfoFromDevice(
      const std::string& storage_name,
      GetStorageInfoFromDeviceCallback callback) override;
  void OpenStorage(const std::string& storage_name,
                   const std::string& mode,
                   OpenStorageCallback callback) override;
  void CloseStorage(const std::string& storage_handle,
                    CloseStorageCallback callback) override;
  void CreateDirectory(const std::string& storage_handle,
                       uint32_t parent_id,
                       const std::string& directory_name,
                       CreateDirectoryCallback callback) override;
  void ReadDirectory(const std::string& storage_handle,
                     uint32_t file_id,
                     uint64_t max_size,
                     ReadDirectoryCallback callback) override;
  void ReadFileChunk(const std::string& storage_handle,
                     uint32_t file_id,
                     uint32_t offset,
                     uint32_t count,
                     ReadFileChunkCallback callback) override;
  void GetFileInfo(const std::string& storage_handle,
                   uint32_t file_id,
                   GetFileInfoCallback callback) override;
  void RenameObject(const std::string& storage_handle,
                    uint32_t object_id,
                    const std::string& new_name,
                    RenameObjectCallback callback) override;
  void CopyFileFromLocal(const std::string& storage_handle,
                         int64_t source_file_descriptor,
                         uint32_t parent_id,
                         const std::string& file_name,
                         CopyFileFromLocalCallback callback) override;
  void DeleteObject(const std::string& storage_handle,
                    uint32_t object_id,
                    DeleteObjectCallback callback) override;

 private:
  std::unique_ptr<MediaTransferProtocolManager>
      media_transfer_protocol_manager_;

  std::unique_ptr<MediaTransferProtocolManager::Observer> observer_;
  mojo::BindingSet<mojom::MtpManager> bindings_;

  DISALLOW_COPY_AND_ASSIGN(MtpDeviceManager);
};

}  // namespace device

#endif  // SERVICES_DEVICE_MEDIA_TRANSFER_PROTOCOL_MTP_DEVICE_MANAGER_H_
