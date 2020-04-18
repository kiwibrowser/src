// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_SMB_PROVIDER_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_SMB_PROVIDER_CLIENT_H_

#include "chromeos/dbus/smb_provider_client.h"

namespace chromeos {

// A fake implementation of SmbProviderClient.
class CHROMEOS_EXPORT FakeSmbProviderClient : public SmbProviderClient {
 public:
  FakeSmbProviderClient();
  ~FakeSmbProviderClient() override;

  // DBusClient override.
  void Init(dbus::Bus* bus) override;

  // SmbProviderClient override.
  void Mount(const base::FilePath& share_path,
             const std::string& workgroup,
             const std::string& username,
             base::ScopedFD password_fd,
             MountCallback callback) override;

  void Remount(const base::FilePath& share_path,
               int32_t mount_id,
               StatusCallback callback) override;
  void Unmount(int32_t mount_id, StatusCallback callback) override;
  void ReadDirectory(int32_t mount_id,
                     const base::FilePath& directory_path,
                     ReadDirectoryCallback callback) override;
  void GetMetadataEntry(int32_t mount_id,
                        const base::FilePath& entry_path,
                        GetMetdataEntryCallback callback) override;
  void OpenFile(int32_t mount_id,
                const base::FilePath& file_path,
                bool writeable,
                OpenFileCallback callback) override;
  void CloseFile(int32_t mount_id,
                 int32_t file_id,
                 StatusCallback callback) override;
  void ReadFile(int32_t mount_id,
                int32_t file_id,
                int64_t offset,
                int32_t length,
                ReadFileCallback callback) override;

  void DeleteEntry(int32_t mount_id,
                   const base::FilePath& entry_path,
                   bool recursive,
                   StatusCallback callback) override;

  void CreateFile(int32_t mount_id,
                  const base::FilePath& file_path,
                  StatusCallback callback) override;

  void Truncate(int32_t mount_id,
                const base::FilePath& file_path,
                int64_t length,
                StatusCallback callback) override;

  void WriteFile(int32_t mount_id,
                 int32_t file_id,
                 int64_t offset,
                 int32_t length,
                 base::ScopedFD temp_fd,
                 StatusCallback callback) override;

  void CreateDirectory(int32_t mount_id,
                       const base::FilePath& directory_path,
                       bool recursive,
                       StatusCallback callback) override;

  void MoveEntry(int32_t mount_id,
                 const base::FilePath& source_path,
                 const base::FilePath& target_path,
                 StatusCallback callback) override;

  void CopyEntry(int32_t mount_id,
                 const base::FilePath& source_path,
                 const base::FilePath& target_path,
                 StatusCallback callback) override;

  void GetDeleteList(int32_t mount_id,
                     const base::FilePath& entry_path,
                     GetDeleteListCallback callback) override;

  void GetShares(const base::FilePath& server_url,
                 ReadDirectoryCallback callback) override;

  void SetupKerberos(const std::string& account_id,
                     SetupKerberosCallback callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeSmbProviderClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_SMB_PROVIDER_CLIENT_H_
