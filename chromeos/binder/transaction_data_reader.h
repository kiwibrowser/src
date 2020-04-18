// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_TRANSACTION_DATA_READER_H_
#define CHROMEOS_BINDER_TRANSACTION_DATA_READER_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "chromeos/binder/buffer_reader.h"
#include "chromeos/chromeos_export.h"

namespace binder {

class CommandBroker;
class Object;
class TransactionData;

// Reads contents of a TransactionData.
// Use this class to get parameters of incoming transactions, or to get values
// from transaction replies.
class CHROMEOS_EXPORT TransactionDataReader {
 public:
  explicit TransactionDataReader(const TransactionData& data);
  ~TransactionDataReader();

  // Returns true when there is some data to read.
  bool HasMoreData() const;

  // Reads the specified number of bytes with appropriate padding.
  // Returns true on success.
  bool ReadData(void* buf, size_t n);

  // Reads an int32_t value. Returns true on success.
  bool ReadInt32(int32_t* value);

  // Reads an uint32_t value. Returns true on success.
  bool ReadUint32(uint32_t* value);

  // Reads an int64_t value. Returns true on success.
  bool ReadInt64(int64_t* value);

  // Reads an uint64_t value. Returns true on success.
  bool ReadUint64(uint64_t* value);

  // Reads a float value. Returns true on success.
  bool ReadFloat(float* value);

  // Reads a double value. Returns true on success.
  bool ReadDouble(double* value);

  // Reads a null-terminated C string.
  bool ReadCString(const char** value);

  // Reads a string.
  bool ReadString(std::string* value);

  // Reads a UTF-16 string.
  bool ReadString16(base::string16* value);

  // Reads an object. Returns null on failure.
  // |command_broker| will be used for object ref-count operations.
  scoped_refptr<Object> ReadObject(CommandBroker* command_broker);

  // Reads a file descriptor.
  // The file descriptor is owned by the TransactionData, and it will be closed
  // when the TransactionData gets destroyed. You should duplicate the FD (e.g.
  // by calling dup()) to keep owning it.
  bool ReadFileDescriptor(int* fd);

 private:
  BufferReader reader_;

  DISALLOW_COPY_AND_ASSIGN(TransactionDataReader);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_TRANSACTION_DATA_READER_H_
