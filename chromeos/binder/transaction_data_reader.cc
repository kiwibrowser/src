// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/transaction_data_reader.h"

#include <stddef.h>
#include <stdint.h>

#include "base/logging.h"
#include "chromeos/binder/binder_driver_api.h"
#include "chromeos/binder/local_object.h"
#include "chromeos/binder/object.h"
#include "chromeos/binder/remote_object.h"
#include "chromeos/binder/transaction_data.h"

namespace binder {

namespace {

// Adds appropriate padding to the given size to make it 4-byte aligned.
size_t AddPadding(size_t n) {
  return (n + 3) & (~3);
}

}  // namespace

TransactionDataReader::TransactionDataReader(const TransactionData& data)
    : reader_(reinterpret_cast<const char*>(data.GetData()),
              data.GetDataSize()) {}

TransactionDataReader::~TransactionDataReader() = default;

bool TransactionDataReader::HasMoreData() const {
  return reader_.HasMoreData();
}

bool TransactionDataReader::ReadData(void* buf, size_t n) {
  DCHECK(buf);
  return reader_.Read(buf, n) && reader_.Skip(AddPadding(n) - n);
}

bool TransactionDataReader::ReadInt32(int32_t* value) {
  return ReadData(value, sizeof(*value));
}

bool TransactionDataReader::ReadUint32(uint32_t* value) {
  return ReadData(value, sizeof(*value));
}

bool TransactionDataReader::ReadInt64(int64_t* value) {
  return ReadData(value, sizeof(*value));
}

bool TransactionDataReader::ReadUint64(uint64_t* value) {
  return ReadData(value, sizeof(*value));
}

bool TransactionDataReader::ReadFloat(float* value) {
  return ReadData(value, sizeof(*value));
}

bool TransactionDataReader::ReadDouble(double* value) {
  return ReadData(value, sizeof(*value));
}

bool TransactionDataReader::ReadCString(const char** value) {
  *value = reader_.current();
  for (size_t len = 0; reader_.HasMoreData(); ++len) {
    char c = 0;
    if (!reader_.Read(&c, sizeof(c))) {
      return false;
    }
    if (c == 0) {
      return reader_.Skip(AddPadding(len + 1) - (len + 1));
    }
  }
  return false;
}

bool TransactionDataReader::ReadString(std::string* value) {
  int32_t len = 0;
  if (!ReadInt32(&len)) {
    return false;
  }
  if (len == 0) {
    // Read only when the string is not empty.
    // This is different from ReadString16().
    value->clear();
    return true;
  }
  const char* start = reader_.current();
  if (!reader_.Skip(AddPadding(len + 1))) {
    return false;
  }
  value->assign(start, len);
  return true;
}

bool TransactionDataReader::ReadString16(base::string16* value) {
  int32_t len = 0;
  if (!ReadInt32(&len)) {
    return false;
  }
  const base::char16* start =
      reinterpret_cast<const base::char16*>(reader_.current());
  if (!reader_.Skip(AddPadding((len + 1) * sizeof(base::char16)))) {
    return false;
  }
  value->assign(start, len);
  return true;
}

scoped_refptr<Object> TransactionDataReader::ReadObject(
    CommandBroker* command_broker) {
  DCHECK(command_broker);
  flat_binder_object obj = {};
  if (!ReadData(&obj, sizeof(obj))) {
    return scoped_refptr<Object>();
  }
  switch (obj.type) {
    case BINDER_TYPE_HANDLE:
      return base::MakeRefCounted<RemoteObject>(command_broker, obj.handle);
    case BINDER_TYPE_BINDER:
      return base::WrapRefCounted(reinterpret_cast<LocalObject*>(obj.cookie));
  }
  return scoped_refptr<Object>();
}

bool TransactionDataReader::ReadFileDescriptor(int* fd) {
  flat_binder_object obj = {};
  if (!ReadData(&obj, sizeof(obj)) || obj.type != BINDER_TYPE_FD) {
    return false;
  }
  *fd = obj.handle;
  return true;
}

}  // namespace binder
