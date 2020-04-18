// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/drive/local_file_reader.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

namespace drive {
namespace util {

LocalFileReader::LocalFileReader(
    base::SequencedTaskRunner* sequenced_task_runner)
    : file_stream_(sequenced_task_runner),
      weak_ptr_factory_(this) {
}

LocalFileReader::~LocalFileReader() {
}

void LocalFileReader::Open(const base::FilePath& file_path,
                           int64_t offset,
                           const net::CompletionCallback& callback) {
  DCHECK(!callback.is_null());
  int flags = base::File::FLAG_OPEN | base::File::FLAG_READ |
              base::File::FLAG_ASYNC;

  int rv = file_stream_.Open(file_path, flags,
                             Bind(&LocalFileReader::DidOpen,
                                  weak_ptr_factory_.GetWeakPtr(),
                                  callback, offset));
  DCHECK_EQ(rv, net::ERR_IO_PENDING);
}

void LocalFileReader::Read(net::IOBuffer* in_buffer,
                           int buffer_length,
                           const net::CompletionCallback& callback) {
  DCHECK(!callback.is_null());
  DCHECK(file_stream_.IsOpen());
  int rv = file_stream_.Read(in_buffer, buffer_length, callback);
  DCHECK_EQ(rv, net::ERR_IO_PENDING);
}

void LocalFileReader::DidOpen(const net::CompletionCallback& callback,
                              int64_t offset,
                              int error) {
  if (error != net::OK)
    return callback.Run(error);

  int rv = file_stream_.Seek(
      offset, Bind(&LocalFileReader::DidSeek, weak_ptr_factory_.GetWeakPtr(),
                   callback, offset));
  DCHECK_EQ(rv, net::ERR_IO_PENDING);
}

void LocalFileReader::DidSeek(const net::CompletionCallback& callback,
                              int64_t offset,
                              int64_t error) {
  callback.Run(error == offset ? net::OK : net::ERR_FAILED);
}

}  // namespace util
}  // namespace drive
