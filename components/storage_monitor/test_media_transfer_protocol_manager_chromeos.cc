// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/storage_monitor/test_media_transfer_protocol_manager_chromeos.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/no_destructor.h"

namespace storage_monitor {

// static
TestMediaTransferProtocolManagerChromeOS*
TestMediaTransferProtocolManagerChromeOS::GetFakeMtpManager() {
  static base::NoDestructor<TestMediaTransferProtocolManagerChromeOS>
      fake_mtp_manager;
  return fake_mtp_manager.get();
}

TestMediaTransferProtocolManagerChromeOS::
    TestMediaTransferProtocolManagerChromeOS() {}

TestMediaTransferProtocolManagerChromeOS::
    ~TestMediaTransferProtocolManagerChromeOS() {}

void TestMediaTransferProtocolManagerChromeOS::AddBinding(
    device::mojom::MtpManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void TestMediaTransferProtocolManagerChromeOS::EnumerateStoragesAndSetClient(
    device::mojom::MtpManagerClientAssociatedPtrInfo client,
    EnumerateStoragesAndSetClientCallback callback) {
  std::move(callback).Run(std::vector<device::mojom::MtpStorageInfoPtr>());
}

void TestMediaTransferProtocolManagerChromeOS::GetStorageInfo(
    const std::string& storage_name,
    GetStorageInfoCallback callback) {
  std::move(callback).Run(nullptr);
}

void TestMediaTransferProtocolManagerChromeOS::GetStorageInfoFromDevice(
    const std::string& storage_name,
    GetStorageInfoFromDeviceCallback callback) {
  std::move(callback).Run(device::mojom::MtpStorageInfo::New(),
                          true /* error */);
}

void TestMediaTransferProtocolManagerChromeOS::OpenStorage(
    const std::string& storage_name,
    const std::string& mode,
    OpenStorageCallback callback) {
  std::move(callback).Run("", true);
}

void TestMediaTransferProtocolManagerChromeOS::CloseStorage(
    const std::string& storage_handle,
    CloseStorageCallback callback) {
  std::move(callback).Run(true);
}

void TestMediaTransferProtocolManagerChromeOS::CreateDirectory(
    const std::string& storage_handle,
    uint32_t parent_id,
    const std::string& directory_name,
    CreateDirectoryCallback callback) {
  std::move(callback).Run(true /* error */);
}

void TestMediaTransferProtocolManagerChromeOS::ReadDirectory(
    const std::string& storage_handle,
    uint32_t file_id,
    uint64_t max_size,
    ReadDirectoryCallback callback) {
  std::move(callback).Run(std::vector<device::mojom::MtpFileEntryPtr>(),
                          false /* no more entries*/, true /* error */);
}

void TestMediaTransferProtocolManagerChromeOS::ReadFileChunk(
    const std::string& storage_handle,
    uint32_t file_id,
    uint32_t offset,
    uint32_t count,
    ReadFileChunkCallback callback) {
  std::move(callback).Run(std::string(), true);
}

void TestMediaTransferProtocolManagerChromeOS::GetFileInfo(
    const std::string& storage_handle,
    uint32_t file_id,
    GetFileInfoCallback callback) {
  std::move(callback).Run(device::mojom::MtpFileEntry::New(), true);
}

void TestMediaTransferProtocolManagerChromeOS::RenameObject(
    const std::string& storage_handle,
    uint32_t object_id,
    const std::string& new_name,
    RenameObjectCallback callback) {
  std::move(callback).Run(true /* error */);
}

void TestMediaTransferProtocolManagerChromeOS::CopyFileFromLocal(
    const std::string& storage_handle,
    int64_t source_file_descriptor,
    uint32_t parent_id,
    const std::string& file_name,
    CopyFileFromLocalCallback callback) {
  std::move(callback).Run(true /* error */);
}

void TestMediaTransferProtocolManagerChromeOS::DeleteObject(
    const std::string& storage_handle,
    uint32_t object_id,
    DeleteObjectCallback callback) {
  std::move(callback).Run(true /* error */);
}

}  // namespace storage_monitor
