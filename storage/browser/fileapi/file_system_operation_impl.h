// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_OPERATION_IMPL_H_
#define STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_OPERATION_IMPL_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "storage/browser/blob/scoped_file.h"
#include "storage/browser/fileapi/file_system_operation.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/browser/fileapi/file_writer_delegate.h"
#include "storage/browser/storage_browser_export.h"
#include "third_party/blink/public/mojom/quota/quota_types.mojom.h"

namespace storage {

class AsyncFileUtil;
class FileSystemContext;
class RecursiveOperationDelegate;

// The default implementation of FileSystemOperation for file systems.
class STORAGE_EXPORT FileSystemOperationImpl : public FileSystemOperation {
 public:
  ~FileSystemOperationImpl() override;

  // FileSystemOperation overrides.
  void CreateFile(const FileSystemURL& url,
                  bool exclusive,
                  const StatusCallback& callback) override;
  void CreateDirectory(const FileSystemURL& url,
                       bool exclusive,
                       bool recursive,
                       const StatusCallback& callback) override;
  void Copy(const FileSystemURL& src_url,
            const FileSystemURL& dest_url,
            CopyOrMoveOption option,
            ErrorBehavior error_behavior,
            const CopyProgressCallback& progress_callback,
            const StatusCallback& callback) override;
  void Move(const FileSystemURL& src_url,
            const FileSystemURL& dest_url,
            CopyOrMoveOption option,
            const StatusCallback& callback) override;
  void DirectoryExists(const FileSystemURL& url,
                       const StatusCallback& callback) override;
  void FileExists(const FileSystemURL& url,
                  const StatusCallback& callback) override;
  void GetMetadata(const FileSystemURL& url,
                   int fields,
                   const GetMetadataCallback& callback) override;
  void ReadDirectory(const FileSystemURL& url,
                     const ReadDirectoryCallback& callback) override;
  void Remove(const FileSystemURL& url,
              bool recursive,
              const StatusCallback& callback) override;
  void Write(const FileSystemURL& url,
             std::unique_ptr<FileWriterDelegate> writer_delegate,
             std::unique_ptr<net::URLRequest> blob_request,
             const WriteCallback& callback) override;
  void Truncate(const FileSystemURL& url,
                int64_t length,
                const StatusCallback& callback) override;
  void TouchFile(const FileSystemURL& url,
                 const base::Time& last_access_time,
                 const base::Time& last_modified_time,
                 const StatusCallback& callback) override;
  void OpenFile(const FileSystemURL& url,
                int file_flags,
                const OpenFileCallback& callback) override;
  void Cancel(const StatusCallback& cancel_callback) override;
  void CreateSnapshotFile(const FileSystemURL& path,
                          const SnapshotFileCallback& callback) override;
  void CopyInForeignFile(const base::FilePath& src_local_disk_path,
                         const FileSystemURL& dest_url,
                         const StatusCallback& callback) override;
  void RemoveFile(const FileSystemURL& url,
                  const StatusCallback& callback) override;
  void RemoveDirectory(const FileSystemURL& url,
                       const StatusCallback& callback) override;
  void CopyFileLocal(const FileSystemURL& src_url,
                     const FileSystemURL& dest_url,
                     CopyOrMoveOption option,
                     const CopyFileProgressCallback& progress_callback,
                     const StatusCallback& callback) override;
  void MoveFileLocal(const FileSystemURL& src_url,
                     const FileSystemURL& dest_url,
                     CopyOrMoveOption option,
                     const StatusCallback& callback) override;
  base::File::Error SyncGetPlatformPath(const FileSystemURL& url,
                                        base::FilePath* platform_path) override;

  FileSystemContext* file_system_context() const {
    return file_system_context_.get();
  }

 private:
  friend class FileSystemOperation;

  FileSystemOperationImpl(
      const FileSystemURL& url,
      FileSystemContext* file_system_context,
      std::unique_ptr<FileSystemOperationContext> operation_context);

  // Queries the quota and usage and then runs the given |task|.
  // If an error occurs during the quota query it runs |error_callback| instead.
  void GetUsageAndQuotaThenRunTask(
      const FileSystemURL& url,
      const base::Closure& task,
      const base::Closure& error_callback);

  // Called after the quota info is obtained from the quota manager
  // (which is triggered by GetUsageAndQuotaThenRunTask).
  // Sets the quota info in the operation_context_ and then runs the given
  // |task| if the returned quota status is successful, otherwise runs
  // |error_callback|.
  void DidGetUsageAndQuotaAndRunTask(const base::Closure& task,
                                     const base::Closure& error_callback,
                                     blink::mojom::QuotaStatusCode status,
                                     int64_t usage,
                                     int64_t quota);

  // The 'body' methods that perform the actual work (i.e. posting the
  // file task on proxy_) after the quota check.
  void DoCreateFile(const FileSystemURL& url,
                    const StatusCallback& callback, bool exclusive);
  void DoCreateDirectory(const FileSystemURL& url,
                         const StatusCallback& callback,
                         bool exclusive,
                         bool recursive);
  void DoCopyFileLocal(const FileSystemURL& src,
                       const FileSystemURL& dest,
                       CopyOrMoveOption option,
                       const CopyFileProgressCallback& progress_callback,
                       const StatusCallback& callback);
  void DoMoveFileLocal(const FileSystemURL& src,
                       const FileSystemURL& dest,
                       CopyOrMoveOption option,
                       const StatusCallback& callback);
  void DoCopyInForeignFile(const base::FilePath& src_local_disk_file_path,
                           const FileSystemURL& dest,
                           const StatusCallback& callback);
  void DoTruncate(const FileSystemURL& url,
                  const StatusCallback& callback,
                  int64_t length);
  void DoOpenFile(const FileSystemURL& url,
                  const OpenFileCallback& callback, int file_flags);

  // Callback for CreateFile for |exclusive|=true cases.
  void DidEnsureFileExistsExclusive(const StatusCallback& callback,
                                    base::File::Error rv,
                                    bool created);

  // Callback for CreateFile for |exclusive|=false cases.
  void DidEnsureFileExistsNonExclusive(const StatusCallback& callback,
                                       base::File::Error rv,
                                       bool created);

  void DidFinishOperation(const StatusCallback& callback,
                          base::File::Error rv);
  void DidDirectoryExists(const StatusCallback& callback,
                          base::File::Error rv,
                          const base::File::Info& file_info);
  void DidFileExists(const StatusCallback& callback,
                     base::File::Error rv,
                     const base::File::Info& file_info);
  void DidDeleteRecursively(const FileSystemURL& url,
                            const StatusCallback& callback,
                            base::File::Error rv);
  void DidWrite(const FileSystemURL& url,
                const WriteCallback& callback,
                base::File::Error rv,
                int64_t bytes,
                FileWriterDelegate::WriteProgressStatus write_status);

  // Used only for internal assertions.
  // Returns false if there's another inflight pending operation.
  bool SetPendingOperationType(OperationType type);

  scoped_refptr<FileSystemContext> file_system_context_;

  std::unique_ptr<FileSystemOperationContext> operation_context_;
  AsyncFileUtil* async_file_util_;  // Not owned.

  std::unique_ptr<FileWriterDelegate> file_writer_delegate_;
  std::unique_ptr<RecursiveOperationDelegate> recursive_operation_delegate_;

  StatusCallback cancel_callback_;

  // A flag to make sure we call operation only once per instance.
  OperationType pending_operation_;

  base::WeakPtrFactory<FileSystemOperationImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemOperationImpl);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_OPERATION_IMPL_H_
