// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/media_transfer_protocol/mtp_device_manager.h"

#include <utility>
#include <vector>

namespace device {

namespace {

class ForwardingMediaTransferProtocolManagerObserver
    : public MediaTransferProtocolManager::Observer {
 public:
  explicit ForwardingMediaTransferProtocolManagerObserver(
      mojom::MtpManagerClientAssociatedPtr client)
      : client_(std::move(client)) {}

  void StorageAttached(
      const device::mojom::MtpStorageInfo& storage_info) override {
    client_->StorageAttached(storage_info.Clone());
  }

  void StorageDetached(const std::string& storage_name) override {
    client_->StorageDetached(storage_name);
  }

 private:
  mojom::MtpManagerClientAssociatedPtr client_;
};

void EnumerateStorageCallbackWrapper(
    mojom::MtpManager::EnumerateStoragesAndSetClientCallback callback,
    std::vector<const mojom::MtpStorageInfo*> storage_info_list) {
  std::vector<mojom::MtpStorageInfoPtr> storage_info_ptr_list(
      storage_info_list.size());
  for (size_t i = 0; i < storage_info_list.size(); ++i)
    storage_info_ptr_list[i] = storage_info_list[i]->Clone();
  std::move(callback).Run(std::move(storage_info_ptr_list));
}

void ReadDirectoryCallbackWrapper(
    mojom::MtpManager::ReadDirectoryCallback callback,
    const std::vector<mojom::MtpFileEntry>& file_entries,
    bool has_more,
    bool error) {
  std::vector<mojom::MtpFileEntryPtr> files(file_entries.size());
  for (size_t i = 0; i < file_entries.size(); ++i)
    files[i] = file_entries[i].Clone();
  std::move(callback).Run(std::move(files), has_more, error);
}

}  // namespace

MtpDeviceManager::MtpDeviceManager()
    : media_transfer_protocol_manager_(
          MediaTransferProtocolManager::Initialize()) {}

MtpDeviceManager::~MtpDeviceManager() {}

void MtpDeviceManager::AddBinding(mojom::MtpManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void MtpDeviceManager::EnumerateStoragesAndSetClient(
    mojom::MtpManagerClientAssociatedPtrInfo client,
    EnumerateStoragesAndSetClientCallback callback) {
  if (!client.is_valid())
    return;

  DCHECK(!observer_);
  mojom::MtpManagerClientAssociatedPtr client_ptr;
  client_ptr.Bind(std::move(client));
  observer_ = std::make_unique<ForwardingMediaTransferProtocolManagerObserver>(
      std::move(client_ptr));
  media_transfer_protocol_manager_->AddObserverAndEnumerateStorages(
      observer_.get(),
      base::BindOnce(EnumerateStorageCallbackWrapper, std::move(callback)));
}

void MtpDeviceManager::GetStorageInfo(const std::string& storage_name,
                                      GetStorageInfoCallback callback) {
  media_transfer_protocol_manager_->GetStorageInfo(storage_name,
                                                   std::move(callback));
}

void MtpDeviceManager::GetStorageInfoFromDevice(
    const std::string& storage_name,
    GetStorageInfoFromDeviceCallback callback) {
  media_transfer_protocol_manager_->GetStorageInfoFromDevice(
      storage_name, std::move(callback));
}

void MtpDeviceManager::OpenStorage(const std::string& storage_name,
                                   const std::string& mode,
                                   OpenStorageCallback callback) {
  media_transfer_protocol_manager_->OpenStorage(
      storage_name, mode, base::AdaptCallbackForRepeating(std::move(callback)));
}

void MtpDeviceManager::CloseStorage(const std::string& storage_handle,
                                    CloseStorageCallback callback) {
  media_transfer_protocol_manager_->CloseStorage(
      storage_handle, base::AdaptCallbackForRepeating(std::move(callback)));
}

void MtpDeviceManager::CreateDirectory(const std::string& storage_handle,
                                       uint32_t parent_id,
                                       const std::string& directory_name,
                                       CreateDirectoryCallback callback) {
  media_transfer_protocol_manager_->CreateDirectory(
      storage_handle, parent_id, directory_name,
      base::AdaptCallbackForRepeating(std::move(callback)));
}

void MtpDeviceManager::ReadDirectory(const std::string& storage_handle,
                                     uint32_t file_id,
                                     uint64_t max_size,
                                     ReadDirectoryCallback callback) {
  media_transfer_protocol_manager_->ReadDirectory(
      storage_handle, file_id, max_size,
      base::Bind(ReadDirectoryCallbackWrapper,
                 base::AdaptCallbackForRepeating(std::move(callback))));
}

void MtpDeviceManager::ReadFileChunk(const std::string& storage_handle,
                                     uint32_t file_id,
                                     uint32_t offset,
                                     uint32_t count,
                                     ReadFileChunkCallback callback) {
  media_transfer_protocol_manager_->ReadFileChunk(
      storage_handle, file_id, offset, count,
      base::AdaptCallbackForRepeating(std::move(callback)));
}

void MtpDeviceManager::GetFileInfo(const std::string& storage_handle,
                                   uint32_t file_id,
                                   GetFileInfoCallback callback) {
  media_transfer_protocol_manager_->GetFileInfo(storage_handle, file_id,
                                                std::move(callback));
}

void MtpDeviceManager::RenameObject(const std::string& storage_handle,
                                    uint32_t object_id,
                                    const std::string& new_name,
                                    RenameObjectCallback callback) {
  media_transfer_protocol_manager_->RenameObject(
      storage_handle, object_id, new_name,
      base::AdaptCallbackForRepeating(std::move(callback)));
}

void MtpDeviceManager::CopyFileFromLocal(const std::string& storage_handle,
                                         int64_t source_file_descriptor,
                                         uint32_t parent_id,
                                         const std::string& file_name,
                                         CopyFileFromLocalCallback callback) {
  media_transfer_protocol_manager_->CopyFileFromLocal(
      storage_handle, source_file_descriptor, parent_id, file_name,
      base::AdaptCallbackForRepeating(std::move(callback)));
}

void MtpDeviceManager::DeleteObject(const std::string& storage_handle,
                                    uint32_t object_id,
                                    DeleteObjectCallback callback) {
  media_transfer_protocol_manager_->DeleteObject(
      storage_handle, object_id,
      base::AdaptCallbackForRepeating(std::move(callback)));
}

}  // namespace device
