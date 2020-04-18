// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCES_CHROMEOS_ZIP_ARCHIVER_CPP_COMPRESSOR_IO_JAVASCRIPT_STREAM_H_
#define CHROME_BROWSER_RESOURCES_CHROMEOS_ZIP_ARCHIVER_CPP_COMPRESSOR_IO_JAVASCRIPT_STREAM_H_

#include <pthread.h>
#include <string>

#include "compressor_stream.h"
#include "javascript_compressor_requestor_interface.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/lock.h"
#include "ppapi/utility/threading/simple_thread.h"

// A namespace with constants used by CompressorArchiveMinizip.
namespace compressor_stream_constants {
// We need at least 256KB for MiniZip.
const int64_t kMaximumDataChunkSize = 512 * 1024;
}  // namespace compressor_stream_constants

class CompressorIOJavaScriptStream : public CompressorStream {
 public:
  CompressorIOJavaScriptStream(
      JavaScriptCompressorRequestorInterface* requestor);

  virtual ~CompressorIOJavaScriptStream();

  // Flushes the data in buffer_. Since minizip sends tons of write requests and
  // communication between C++ and JS is very expensive, we need to cache data
  // in buffer_ and send them in a lump.
  virtual int64_t Flush();

  virtual int64_t Write(int64_t zip_offset,
                        int64_t zip_length,
                        const char* zip_buffer);

  virtual int64_t WriteChunkDone(int64_t write_bytes);

  virtual int64_t Read(int64_t bytes_to_read, char* destination_buffer);

  virtual int64_t ReadFileChunkDone(int64_t read_bytes,
                                    pp::VarArrayBuffer* buffer);

 private:
  // A requestor that makes calls to JavaScript to read and write chunks.
  JavaScriptCompressorRequestorInterface* requestor_;

  pthread_mutex_t shared_state_lock_;
  pthread_cond_t available_data_cond_;
  pthread_cond_t data_written_cond_;

  // The bytelength of the data written onto the archive for the last write
  // chunk request. If this value is negative, some error occurred when writing
  // a chunk in JavaScript.
  int64_t written_bytes_;

  // The bytelength of the data read from the entry for the last read file chunk
  // request. If this value is negative, some error occurred when reading a
  // chunk in JavaScript.
  int64_t read_bytes_;

  // True if destination_buffer_ is available.
  bool available_data_;

  // Stores the data read from JavaScript.
  char* destination_buffer_;

  // The current offset from which buffer_ has data.
  int64_t buffer_offset_;

  // The size of the data in buffer_.
  int64_t buffer_data_length_;

  // The buffer that contains cached data.
  char* buffer_;
};

#endif  // CHROME_BROWSER_RESOURCES_CHROMEOS_ZIP_ARCHIVER_CPP_COMPRESSOR_IO_JAVASCRIPT_STREAM_H_
