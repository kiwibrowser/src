// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_DRIVE_FILE_STREAM_READER_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_DRIVE_FILE_STREAM_READER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_errors.h"
#include "google_apis/drive/drive_api_error_codes.h"
#include "net/base/completion_callback.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace net {
class HttpByteRange;
class IOBuffer;
}  // namespace net

namespace drive {
namespace util {
class LocalFileReader;
}  // namespace util

namespace internal {

// An interface to dispatch the reading operation. If the file is locally
// cached, LocalReaderProxy defined below will be used. Otherwise (i.e. the
// file is being downloaded from the server), NetworkReaderProxy will be used.
class ReaderProxy {
 public:
  virtual ~ReaderProxy() {}

  // Called from DriveFileStreamReader::Read method.
  virtual int Read(net::IOBuffer* buffer, int buffer_length,
                   const net::CompletionCallback& callback) = 0;

  // Called when the data from the server is received.
  virtual void OnGetContent(std::unique_ptr<std::string> data) = 0;

  // Called when the accessing to the file system is completed.
  virtual void OnCompleted(FileError error) = 0;
};

// The read operation implementation for the locally cached files.
class LocalReaderProxy : public ReaderProxy {
 public:
  // The |file_reader| should be the instance which is already opened.
  // This class takes its ownership.
  // |length| is the number of bytes to be read. It must be equal or
  // smaller than the remaining data size in the |file_reader|.
  LocalReaderProxy(std::unique_ptr<util::LocalFileReader> file_reader,
                   int64_t length);
  ~LocalReaderProxy() override;

  // ReaderProxy overrides.
  int Read(net::IOBuffer* buffer,
           int buffer_length,
           const net::CompletionCallback& callback) override;
  void OnGetContent(std::unique_ptr<std::string> data) override;
  void OnCompleted(FileError error) override;

 private:
  std::unique_ptr<util::LocalFileReader> file_reader_;

  // Callback for the LocalFileReader::Read.
  void OnReadCompleted(
      const net::CompletionCallback& callback, int read_result);

  // The number of remaining bytes to be read.
  int64_t remaining_length_;

  base::ThreadChecker thread_checker_;

  // This should remain the last member so it'll be destroyed first and
  // invalidate its weak pointers before other members are destroyed.
  base::WeakPtrFactory<LocalReaderProxy> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(LocalReaderProxy);
};

// The read operation implementation for the file which is being downloaded.
class NetworkReaderProxy : public ReaderProxy {
 public:
  // If the instance is deleted during the download process, it is necessary
  // to cancel the job. |job_canceller| should be the callback to run the
  // cancelling. |full_content_length| is necessary for determining whether the
  // deletion is done in the middle of download process.
  NetworkReaderProxy(int64_t offset,
                     int64_t content_length,
                     int64_t full_content_length,
                     const base::Closure& job_canceller);
  ~NetworkReaderProxy() override;

  // ReaderProxy overrides.
  int Read(net::IOBuffer* buffer,
           int buffer_length,
           const net::CompletionCallback& callback) override;
  void OnGetContent(std::unique_ptr<std::string> data) override;
  void OnCompleted(FileError error) override;

 private:
  // The data received from the server, but not yet read.
  std::vector<std::unique_ptr<std::string>> pending_data_;

  // The number of bytes to be skipped.
  int64_t remaining_offset_;

  // The number of bytes of remaining data (including the data not yet
  // received from the server).
  int64_t remaining_content_length_;

  // Flag to remember whether this read request is for reading till the end of
  // the file.
  const bool is_full_download_;

  int error_code_;

  // To support pending Read(), it is necessary to keep its arguments.
  scoped_refptr<net::IOBuffer> buffer_;
  int buffer_length_;
  net::CompletionCallback callback_;

  base::ThreadChecker thread_checker_;

  // Keeps the closure to cancel downloading job if necessary.
  // Will be reset when the job is completed (regardless whether the job is
  // successfully done or not).
  base::Closure job_canceller_;

  DISALLOW_COPY_AND_ASSIGN(NetworkReaderProxy);
};

}  // namespace internal

class FileSystemInterface;
class ResourceEntry;

// The stream reader for a file in FileSystem. Instances of this class
// should live on IO thread.
// Operations to communicate with a locally cached file will run on
// |file_task_runner| specified by the constructor.
class DriveFileStreamReader {
 public:
  // Callback to return the FileSystemInterface instance. This is an
  // injecting point for testing.
  // Note that the callback will be copied between threads (IO and UI), and
  // will be called on UI thread.
  typedef base::Callback<FileSystemInterface*()> FileSystemGetter;

  // Callback to return the result of Initialize().
  // |error| is net::Error code.
  typedef base::Callback<void(int error, std::unique_ptr<ResourceEntry> entry)>
      InitializeCompletionCallback;

  DriveFileStreamReader(const FileSystemGetter& file_system_getter,
                        base::SequencedTaskRunner* file_task_runner);
  ~DriveFileStreamReader();

  // Returns true if the reader is initialized.
  bool IsInitialized() const;

  // Initializes the stream for the |drive_file_path|.
  // |callback| must not be null.
  void Initialize(const base::FilePath& drive_file_path,
                  const net::HttpByteRange& byte_range,
                  const InitializeCompletionCallback& callback);

  // Reads the data into |buffer| at most |buffer_length|, and returns
  // the number of bytes. If an error happened, returns an error code.
  // If no data is available yet, returns net::ERR_IO_PENDING immediately,
  // and when the data is available the actual Read operation is done
  // and |callback| will be run with the result.
  // The Read() method must not be called before the Initialize() is completed
  // successfully, or if there is pending read operation.
  // Neither |buffer| nor |callback| must be null.
  int Read(net::IOBuffer* buffer, int buffer_length,
           const net::CompletionCallback& callback);

 private:
  // Used to store the cancel closure returned by FileSystemInterface.
  void StoreCancelDownloadClosure(const base::Closure& cancel_download_closure);

  // Part of Initialize. Called after GetFileContent's initialization
  // is done.
  void InitializeAfterGetFileContentInitialized(
      const net::HttpByteRange& byte_range,
      const InitializeCompletionCallback& callback,
      FileError error,
      const base::FilePath& local_cache_file_path,
      std::unique_ptr<ResourceEntry> entry);

  // Part of Initialize. Called when the local file open process is done.
  void InitializeAfterLocalFileOpen(
      int64_t length,
      const InitializeCompletionCallback& callback,
      std::unique_ptr<ResourceEntry> entry,
      std::unique_ptr<util::LocalFileReader> file_reader,
      int open_result);

  // Called when the data is received from the server.
  void OnGetContent(google_apis::DriveApiErrorCode error_code,
                    std::unique_ptr<std::string> data);

  // Called when GetFileContent is completed.
  void OnGetFileContentCompletion(
      const InitializeCompletionCallback& callback,
      FileError error);

  const FileSystemGetter file_system_getter_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  base::Closure cancel_download_closure_;
  std::unique_ptr<internal::ReaderProxy> reader_proxy_;

  base::ThreadChecker thread_checker_;

  // This should remain the last member so it'll be destroyed first and
  // invalidate its weak pointers before other members are destroyed.
  base::WeakPtrFactory<DriveFileStreamReader> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DriveFileStreamReader);
};

}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_DRIVE_FILE_STREAM_READER_H_
