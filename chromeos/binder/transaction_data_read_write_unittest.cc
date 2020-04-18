// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chromeos/binder/binder_driver_api.h"
#include "chromeos/binder/command_broker.h"
#include "chromeos/binder/driver.h"
#include "chromeos/binder/local_object.h"
#include "chromeos/binder/remote_object.h"
#include "chromeos/binder/transaction_data_reader.h"
#include "chromeos/binder/writable_transaction_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace binder {

TEST(BinderTransactionDataReadWriteTest, Data) {
  std::vector<char> input(3);
  for (size_t i = 0; i < input.size(); ++i) {
    input[i] = i;
  }
  WritableTransactionData data;
  EXPECT_EQ(0u, data.GetDataSize());
  data.WriteData(input.data(), input.size());
  EXPECT_EQ(4u, data.GetDataSize());  // Padded for 4-byte alignment.
  for (size_t i = 0; i < input.size(); ++i) {
    SCOPED_TRACE(i);
    EXPECT_EQ(input[i], reinterpret_cast<const char*>(data.GetData())[i]);
  }

  TransactionDataReader reader(data);
  EXPECT_TRUE(reader.HasMoreData());

  std::vector<char> result(input.size());
  EXPECT_TRUE(reader.ReadData(result.data(), result.size()));
  EXPECT_EQ(input, result);
  // Although we read only 3 bytes, we've already consumed 4 bytes because of
  // padding.
  EXPECT_FALSE(reader.HasMoreData());
}

TEST(BinderTransactionDataReadWriteTest, ScalarValues) {
  const int32_t kInt32Value = -1;
  const uint32_t kUint32Value = 2;
  const int64_t kInt64Value = -3;
  const uint64_t kUint64Value = 4;
  const float kFloatValue = 5.55;
  const double kDoubleValue = 6.66;

  WritableTransactionData data;
  data.WriteInt32(kInt32Value);
  data.WriteUint32(kUint32Value);
  data.WriteInt64(kInt64Value);
  data.WriteUint64(kUint64Value);
  data.WriteFloat(kFloatValue);
  data.WriteDouble(kDoubleValue);
  {
    BufferReader reader(reinterpret_cast<const char*>(data.GetData()),
                        data.GetDataSize());
    {
      int32_t result = 0;
      EXPECT_TRUE(reader.Read(&result, sizeof(result)));
      EXPECT_EQ(kInt32Value, result);
    }
    {
      uint32_t result = 0;
      EXPECT_TRUE(reader.Read(&result, sizeof(result)));
      EXPECT_EQ(kUint32Value, result);
    }
    {
      int64_t result = 0;
      EXPECT_TRUE(reader.Read(&result, sizeof(result)));
      EXPECT_EQ(kInt64Value, result);
    }
    {
      uint64_t result = 0;
      EXPECT_TRUE(reader.Read(&result, sizeof(result)));
      EXPECT_EQ(kUint64Value, result);
    }
    {
      float result = 0;
      EXPECT_TRUE(reader.Read(&result, sizeof(result)));
      EXPECT_EQ(kFloatValue, result);
    }
    {
      double result = 0;
      EXPECT_TRUE(reader.Read(&result, sizeof(result)));
      EXPECT_EQ(kDoubleValue, result);
    }
    EXPECT_FALSE(reader.HasMoreData());
  }

  TransactionDataReader reader(data);
  EXPECT_TRUE(reader.HasMoreData());
  {
    int32_t result = 0;
    EXPECT_TRUE(reader.ReadInt32(&result));
    EXPECT_EQ(kInt32Value, result);
  }
  {
    uint32_t result = 0;
    EXPECT_TRUE(reader.ReadUint32(&result));
    EXPECT_EQ(kUint32Value, result);
  }
  {
    int64_t result = 0;
    EXPECT_TRUE(reader.ReadInt64(&result));
    EXPECT_EQ(kInt64Value, result);
  }
  {
    uint64_t result = 0;
    EXPECT_TRUE(reader.ReadUint64(&result));
    EXPECT_EQ(kUint64Value, result);
  }
  {
    float result = 0;
    EXPECT_TRUE(reader.ReadFloat(&result));
    EXPECT_EQ(kFloatValue, result);
  }
  {
    double result = 0;
    EXPECT_TRUE(reader.ReadDouble(&result));
    EXPECT_EQ(kDoubleValue, result);
  }
  EXPECT_FALSE(reader.HasMoreData());
}

TEST(BinderTransactionDataReadWriteTest, CString) {
  const char kValue[] = "FooBar";
  WritableTransactionData data;
  data.WriteCString(kValue);
  EXPECT_EQ(8u, data.GetDataSize());  // Padded for 4-byte alignment.
  {
    BufferReader reader(reinterpret_cast<const char*>(data.GetData()),
                        data.GetDataSize());
    char result[sizeof(kValue)];
    EXPECT_TRUE(reader.Read(&result, sizeof(result)));
    EXPECT_STREQ(kValue, result);
  }
  TransactionDataReader reader(data);
  const char* result = nullptr;
  EXPECT_TRUE(reader.ReadCString(&result));
  EXPECT_STREQ(kValue, result);
  EXPECT_FALSE(reader.HasMoreData());
}

TEST(BinderTransactionDataReadWriteTest, String) {
  const std::string kValue = "FooBar";
  WritableTransactionData data;
  data.WriteString(kValue);
  // 12 = 4 (length) + 6 (string) + 1 (null) + 1 (padding).
  EXPECT_EQ(12u, data.GetDataSize());
  {
    BufferReader reader(reinterpret_cast<const char*>(data.GetData()),
                        data.GetDataSize());
    int32_t len = 0;
    EXPECT_TRUE(reader.Read(&len, sizeof(len)));
    EXPECT_EQ(kValue.size(), len);
    std::vector<char> result(kValue.size() + 1);
    EXPECT_TRUE(reader.Read(result.data(), result.size()));
    EXPECT_EQ(kValue, std::string(result.data(), result.size() - 1));
    EXPECT_EQ(0, result.back());  // null-terminated.
  }
  TransactionDataReader reader(data);
  std::string result;
  EXPECT_TRUE(reader.ReadString(&result));
  EXPECT_EQ(kValue, result);
  EXPECT_FALSE(reader.HasMoreData());
}

// Empty string is handled specially.
TEST(BinderTransactionDataReadWriteTest, EmptyString) {
  WritableTransactionData data;
  data.WriteString(std::string());
  EXPECT_EQ(4u, data.GetDataSize());  // Only the length is written.
  {
    BufferReader reader(reinterpret_cast<const char*>(data.GetData()),
                        data.GetDataSize());
    int32_t len = 0;
    EXPECT_TRUE(reader.Read(&len, sizeof(len)));
    EXPECT_EQ(0, len);
    EXPECT_FALSE(reader.HasMoreData());
  }
  TransactionDataReader reader(data);
  std::string result;
  EXPECT_TRUE(reader.ReadString(&result));
  EXPECT_EQ(std::string(), result);
  EXPECT_FALSE(reader.HasMoreData());
}

TEST(BinderTransactionDataReadWriteTest, String16) {
  const base::string16 kValue = base::ASCIIToUTF16("FooBar");
  WritableTransactionData data;
  data.WriteString16(kValue);
  // 20 = 4 (length) + 12 (string) + 2 (null) + 2 (padding).
  EXPECT_EQ(20u, data.GetDataSize());
  {
    BufferReader reader(reinterpret_cast<const char*>(data.GetData()),
                        data.GetDataSize());
    int32_t len = 0;
    EXPECT_TRUE(reader.Read(&len, sizeof(len)));
    EXPECT_EQ(kValue.size(), len);
    std::vector<base::char16> result(kValue.size() + 1);
    EXPECT_TRUE(
        reader.Read(result.data(), result.size() * sizeof(base::char16)));
    EXPECT_EQ(kValue, base::string16(result.data(), result.size() - 1));
    EXPECT_EQ(0, result.back());  // null-terminated.
  }
  TransactionDataReader reader(data);
  base::string16 result;
  EXPECT_TRUE(reader.ReadString16(&result));
  EXPECT_EQ(kValue, result);
  EXPECT_FALSE(reader.HasMoreData());
}

TEST(BinderTransactionDataReadWriteTest, Object) {
  base::MessageLoopForIO message_loop;

  Driver driver;
  ASSERT_TRUE(driver.Initialize());
  CommandBroker command_broker(&driver);

  scoped_refptr<LocalObject> local(
      new LocalObject(std::unique_ptr<LocalObject::TransactionHandler>()));

  const int32_t kDummyHandle = 42;
  scoped_refptr<RemoteObject> remote(
      new RemoteObject(&command_broker, kDummyHandle));

  // Write a local object & a remote object.
  WritableTransactionData data;
  data.WriteObject(local);
  data.WriteObject(remote);

  // Check object offsets.
  ASSERT_EQ(2u, data.GetNumObjectOffsets());
  EXPECT_EQ(0u, data.GetObjectOffsets()[0]);
  EXPECT_EQ(sizeof(flat_binder_object), data.GetObjectOffsets()[1]);
  {
    // Check the written local object.
    BufferReader reader(reinterpret_cast<const char*>(data.GetData()),
                        data.GetDataSize());
    flat_binder_object result = {};
    EXPECT_TRUE(reader.Read(&result, sizeof(result)));
    EXPECT_EQ(BINDER_TYPE_BINDER, result.type);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(local.get()), result.cookie);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(local.get()), result.binder);

    // Check the written remote object.
    EXPECT_TRUE(reader.Read(&result, sizeof(result)));
    EXPECT_EQ(BINDER_TYPE_HANDLE, result.type);
    EXPECT_EQ(kDummyHandle, result.handle);
  }
  // Read the local object.
  TransactionDataReader reader(data);
  scoped_refptr<Object> result = reader.ReadObject(&command_broker);
  ASSERT_TRUE(result);
  ASSERT_EQ(Object::TYPE_LOCAL, result->GetType());
  EXPECT_EQ(local.get(), static_cast<LocalObject*>(result.get()));

  // Read the remote object.
  result = reader.ReadObject(&command_broker);
  ASSERT_TRUE(result);
  ASSERT_EQ(Object::TYPE_REMOTE, result->GetType());
  EXPECT_EQ(kDummyHandle,
            static_cast<RemoteObject*>(result.get())->GetHandle());

  EXPECT_FALSE(reader.HasMoreData());
}

TEST(BinderTransactionDataReadWriteTest, FileDescriptor) {
  // Prepare a test file.
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath path;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir.GetPath(), &path));

  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  ASSERT_TRUE(file.IsValid());

  base::ScopedFD scoped_fd(file.TakePlatformFile());
  int kFdValue = scoped_fd.get();

  // Write the file descriptor.
  WritableTransactionData data;
  data.WriteFileDescriptor(std::move(scoped_fd));

  // Check object offsets.
  ASSERT_EQ(1u, data.GetNumObjectOffsets());
  EXPECT_EQ(0u, data.GetObjectOffsets()[0]);
  {
    // Check the written file descriptor.
    BufferReader reader(reinterpret_cast<const char*>(data.GetData()),
                        data.GetDataSize());
    flat_binder_object result = {};
    EXPECT_TRUE(reader.Read(&result, sizeof(result)));
    EXPECT_EQ(BINDER_TYPE_FD, result.type);
    EXPECT_EQ(kFdValue, result.handle);
  }
  // Read the file descriptor.
  TransactionDataReader reader(data);
  int result = -1;
  EXPECT_TRUE(reader.ReadFileDescriptor(&result));
  EXPECT_EQ(kFdValue, result);
}

}  // namespace binder
