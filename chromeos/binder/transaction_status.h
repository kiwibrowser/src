// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_TRANSACTION_STATUS_H_
#define CHROMEOS_BINDER_TRANSACTION_STATUS_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"
#include "chromeos/binder/status.h"
#include "chromeos/binder/transaction_data.h"
#include "chromeos/chromeos_export.h"

namespace binder {

// TransactionData whose contents is a status code.
// Use this class to return an error for transactions.
// GetSenderPID() and GetSenderEUID() return 0.
class CHROMEOS_EXPORT TransactionStatus : public TransactionData {
 public:
  explicit TransactionStatus(Status status);
  ~TransactionStatus() override;

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

 private:
  Status status_;
  DISALLOW_COPY_AND_ASSIGN(TransactionStatus);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_TRANSACTION_STATUS_H_
