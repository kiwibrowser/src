// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_WRITABLE_TRANSACTION_DATA_H_
#define CHROMEOS_BINDER_WRITABLE_TRANSACTION_DATA_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "chromeos/binder/transaction_data.h"
#include "chromeos/chromeos_export.h"

namespace binder {

class Object;

// Use this class to construct TransactionData (as parameters and replies) to
// transact with remote objects.
// GetSenderPID() and GetSenderEUID() return 0.
//
// Note: Unlike D-Bus, the binder driver doesn't care about the validity of
// contents being communicated. Packed data is treated simply as a blob, so it
// doesn't provide any string encoding check, endian conversion, nor type
// information about the packed data. It's developers' responsibility to ensure
// that the sending process and the receiving process are handling the
// transaction data in the same manner.
class CHROMEOS_EXPORT WritableTransactionData : public TransactionData {
 public:
  WritableTransactionData();
  ~WritableTransactionData() override;

  // TransactionData override:
  uintptr_t GetCookie() const override;
  uint32_t GetCode() const override;
  pid_t GetSenderPID() const override;
  uid_t GetSenderEUID() const override;
  bool IsOneWay() const override;
  bool HasStatus() const override;
  Status GetStatus() const override;
  const void* GetData() const override;
  size_t GetDataSize() const override;
  const binder_uintptr_t* GetObjectOffsets() const override;
  size_t GetNumObjectOffsets() const override;

  // Expands the capacity of the internal buffer.
  void Reserve(size_t n);

  // Sets the transaction code returned by GetCode().
  void SetCode(uint32_t code) { code_ = code; }

  // Sets the value returned by IsOneWay().
  void SetIsOneWay(bool is_one_way) { is_one_way_ = is_one_way; }

  // Appends the specified data with appropriate padding.
  void WriteData(const void* data, size_t n);

  // Appends an int32_t value.
  void WriteInt32(int32_t value);

  // Appends a uint32_t value.
  void WriteUint32(uint32_t value);

  // Appends an int64_t vlaue.
  void WriteInt64(int64_t value);

  // Appends a uint64_t value.
  void WriteUint64(uint64_t value);

  // Appends a float value.
  void WriteFloat(float value);

  // Appends a double value.
  void WriteDouble(double value);

  // Appends a null-terminated C string.
  void WriteCString(const char* value);

  // Appends a string.
  void WriteString(const std::string& value);

  // Appends a UTF-16 string.
  void WriteString16(const base::string16& value);

  // Appends an RPC header.
  // |interface| is the interface which must be implemented by the receiving
  // process (e.g. android.os.IServiceManager).
  // |strict_mode_policy| is the current thread's strict mode policy.
  // (see http://developer.android.com/reference/android/os/StrictMode.html)
  void WriteInterfaceToken(const base::string16& interface,
                           int32_t strict_mode_policy);

  // Appends an object.
  void WriteObject(scoped_refptr<Object> object);

  // Appends a file descriptor.
  void WriteFileDescriptor(base::ScopedFD fd);

 private:
  uint32_t code_ = 0;
  bool is_one_way_ = false;
  std::vector<char> data_;
  std::vector<binder_uintptr_t> object_offsets_;
  std::vector<scoped_refptr<Object>> objects_;
  std::vector<base::ScopedFD> files_;

  DISALLOW_COPY_AND_ASSIGN(WritableTransactionData);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_WRITABLE_TRANSACTION_DATA_H_
