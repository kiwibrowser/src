// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/transaction_status.h"

namespace binder {

TransactionStatus::TransactionStatus(Status status) : status_(status) {}

TransactionStatus::~TransactionStatus() = default;

uintptr_t TransactionStatus::GetCookie() const {
  return 0;
}

uint32_t TransactionStatus::GetCode() const {
  return 0;
}

pid_t TransactionStatus::GetSenderPID() const {
  return 0;
}

uid_t TransactionStatus::GetSenderEUID() const {
  return 0;
}

bool TransactionStatus::IsOneWay() const {
  return false;
}

bool TransactionStatus::HasStatus() const {
  return true;
}

Status TransactionStatus::GetStatus() const {
  return status_;
}

const void* TransactionStatus::GetData() const {
  return reinterpret_cast<const char*>(&status_);
}

size_t TransactionStatus::GetDataSize() const {
  return sizeof(status_);
}

const binder_uintptr_t* TransactionStatus::GetObjectOffsets() const {
  return nullptr;
}

size_t TransactionStatus::GetNumObjectOffsets() const {
  return 0;
}

}  // namespace binder
