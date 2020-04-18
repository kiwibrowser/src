// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/writable_transaction_data.h"

#include "chromeos/binder/binder_driver_api.h"
#include "chromeos/binder/constants.h"
#include "chromeos/binder/local_object.h"
#include "chromeos/binder/object.h"
#include "chromeos/binder/remote_object.h"

namespace binder {

WritableTransactionData::WritableTransactionData() = default;

WritableTransactionData::~WritableTransactionData() = default;

uintptr_t WritableTransactionData::GetCookie() const {
  return 0;
}

uint32_t WritableTransactionData::GetCode() const {
  return code_;
}

pid_t WritableTransactionData::GetSenderPID() const {
  return 0;
}

uid_t WritableTransactionData::GetSenderEUID() const {
  return 0;
}

bool WritableTransactionData::IsOneWay() const {
  return is_one_way_;
}

bool WritableTransactionData::HasStatus() const {
  return false;
}

Status WritableTransactionData::GetStatus() const {
  return Status::OK;
}

const void* WritableTransactionData::GetData() const {
  return data_.data();
}

size_t WritableTransactionData::GetDataSize() const {
  return data_.size();
}

const binder_uintptr_t* WritableTransactionData::GetObjectOffsets() const {
  return object_offsets_.data();
}

size_t WritableTransactionData::GetNumObjectOffsets() const {
  return object_offsets_.size();
}

void WritableTransactionData::Reserve(size_t n) {
  data_.reserve(n);
}

void WritableTransactionData::WriteData(const void* data, size_t n) {
  data_.insert(data_.end(), static_cast<const char*>(data),
               static_cast<const char*>(data) + n);
  if (n % 4 != 0) {  // Add padding.
    data_.resize(data_.size() + 4 - (n % 4));
  }
}

void WritableTransactionData::WriteInt32(int32_t value) {
  WriteData(&value, sizeof(value));
}

void WritableTransactionData::WriteUint32(uint32_t value) {
  WriteData(&value, sizeof(value));
}

void WritableTransactionData::WriteInt64(int64_t value) {
  WriteData(&value, sizeof(value));
}

void WritableTransactionData::WriteUint64(uint64_t value) {
  WriteData(&value, sizeof(value));
}

void WritableTransactionData::WriteFloat(float value) {
  WriteData(&value, sizeof(value));
}

void WritableTransactionData::WriteDouble(double value) {
  WriteData(&value, sizeof(value));
}

void WritableTransactionData::WriteCString(const char* value) {
  WriteData(value, strlen(value) + 1);
}

void WritableTransactionData::WriteString(const std::string& value) {
  WriteInt32(value.size());
  if (value.size() > 0) {
    // Write only when the string is not empty.
    // This is different from WriteString16().
    //
    // Despite having the length info, null-terminate the data to be consistent
    // with libbinder.
    WriteData(value.c_str(), value.size() + 1);
  }
}

void WritableTransactionData::WriteString16(const base::string16& value) {
  WriteInt32(value.size());
  // Despite having the length info, null-terminate the data to be consistent
  // with libbinder.
  WriteData(value.c_str(), (value.size() + 1) * sizeof(base::char16));
}

void WritableTransactionData::WriteInterfaceToken(
    const base::string16& interface,
    int32_t strict_mode_policy) {
  WriteInt32(kStrictModePenaltyGather | strict_mode_policy);
  WriteString16(interface);
}

void WritableTransactionData::WriteObject(scoped_refptr<Object> object) {
  objects_.push_back(object);  // Hold reference.

  flat_binder_object flat = {};
  flat.flags = 0x7f | FLAT_BINDER_FLAG_ACCEPTS_FDS;

  switch (object->GetType()) {
    case Object::TYPE_LOCAL: {
      auto* local = static_cast<LocalObject*>(object.get());
      flat.type = BINDER_TYPE_BINDER;
      flat.cookie = reinterpret_cast<uintptr_t>(local);
      // flat.binder is unused, but the driver requires it to be a non-zero
      // unique value.
      flat.binder = reinterpret_cast<uintptr_t>(local);
      break;
    }
    case Object::TYPE_REMOTE: {
      auto* remote = static_cast<RemoteObject*>(object.get());
      flat.type = BINDER_TYPE_HANDLE;
      flat.handle = remote->GetHandle();
      break;
    }
  }
  object_offsets_.push_back(data_.size());
  WriteData(&flat, sizeof(flat));
}

void WritableTransactionData::WriteFileDescriptor(base::ScopedFD fd) {
  files_.push_back(std::move(fd));

  flat_binder_object flat = {};
  flat.type = BINDER_TYPE_FD;
  flat.flags = 0x7f | FLAT_BINDER_FLAG_ACCEPTS_FDS;
  flat.handle = files_.back().get();
  object_offsets_.push_back(data_.size());
  WriteData(&flat, sizeof(flat));
}

}  // namespace binder
