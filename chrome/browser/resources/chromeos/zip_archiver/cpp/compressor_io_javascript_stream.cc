// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "compressor_io_javascript_stream.h"

#include <limits>
#include <thread>

#include "ppapi/cpp/logging.h"

CompressorIOJavaScriptStream::CompressorIOJavaScriptStream(
    JavaScriptCompressorRequestorInterface* requestor)
    : requestor_(requestor), buffer_offset_(-1), buffer_data_length_(0) {
  pthread_mutex_init(&shared_state_lock_, nullptr);
  pthread_cond_init(&available_data_cond_, nullptr);
  pthread_cond_init(&data_written_cond_, nullptr);

  pthread_mutex_lock(&shared_state_lock_);
  available_data_ = false;
  buffer_ = new char[compressor_stream_constants::kMaximumDataChunkSize];
  pthread_mutex_unlock(&shared_state_lock_);
}

CompressorIOJavaScriptStream::~CompressorIOJavaScriptStream() {
  pthread_mutex_lock(&shared_state_lock_);
  delete buffer_;
  pthread_mutex_unlock(&shared_state_lock_);
  pthread_cond_destroy(&data_written_cond_);
  pthread_cond_destroy(&available_data_cond_);
  pthread_mutex_destroy(&shared_state_lock_);
}

int64_t CompressorIOJavaScriptStream::Flush() {
  pthread_mutex_lock(&shared_state_lock_);

  if (buffer_data_length_ == 0) {
    pthread_mutex_unlock(&shared_state_lock_);
    return 0;
  }

  // Copy the data in buffer_ to array_buffer.
  pp::VarArrayBuffer array_buffer(buffer_data_length_);
  char* array_buffer_data = static_cast<char*>(array_buffer.Map());
  memcpy(array_buffer_data, buffer_, buffer_data_length_);
  array_buffer.Unmap();

  requestor_->WriteChunkRequest(buffer_offset_, buffer_data_length_,
                                array_buffer);

  pthread_cond_wait(&data_written_cond_, &shared_state_lock_);

  int64_t written_bytes = written_bytes_;
  if (written_bytes < buffer_data_length_) {
    pthread_mutex_unlock(&shared_state_lock_);
    return -1 /* Error */;
  }

  // Reset the offset and length to the default values.
  buffer_offset_ = -1;
  buffer_data_length_ = 0;

  pthread_mutex_unlock(&shared_state_lock_);
  return written_bytes;
}

int64_t CompressorIOJavaScriptStream::Write(int64_t zip_offset,
                                            int64_t zip_length,
                                            const char* zip_buffer) {
  pthread_mutex_lock(&shared_state_lock_);

  // The offset from which the data should be written onto the archive.
  int64_t current_offset = zip_offset;
  int64_t left_length = zip_length;
  const char* buffer_pointer = zip_buffer;

  do {
    // Flush the buffer if the data in the buffer cannot be reused.
    // The following is the brief explanation about the conditions of the
    // following if statement.
    // 1: The buffer is not in the initial state (empty).
    //    The buffer should have some data to flush.
    // 2: This write operation is not to append the data to the buffer.
    // 3: The buffer overflows if we append the data to the buffer.
    //    If we can append the new data to the current data in the buffer,
    //    we should not flush the buffer.
    // 4: The index to write is outside the buffer.
    //    If we want to write data outside the range, we first need to flush
    //    the buffer, and then cache the data in the buffer.
    if (buffer_offset_ >= 0 &&                                     /* 1 */
        (current_offset != buffer_offset_ + buffer_data_length_ || /* 2 */
         buffer_data_length_ + left_length >
             compressor_stream_constants::kMaximumDataChunkSize) && /* 3 */
        (current_offset < buffer_offset_ ||
         buffer_offset_ + buffer_data_length_ <
             current_offset + left_length) /* 4 */) {
      pthread_mutex_unlock(&shared_state_lock_);
      int64_t bytes_to_write = buffer_data_length_;
      if (Flush() != bytes_to_write)
        return -1;
      pthread_mutex_lock(&shared_state_lock_);
    }

    // How many bytes we should copy to buffer_ in this iteration.
    int64_t copy_length = std::min(
        left_length, compressor_stream_constants::kMaximumDataChunkSize);
    // Set up the buffer_offset_ if the buffer_ has no data.
    if (buffer_offset_ == -1 /* initial state */)
      buffer_offset_ = current_offset;
    // Calculate the relative offset from left_length.
    int64_t offset_in_buffer = current_offset - buffer_offset_;
    // Copy data from zip_buffer, which is pointed by buffer_pointer, to
    // buffer_.
    memcpy(buffer_ + offset_in_buffer, buffer_pointer, copy_length);

    buffer_pointer += copy_length;
    buffer_data_length_ =
        std::max(buffer_data_length_, offset_in_buffer + copy_length);
    current_offset += copy_length;
    left_length -= copy_length;
  } while (left_length > 0);

  pthread_mutex_unlock(&shared_state_lock_);
  return zip_length;
}

int64_t CompressorIOJavaScriptStream::WriteChunkDone(int64_t written_bytes) {
  pthread_mutex_lock(&shared_state_lock_);
  written_bytes_ = written_bytes;
  pthread_cond_signal(&data_written_cond_);
  pthread_mutex_unlock(&shared_state_lock_);
  return written_bytes;
}

int64_t CompressorIOJavaScriptStream::Read(int64_t bytes_to_read,
                                           char* destination_buffer) {
  pthread_mutex_lock(&shared_state_lock_);

  destination_buffer_ = destination_buffer;
  requestor_->ReadFileChunkRequest(bytes_to_read);

  while (!available_data_) {
    pthread_cond_wait(&available_data_cond_, &shared_state_lock_);
  }

  int64_t read_bytes = read_bytes_;
  available_data_ = false;
  pthread_mutex_unlock(&shared_state_lock_);
  return read_bytes;
}

int64_t CompressorIOJavaScriptStream::ReadFileChunkDone(
    int64_t read_bytes,
    pp::VarArrayBuffer* array_buffer) {
  pthread_mutex_lock(&shared_state_lock_);

  // JavaScript sets a negative value in read_bytes if an error occurred while
  // reading a chunk.
  if (read_bytes >= 0) {
    char* array_buffer_data = static_cast<char*>(array_buffer->Map());
    memcpy(destination_buffer_, array_buffer_data, read_bytes);
    array_buffer->Unmap();
  }

  read_bytes_ = read_bytes;
  available_data_ = true;
  pthread_cond_signal(&available_data_cond_);
  pthread_mutex_unlock(&shared_state_lock_);
  return read_bytes;
}
