// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_smb_provider_client.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"

namespace chromeos {

FakeSmbProviderClient::FakeSmbProviderClient() {}

FakeSmbProviderClient::~FakeSmbProviderClient() {}

void FakeSmbProviderClient::Init(dbus::Bus* bus) {}

void FakeSmbProviderClient::Mount(const base::FilePath& share_path,
                                  const std::string& workgroup,
                                  const std::string& username,
                                  base::ScopedFD password_fd,
                                  MountCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK, 1));
}

void FakeSmbProviderClient::Remount(const base::FilePath& share_path,
                                    int32_t mount_id,
                                    StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::Unmount(int32_t mount_id, StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::ReadDirectory(int32_t mount_id,
                                          const base::FilePath& directory_path,
                                          ReadDirectoryCallback callback) {
  smbprovider::DirectoryEntryListProto entry_list;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), smbprovider::ERROR_OK, entry_list));
}

void FakeSmbProviderClient::GetMetadataEntry(int32_t mount_id,
                                             const base::FilePath& entry_path,
                                             GetMetdataEntryCallback callback) {
  smbprovider::DirectoryEntryProto entry;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), smbprovider::ERROR_OK, entry));
}

void FakeSmbProviderClient::OpenFile(int32_t mount_id,
                                     const base::FilePath& file_path,
                                     bool writeable,
                                     OpenFileCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK, 1));
}

void FakeSmbProviderClient::CloseFile(int32_t mount_id,
                                      int32_t file_id,
                                      StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::ReadFile(int32_t mount_id,
                                     int32_t file_id,
                                     int64_t offset,
                                     int32_t length,
                                     ReadFileCallback callback) {
  base::ScopedFD fd;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK,
                                std::move(fd)));
}

void FakeSmbProviderClient::DeleteEntry(int32_t mount_id,
                                        const base::FilePath& entry_path,
                                        bool recursive,
                                        StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::CreateFile(int32_t mount_id,
                                       const base::FilePath& file_path,
                                       StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::Truncate(int32_t mount_id,
                                     const base::FilePath& file_path,
                                     int64_t length,
                                     StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::WriteFile(int32_t mount_id,
                                      int32_t file_id,
                                      int64_t offset,
                                      int32_t length,
                                      base::ScopedFD temp_fd,
                                      StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::CreateDirectory(
    int32_t mount_id,
    const base::FilePath& directory_path,
    bool recursive,
    StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::MoveEntry(int32_t mount_id,
                                      const base::FilePath& source_path,
                                      const base::FilePath& target_path,
                                      StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::CopyEntry(int32_t mount_id,
                                      const base::FilePath& source_path,
                                      const base::FilePath& target_path,
                                      StatusCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), smbprovider::ERROR_OK));
}

void FakeSmbProviderClient::GetDeleteList(int32_t mount_id,
                                          const base::FilePath& entry_path,
                                          GetDeleteListCallback callback) {
  smbprovider::DeleteListProto delete_list;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), smbprovider::ERROR_OK, delete_list));
}

void FakeSmbProviderClient::GetShares(const base::FilePath& server_url,
                                      ReadDirectoryCallback callback) {
  smbprovider::DirectoryEntryListProto entry_list;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(callback), smbprovider::ERROR_OK, entry_list));
}

void FakeSmbProviderClient::SetupKerberos(const std::string& account_id,
                                          SetupKerberosCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), true /* success */));
}

}  // namespace chromeos
