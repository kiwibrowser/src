// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_FILEAPI_BUFFERING_FILE_STREAM_WRITER_H_
#define CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_FILEAPI_BUFFERING_FILE_STREAM_WRITER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "storage/browser/fileapi/file_stream_writer.h"

namespace net {
class IOBuffer;
}  // namespace net

namespace chromeos {
namespace file_system_provider {

// Wraps the file stream writer implementation with an intermediate buffer.
// Writes data from the inner file stream writer in chunks of size at least
// |intermediate_buffer_length| bytes (or less for the last chunk, or when
// Flush() is explicitely called).
//
// The underlying inner file stream writer *must not* return any values
// synchronously. Instead, results must be returned by a callback, including
// errors. Moreover, partial writes are *not* supported.
class BufferingFileStreamWriter : public storage::FileStreamWriter {
 public:
  BufferingFileStreamWriter(
      std::unique_ptr<storage::FileStreamWriter> file_stream_writer,
      int intermediate_buffer_length);

  ~BufferingFileStreamWriter() override;

  // storage::FileStreamWriter overrides.
  int Write(net::IOBuffer* buf,
            int buf_len,
            const net::CompletionCallback& callback) override;
  int Cancel(const net::CompletionCallback& callback) override;
  int Flush(const net::CompletionCallback& callback) override;

 private:
  // Copies |buffer_length| bytes of data from the |buffer| starting at
  // |buffer_offset| position to the current position of the intermediate
  // buffer.
  void CopyToIntermediateBuffer(scoped_refptr<net::IOBuffer> buffer,
                                int buffer_offset,
                                int buffer_length);

  // Flushes all of the bytes in the intermediate buffer to the inner file
  // stream writer.
  void FlushIntermediateBuffer(const net::CompletionCallback& callback);

  // Called when flushing the intermediate buffer is completed with either
  // a success or an error.
  void OnFlushIntermediateBufferCompleted(
      int length,
      const net::CompletionCallback& callback,
      int result);

  // Called when flushing the intermediate buffer for direct write is completed
  // with either a success or an error.
  void OnFlushIntermediateBufferForDirectWriteCompleted(
      scoped_refptr<net::IOBuffer> buffer,
      int length,
      const net::CompletionCallback& callback,
      int result);

  // Called when flushing the intermediate buffer for a buffered write is
  // completed with either a success or an error.
  void OnFlushIntermediateBufferForBufferedWriteCompleted(
      scoped_refptr<net::IOBuffer> buffer,
      int buffered_bytes,
      int bytes_left,
      const net::CompletionCallback& callback,
      int result);

  // Called when flushing the intermediate buffer for a flush call is completed
  // with either a success or an error.
  void OnFlushIntermediateBufferForFlushCompleted(
      const net::CompletionCallback& callback,
      int result);

  std::unique_ptr<storage::FileStreamWriter> file_stream_writer_;
  int intermediate_buffer_length_;
  scoped_refptr<net::IOBuffer> intermediate_buffer_;
  int buffered_bytes_;

  base::WeakPtrFactory<BufferingFileStreamWriter> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(BufferingFileStreamWriter);
};

}  // namespace file_system_provider
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_FILEAPI_BUFFERING_FILE_STREAM_WRITER_H_
