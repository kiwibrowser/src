// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/smb_provider_client.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "base/memory/weak_ptr.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

namespace chromeos {

namespace {

smbprovider::ErrorType GetErrorFromReader(dbus::MessageReader* reader) {
  int32_t int_error;
  if (!reader->PopInt32(&int_error) ||
      !smbprovider::ErrorType_IsValid(int_error)) {
    DLOG(ERROR)
        << "SmbProviderClient: Failed to get an error from the response";
    return smbprovider::ERROR_DBUS_PARSE_FAILED;
  }
  return static_cast<smbprovider::ErrorType>(int_error);
}

smbprovider::ErrorType GetErrorAndProto(
    dbus::Response* response,
    google::protobuf::MessageLite* protobuf_out) {
  if (!response) {
    DLOG(ERROR) << "Failed to call smbprovider";
    return smbprovider::ERROR_DBUS_PARSE_FAILED;
  }
  dbus::MessageReader reader(response);
  smbprovider::ErrorType error = GetErrorFromReader(&reader);
  if (error != smbprovider::ERROR_OK) {
    return error;
  }
  if (!reader.PopArrayOfBytesAsProto(protobuf_out)) {
    DLOG(ERROR) << "Failed to parse protobuf.";
    return smbprovider::ERROR_DBUS_PARSE_FAILED;
  }
  return smbprovider::ERROR_OK;
}

bool ParseDeleteList(const base::ScopedFD& fd,
                     int32_t bytes_written,
                     smbprovider::DeleteListProto* delete_list) {
  DCHECK(delete_list);
  std::vector<uint8_t> buffer(bytes_written);
  return base::ReadFromFD(fd.get(), reinterpret_cast<char*>(buffer.data()),
                          buffer.size()) &&
         delete_list->ParseFromArray(buffer.data(), buffer.size());
}

class SmbProviderClientImpl : public SmbProviderClient {
 public:
  SmbProviderClientImpl() = default;

  ~SmbProviderClientImpl() override {}

  void Mount(const base::FilePath& share_path,
             const std::string& workgroup,
             const std::string& username,
             base::ScopedFD password_fd,
             MountCallback callback) override {
    smbprovider::MountOptionsProto options;
    options.set_path(share_path.value());
    options.set_workgroup(workgroup);
    options.set_username(username);

    dbus::MethodCall method_call(smbprovider::kSmbProviderInterface,
                                 smbprovider::kMountMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendProtoAsArrayOfBytes(options);
    writer.AppendFileDescriptor(password_fd.release());
    CallMethod(&method_call, &SmbProviderClientImpl::HandleMountCallback,
               &callback);
  }

  void Remount(const base::FilePath& share_path,
               int32_t mount_id,
               StatusCallback callback) override {
    smbprovider::RemountOptionsProto options;
    options.set_path(share_path.value());
    options.set_mount_id(mount_id);
    CallDefaultMethod(smbprovider::kRemountMethod, options, &callback);
  }

  void Unmount(int32_t mount_id, StatusCallback callback) override {
    smbprovider::UnmountOptionsProto options;
    options.set_mount_id(mount_id);
    CallDefaultMethod(smbprovider::kUnmountMethod, options, &callback);
  }

  void ReadDirectory(int32_t mount_id,
                     const base::FilePath& directory_path,
                     ReadDirectoryCallback callback) override {
    smbprovider::ReadDirectoryOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_directory_path(directory_path.value());
    CallMethod(smbprovider::kReadDirectoryMethod, options,
               &SmbProviderClientImpl::HandleProtoCallback<
                   smbprovider::DirectoryEntryListProto>,
               &callback);
  }

  void GetMetadataEntry(int32_t mount_id,
                        const base::FilePath& entry_path,
                        GetMetdataEntryCallback callback) override {
    smbprovider::GetMetadataEntryOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_entry_path(entry_path.value());
    CallMethod(smbprovider::kGetMetadataEntryMethod, options,
               &SmbProviderClientImpl::HandleProtoCallback<
                   smbprovider::DirectoryEntryProto>,
               &callback);
  }

  void OpenFile(int32_t mount_id,
                const base::FilePath& file_path,
                bool writeable,
                OpenFileCallback callback) override {
    smbprovider::OpenFileOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_file_path(file_path.value());
    options.set_writeable(writeable);
    CallMethod(smbprovider::kOpenFileMethod, options,
               &SmbProviderClientImpl::HandleOpenFileCallback, &callback);
  }

  void CloseFile(int32_t mount_id,
                 int32_t file_id,
                 StatusCallback callback) override {
    smbprovider::CloseFileOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_file_id(file_id);
    CallDefaultMethod(smbprovider::kCloseFileMethod, options, &callback);
  }

  void ReadFile(int32_t mount_id,
                int32_t file_id,
                int64_t offset,
                int32_t length,
                ReadFileCallback callback) override {
    smbprovider::ReadFileOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_file_id(file_id);
    options.set_offset(offset);
    options.set_length(length);
    CallMethod(smbprovider::kReadFileMethod, options,
               &SmbProviderClientImpl::HandleReadFileCallback, &callback);
  }

  void DeleteEntry(int32_t mount_id,
                   const base::FilePath& entry_path,
                   bool recursive,
                   StatusCallback callback) override {
    smbprovider::DeleteEntryOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_entry_path(entry_path.value());
    options.set_recursive(recursive);
    CallDefaultMethod(smbprovider::kDeleteEntryMethod, options, &callback);
  }

  void CreateFile(int32_t mount_id,
                  const base::FilePath& file_path,
                  StatusCallback callback) override {
    smbprovider::CreateFileOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_file_path(file_path.value());
    CallDefaultMethod(smbprovider::kCreateFileMethod, options, &callback);
  }

  void Truncate(int32_t mount_id,
                const base::FilePath& file_path,
                int64_t length,
                StatusCallback callback) override {
    smbprovider::TruncateOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_file_path(file_path.value());
    options.set_length(length);
    CallDefaultMethod(smbprovider::kTruncateMethod, options, &callback);
  }

  void WriteFile(int32_t mount_id,
                 int32_t file_id,
                 int64_t offset,
                 int32_t length,
                 base::ScopedFD temp_fd,
                 StatusCallback callback) override {
    smbprovider::WriteFileOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_file_id(file_id);
    options.set_offset(offset);
    options.set_length(length);

    dbus::MethodCall method_call(smbprovider::kSmbProviderInterface,
                                 smbprovider::kWriteFileMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendProtoAsArrayOfBytes(options);
    writer.AppendFileDescriptor(temp_fd.release());
    CallDefaultMethod(&method_call, &callback);
  }

  void CreateDirectory(int32_t mount_id,
                       const base::FilePath& directory_path,
                       bool recursive,
                       StatusCallback callback) override {
    smbprovider::CreateDirectoryOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_directory_path(directory_path.value());
    options.set_recursive(recursive);
    CallDefaultMethod(smbprovider::kCreateDirectoryMethod, options, &callback);
  }

  void MoveEntry(int32_t mount_id,
                 const base::FilePath& source_path,
                 const base::FilePath& target_path,
                 StatusCallback callback) override {
    smbprovider::MoveEntryOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_source_path(source_path.value());
    options.set_target_path(target_path.value());
    CallDefaultMethod(smbprovider::kMoveEntryMethod, options, &callback);
  }

  void CopyEntry(int32_t mount_id,
                 const base::FilePath& source_path,
                 const base::FilePath& target_path,
                 StatusCallback callback) override {
    smbprovider::CopyEntryOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_source_path(source_path.value());
    options.set_target_path(target_path.value());
    CallCopyEntryMethod(options, std::move(callback));
  }

  void GetDeleteList(int32_t mount_id,
                     const base::FilePath& entry_path,
                     GetDeleteListCallback callback) override {
    smbprovider::GetDeleteListOptionsProto options;
    options.set_mount_id(mount_id);
    options.set_entry_path(entry_path.value());
    CallMethod(smbprovider::kGetDeleteListMethod, options,
               &SmbProviderClientImpl::HandleGetDeleteListCallback, &callback);
  }

  void GetShares(const base::FilePath& server_url,
                 ReadDirectoryCallback callback) override {
    smbprovider::GetSharesOptionsProto options;
    options.set_server_url(server_url.value());
    CallMethod(smbprovider::kGetSharesMethod, options,
               &SmbProviderClientImpl::HandleProtoCallback<
                   smbprovider::DirectoryEntryListProto>,
               &callback);
  }

  void SetupKerberos(const std::string& account_id,
                     SetupKerberosCallback callback) override {
    dbus::MethodCall method_call(smbprovider::kSmbProviderInterface,
                                 smbprovider::kSetupKerberosMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(account_id);
    CallMethod(&method_call,
               &SmbProviderClientImpl::HandleSetupKerberosCallback, &callback);
  }

 protected:
  // DBusClient override.
  void Init(dbus::Bus* bus) override {
    proxy_ = bus->GetObjectProxy(
        smbprovider::kSmbProviderServiceName,
        dbus::ObjectPath(smbprovider::kSmbProviderServicePath));
    DCHECK(proxy_);
  }

 private:
  // Calls the DBUS method |name|, passing the |protobuf| as an argument.
  // |handler| is the member function in this class that receives
  // the response and then passes the processed response to |callback|.
  template <typename CallbackHandler, typename Callback>
  void CallMethod(const char* name,
                  const google::protobuf::MessageLite& protobuf,
                  CallbackHandler handler,
                  Callback callback) {
    dbus::MethodCall method_call(smbprovider::kSmbProviderInterface, name);
    dbus::MessageWriter writer(&method_call);
    writer.AppendProtoAsArrayOfBytes(protobuf);
    CallMethod(&method_call, handler, callback);
  }

  // Calls the method specified in |method_call|. |handler| is the member
  // function in this class that receives the response and then passes the
  // processed response to |callback|.
  template <typename CallbackHandler, typename Callback>
  void CallMethod(dbus::MethodCall* method_call,
                  CallbackHandler handler,
                  Callback callback) {
    proxy_->CallMethod(
        method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(handler, GetWeakPtr(), std::move(*callback)));
  }

  // Calls the D-Bus method |name|, passing the |protobuf| as an argument.
  // Uses the default callback handler to process |callback|.
  template <typename Callback>
  void CallDefaultMethod(const char* name,
                         const google::protobuf::MessageLite& protobuf,
                         Callback callback) {
    dbus::MethodCall method_call(smbprovider::kSmbProviderInterface, name);
    dbus::MessageWriter writer(&method_call);
    writer.AppendProtoAsArrayOfBytes(protobuf);
    CallDefaultMethod(&method_call, callback);
  }

  // Calls the method specified in |method_call|. Uses the default callback
  // handler to process |callback|.
  template <typename Callback>
  void CallDefaultMethod(dbus::MethodCall* method_call, Callback callback) {
    proxy_->CallMethod(
        method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SmbProviderClientImpl::HandleDefaultCallback,
                       GetWeakPtr(), method_call->GetMember(),
                       std::move(*callback)));
  }

  // Calls the CopyEntry D-Bus method with no timeout, passing the |protobuf| as
  // an argument. Uses the default callback handler to process |callback|.
  void CallCopyEntryMethod(const google::protobuf::MessageLite& protobuf,
                           StatusCallback callback) {
    dbus::MethodCall method_call(smbprovider::kSmbProviderInterface,
                                 smbprovider::kCopyEntryMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendProtoAsArrayOfBytes(protobuf);
    proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_INFINITE,
        base::BindOnce(&SmbProviderClientImpl::HandleDefaultCallback,
                       GetWeakPtr(), method_call.GetMember(),
                       std::move(callback)));
  }

  // Handles D-Bus callback for mount.
  void HandleMountCallback(MountCallback callback, dbus::Response* response) {
    if (!response) {
      LOG(ERROR) << "Mount: failed to call smbprovider";
      std::move(callback).Run(smbprovider::ERROR_DBUS_PARSE_FAILED, -1);
      return;
    }
    dbus::MessageReader reader(response);
    smbprovider::ErrorType error = GetErrorFromReader(&reader);
    if (error != smbprovider::ERROR_OK) {
      std::move(callback).Run(error, -1);
      return;
    }
    int32_t mount_id = -1;
    if (!reader.PopInt32(&mount_id) || mount_id < 0) {
      LOG(ERROR) << "Mount: failed to parse mount id";
      std::move(callback).Run(smbprovider::ERROR_DBUS_PARSE_FAILED, -1);
      return;
    }
    std::move(callback).Run(smbprovider::ERROR_OK, mount_id);
  }

  // Handles D-Bus callback for OpenFile.
  void HandleOpenFileCallback(OpenFileCallback callback,
                              dbus::Response* response) {
    if (!response) {
      LOG(ERROR) << "OpenFile: failed to call smbprovider";
      std::move(callback).Run(smbprovider::ERROR_DBUS_PARSE_FAILED, -1);
      return;
    }
    dbus::MessageReader reader(response);
    smbprovider::ErrorType error = GetErrorFromReader(&reader);
    if (error != smbprovider::ERROR_OK) {
      std::move(callback).Run(error, -1);
      return;
    }
    int32_t file_id = -1;
    if (!reader.PopInt32(&file_id) || file_id < 0) {
      LOG(ERROR) << "OpenFile: failed to parse mount id";
      std::move(callback).Run(smbprovider::ERROR_DBUS_PARSE_FAILED, -1);
      return;
    }
    std::move(callback).Run(smbprovider::ERROR_OK, file_id);
  }

  // Handles D-Bus callback for ReadFile.
  void HandleReadFileCallback(ReadFileCallback callback,
                              dbus::Response* response) {
    base::ScopedFD fd;
    if (!response) {
      LOG(ERROR) << "ReadFile: failed to call smbprovider";
      std::move(callback).Run(smbprovider::ERROR_DBUS_PARSE_FAILED, fd);
      return;
    }
    dbus::MessageReader reader(response);
    smbprovider::ErrorType error = GetErrorFromReader(&reader);
    if (error != smbprovider::ERROR_OK) {
      std::move(callback).Run(error, fd);
      return;
    }
    if (!reader.PopFileDescriptor(&fd)) {
      LOG(ERROR) << "ReadFile: failed to parse file descriptor";
      std::move(callback).Run(smbprovider::ERROR_DBUS_PARSE_FAILED, fd);
      return;
    }
    std::move(callback).Run(smbprovider::ERROR_OK, fd);
  }

  // Handles D-Bus callback for GetDeleteList.
  void HandleGetDeleteListCallback(GetDeleteListCallback callback,
                                   dbus::Response* response) {
    base::ScopedFD fd;
    smbprovider::DeleteListProto delete_list;
    if (!response) {
      LOG(ERROR) << "GetDeleteList: failed to call smbprovider";
      std::move(callback).Run(smbprovider::ERROR_DBUS_PARSE_FAILED,
                              delete_list);
      return;
    }

    dbus::MessageReader reader(response);
    smbprovider::ErrorType error = GetErrorFromReader(&reader);
    if (error != smbprovider::ERROR_OK) {
      std::move(callback).Run(error, delete_list);
      return;
    }

    int32_t bytes_written;
    bool success = reader.PopFileDescriptor(&fd) &&
                   reader.PopInt32(&bytes_written) &&
                   ParseDeleteList(fd, bytes_written, &delete_list);
    if (!success) {
      LOG(ERROR) << "GetDeleteList: parse failure.";
      std::move(callback).Run(smbprovider::ERROR_DBUS_PARSE_FAILED,
                              delete_list);
      return;
    }

    std::move(callback).Run(smbprovider::ERROR_OK, delete_list);
  }

  // Handles D-Bus callback for SetupKerberos.
  void HandleSetupKerberosCallback(SetupKerberosCallback callback,
                                   dbus::Response* response) {
    dbus::MessageReader reader(response);
    bool result;
    if (!reader.PopBool(&result)) {
      LOG(ERROR) << "SetupKerberos: parse failure.";
      std::move(callback).Run(false /* success */);
    }

    std::move(callback).Run(result);
  }

  // Default callback handler for D-Bus calls.
  void HandleDefaultCallback(const std::string& method_name,
                             StatusCallback callback,
                             dbus::Response* response) {
    if (!response) {
      LOG(ERROR) << method_name << ": failed to call smbprovider";
      std::move(callback).Run(smbprovider::ERROR_DBUS_PARSE_FAILED);
      return;
    }
    dbus::MessageReader reader(response);
    std::move(callback).Run(GetErrorFromReader(&reader));
  }

  // Handles D-Bus responses for methods that return an error and a protobuf
  // object.
  template <class T>
  void HandleProtoCallback(base::OnceCallback<void(smbprovider::ErrorType error,
                                                   const T& response)> callback,
                           dbus::Response* response) {
    T proto;
    smbprovider::ErrorType error(GetErrorAndProto(response, &proto));
    std::move(callback).Run(error, proto);
  }

  base::WeakPtr<SmbProviderClientImpl> GetWeakPtr() {
    return base::AsWeakPtr(this);
  }

  dbus::ObjectProxy* proxy_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(SmbProviderClientImpl);
};

}  // namespace

SmbProviderClient::SmbProviderClient() = default;

SmbProviderClient::~SmbProviderClient() = default;

// static
SmbProviderClient* SmbProviderClient::Create() {
  return new SmbProviderClientImpl();
}

}  // namespace chromeos
