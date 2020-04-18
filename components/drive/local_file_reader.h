// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_LOCAL_FILE_READER_H_
#define COMPONENTS_DRIVE_LOCAL_FILE_READER_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/completion_callback.h"
#include "net/base/file_stream.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace net {
class IOBuffer;
}  // namespace net

namespace drive {
namespace util {

// This is simple local file reader implementation focusing on Drive's use
// case. All the operations run on |sequenced_task_runner| asynchronously and
// the result will be notified to the caller via |callback|s on the caller's
// thread.
class LocalFileReader {
 public:
  explicit LocalFileReader(base::SequencedTaskRunner* sequenced_task_runner);
  ~LocalFileReader();

  // Opens the file at |file_path|. The initial position of the read will be
  // at |offset| from the beginning of the file.
  // Upon completion, |callback| will be called.
  // |callback| must not be null.
  void Open(const base::FilePath& file_path,
            int64_t offset,
            const net::CompletionCallback& callback);

  // Reads the file and copies the data into |buffer|. |buffer_length|
  // is the length of |buffer|.
  // Upon completion, |callback| will be called with the result.
  // |callback| must not be null.
  void Read(net::IOBuffer* buffer,
            int buffer_length,
            const net::CompletionCallback& callback);

 private:
  void DidOpen(const net::CompletionCallback& callback,
               int64_t offset,
               int error);
  void DidSeek(const net::CompletionCallback& callback,
               int64_t offset,
               int64_t error);

  net::FileStream file_stream_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<LocalFileReader> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(LocalFileReader);
};

}  // namespace util
}  // namespace drive

#endif  // COMPONENTS_DRIVE_LOCAL_FILE_READER_H_
