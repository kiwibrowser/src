// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_GALLERIES_CHROMEOS_MTP_DEVICE_TASK_HELPER_H_
#define CHROME_BROWSER_MEDIA_GALLERIES_CHROMEOS_MTP_DEVICE_TASK_HELPER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/media_galleries/fileapi/mtp_device_async_delegate.h"
#include "services/device/public/mojom/mtp_file_entry.mojom.h"
#include "storage/browser/fileapi/async_file_util.h"

class MTPReadFileWorker;
struct SnapshotRequestInfo;

// MTPDeviceTaskHelper dispatches the media transfer protocol (MTP) device
// operation requests (such as GetFileInfo, ReadDirectory, CreateSnapshotFile,
// OpenStorage and CloseStorage) to the MediaTransferProtocolManager.
// MTPDeviceTaskHelper lives on the UI thread. MTPDeviceTaskHelperMapService
// owns the MTPDeviceTaskHelper objects. MTPDeviceTaskHelper is instantiated per
// MTP device storage.
class MTPDeviceTaskHelper {
 public:
  struct MTPEntry {
    MTPEntry();

    uint32_t file_id;
    std::string name;
    base::File::Info file_info;
  };

  typedef std::vector<MTPEntry> MTPEntries;

  typedef base::Callback<void(bool succeeded)> OpenStorageCallback;

  typedef MTPDeviceAsyncDelegate::GetFileInfoSuccessCallback
      GetFileInfoSuccessCallback;

  typedef base::Closure CreateDirectorySuccessCallback;

  typedef base::Callback<void(const MTPEntries& entries, bool has_more)>
      ReadDirectorySuccessCallback;

  typedef base::Closure RenameObjectSuccessCallback;

  typedef base::Closure CopyFileFromLocalSuccessCallback;

  typedef base::Closure DeleteObjectSuccessCallback;

  typedef MTPDeviceAsyncDelegate::ErrorCallback ErrorCallback;

  MTPDeviceTaskHelper();
  ~MTPDeviceTaskHelper();

  // Dispatches the request to the MediaTransferProtocolManager to open the MTP
  // storage for communication.
  //
  // |storage_name| specifies the name of the storage device.
  // |callback| is called when the OpenStorage request completes. |callback|
  // runs on the IO thread.
  void OpenStorage(const std::string& storage_name,
                   const bool read_only,
                   const OpenStorageCallback& callback);

  // Dispatches the GetFileInfo request to the MediaTransferProtocolManager.
  //
  // |file_id| specifies the id of the file whose details are requested.
  //
  // If the file details are fetched successfully, |success_callback| is invoked
  // on the IO thread to notify the caller about the file details.
  //
  // If there is an error, |error_callback| is invoked on the IO thread to
  // notify the caller about the file error.
  void GetFileInfo(uint32_t file_id,
                   const GetFileInfoSuccessCallback& success_callback,
                   const ErrorCallback& error_callback);

  // Forwards CreateDirectory request to the MediaTransferProtocolManager.
  void CreateDirectory(const uint32_t parent_id,
                       const std::string& directory_name,
                       const CreateDirectorySuccessCallback& success_callback,
                       const ErrorCallback& error_callback);

  // Dispatches the read directory request to the MediaTransferProtocolManager.
  //
  // |dir_id| specifies the directory id.
  //
  // If the directory file entries are enumerated successfully,
  // |success_callback| is invoked on the IO thread to notify the caller about
  // the directory file entries. Please see the note in the
  // ReadDirectorySuccessCallback typedef regarding the special treatment of
  // file names.
  //
  // If there is an error, |error_callback| is invoked on the IO thread to
  // notify the caller about the file error.
  void ReadDirectory(const uint32_t directory_id,
                     const size_t max_size,
                     const ReadDirectorySuccessCallback& success_callback,
                     const ErrorCallback& error_callback);

  // Forwards the WriteDataIntoSnapshotFile request to the MTPReadFileWorker
  // object.
  //
  // |request_info| specifies the snapshot file request params.
  // |snapshot_file_info| specifies the metadata of the snapshot file.
  void WriteDataIntoSnapshotFile(
      const SnapshotRequestInfo& request_info,
      const base::File::Info& snapshot_file_info);

  // Dispatches the read bytes request to the MediaTransferProtocolManager.
  //
  // |request| contains details about the byte request including the file path,
  // byte range, and the callbacks. The callbacks specified within |request| are
  // called on the IO thread to notify the caller about success or failure.
  void ReadBytes(const MTPDeviceAsyncDelegate::ReadBytesRequest& request);

  // Forwards RenameObject request to the MediaTransferProtocolManager.
  void RenameObject(const uint32_t object_id,
                    const std::string& new_name,
                    const RenameObjectSuccessCallback& success_callback,
                    const ErrorCallback& error_callback);

  // Forwards CopyFileFromLocal request to the MediaTransferProtocolManager.
  void CopyFileFromLocal(
      const std::string& storage_name,
      const int source_file_descriptor,
      const uint32_t parent_id,
      const std::string& file_name,
      const CopyFileFromLocalSuccessCallback& success_callback,
      const ErrorCallback& error_callback);

  // Forwards DeleteObject request to the MediaTransferProtocolManager.
  void DeleteObject(const uint32_t object_id,
                    const DeleteObjectSuccessCallback& success_callback,
                    const ErrorCallback& error_callback);

  // Dispatches the CloseStorage request to the MediaTransferProtocolManager.
  void CloseStorage() const;

 private:
  // Query callback for OpenStorage() to run |callback| on the IO thread.
  //
  // If OpenStorage request succeeds, |error| is set to false and
  // |device_handle| contains the handle to communicate with the MTP device.
  //
  // If OpenStorage request fails, |error| is set to true and |device_handle| is
  // set to an empty string.
  void OnDidOpenStorage(const OpenStorageCallback& callback,
                        const std::string& device_handle,
                        bool error);

  // Query callback for GetFileInfo().
  //
  // If there is no error, |file_entry| will contain the
  // requested media device file details and |error| is set to false.
  // |success_callback| is invoked on the IO thread to notify the caller.
  //
  // If there is an error, |file_entry| is invalid and |error| is
  // set to true. |error_callback| is invoked on the IO thread to notify the
  // caller.
  void OnGetFileInfo(const GetFileInfoSuccessCallback& success_callback,
                     const ErrorCallback& error_callback,
                     device::mojom::MtpFileEntryPtr file_entry,
                     bool error) const;

  // Called when CreateDirectory completes.
  void OnCreateDirectory(const CreateDirectorySuccessCallback& success_callback,
                         const ErrorCallback& error_callback,
                         const bool error) const;

  // Query callback for ReadDirectory().
  //
  // If there is no error, |error| is set to false, |file_entries| has the
  // directory file entries and |success_callback| is invoked on the IO thread
  // to notify the caller.
  //
  // If there is an error, |error| is set to true, |file_entries| is empty
  // and |error_callback| is invoked on the IO thread to notify the caller.
  void OnDidReadDirectory(
      const ReadDirectorySuccessCallback& success_callback,
      const ErrorCallback& error_callback,
      std::vector<device::mojom::MtpFileEntryPtr> file_entries,
      bool has_more,
      bool error) const;

  // Intermediate step to finish a ReadBytes request.
  void OnGetFileInfoToReadBytes(
      const MTPDeviceAsyncDelegate::ReadBytesRequest& request,
      device::mojom::MtpFileEntryPtr file_entry,
      bool error);

  // Query callback for ReadBytes();
  //
  // If there is no error, |error| is set to false, the buffer within |request|
  // is written to, and the success callback within |request| is invoked on the
  // IO thread to notify the caller.
  //
  // If there is an error, |error| is set to true, the buffer within |request|
  // is untouched, and the error callback within |request| is invoked on the
  // IO thread to notify the caller.
  void OnDidReadBytes(
      const MTPDeviceAsyncDelegate::ReadBytesRequest& request,
      const base::File::Info& file_info,
      const std::string& data,
      bool error) const;

  // Called when RenameObject completes.
  void OnRenameObject(const RenameObjectSuccessCallback& success_callback,
                      const ErrorCallback& error_callback,
                      const bool error) const;

  // Called when CopyFileFromLocal completes.
  void OnCopyFileFromLocal(
      const CopyFileFromLocalSuccessCallback& success_callback,
      const ErrorCallback& error_callback,
      const bool error) const;

  // Called when DeleteObject completes.
  void OnDeleteObject(const DeleteObjectSuccessCallback& success_callback,
                      const ErrorCallback& error_callback,
                      const bool error) const;

  // Called when the device is uninitialized.
  //
  // Runs |error_callback| on the IO thread to notify the caller about the
  // device |error|.
  void HandleDeviceError(const ErrorCallback& error_callback,
                         base::File::Error error) const;

  // Handle to communicate with the MTP device.
  std::string device_handle_;

  // Used to handle WriteDataInfoSnapshotFile request.
  std::unique_ptr<MTPReadFileWorker> read_file_worker_;

  // For callbacks that may run after destruction.
  base::WeakPtrFactory<MTPDeviceTaskHelper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MTPDeviceTaskHelper);
};

#endif  // CHROME_BROWSER_MEDIA_GALLERIES_CHROMEOS_MTP_DEVICE_TASK_HELPER_H_
