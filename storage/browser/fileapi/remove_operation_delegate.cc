// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/remove_operation_delegate.h"

#include "base/bind.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_runner.h"

namespace storage {

RemoveOperationDelegate::RemoveOperationDelegate(
    FileSystemContext* file_system_context,
    const FileSystemURL& url,
    const StatusCallback& callback)
    : RecursiveOperationDelegate(file_system_context),
      url_(url),
      callback_(callback),
      weak_factory_(this) {
}

RemoveOperationDelegate::~RemoveOperationDelegate() = default;

void RemoveOperationDelegate::Run() {
  operation_runner()->RemoveFile(url_, base::Bind(
      &RemoveOperationDelegate::DidTryRemoveFile, weak_factory_.GetWeakPtr()));
}

void RemoveOperationDelegate::RunRecursively() {
  StartRecursiveOperation(url_, FileSystemOperation::ERROR_BEHAVIOR_ABORT,
                          callback_);
}

void RemoveOperationDelegate::ProcessFile(const FileSystemURL& url,
                                          const StatusCallback& callback) {
  operation_runner()->RemoveFile(
      url,
      base::Bind(&RemoveOperationDelegate::DidRemoveFile,
                 weak_factory_.GetWeakPtr(), callback));
}

void RemoveOperationDelegate::ProcessDirectory(const FileSystemURL& url,
                                               const StatusCallback& callback) {
  callback.Run(base::File::FILE_OK);
}

void RemoveOperationDelegate::PostProcessDirectory(
    const FileSystemURL& url, const StatusCallback& callback) {
  operation_runner()->RemoveDirectory(url, callback);
}

void RemoveOperationDelegate::DidTryRemoveFile(base::File::Error error) {
  if (error != base::File::FILE_ERROR_NOT_A_FILE &&
      error != base::File::FILE_ERROR_SECURITY) {
    callback_.Run(error);
    return;
  }
  operation_runner()->RemoveDirectory(
      url_,
      base::Bind(&RemoveOperationDelegate::DidTryRemoveDirectory,
                 weak_factory_.GetWeakPtr(), error));
}

void RemoveOperationDelegate::DidTryRemoveDirectory(
    base::File::Error remove_file_error,
    base::File::Error remove_directory_error) {
  callback_.Run(
      remove_directory_error == base::File::FILE_ERROR_NOT_A_DIRECTORY ?
      remove_file_error :
      remove_directory_error);
}

void RemoveOperationDelegate::DidRemoveFile(const StatusCallback& callback,
                                            base::File::Error error) {
  if (error == base::File::FILE_ERROR_NOT_FOUND) {
    callback.Run(base::File::FILE_OK);
    return;
  }
  callback.Run(error);
}

}  // namespace storage
