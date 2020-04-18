// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_TRANSACTION_DATA_H_
#define CHROMEOS_BINDER_TRANSACTION_DATA_H_

#include <sys/types.h>

#include "chromeos/binder/binder_driver_api.h"
#include "chromeos/binder/status.h"

namespace binder {

// Data passed via transactions.
class TransactionData {
 public:
  virtual ~TransactionData() {}

  // Returns the cookie which can be used to identify the target object.
  virtual uintptr_t GetCookie() const = 0;

  // Returns the transaction code.
  virtual uint32_t GetCode() const = 0;

  // Returns the PID of the sender.
  virtual pid_t GetSenderPID() const = 0;

  // Returns the EUID of the sender.
  virtual uid_t GetSenderEUID() const = 0;

  // Returns true if there is no need to send reply for the transaction.
  virtual bool IsOneWay() const = 0;

  // Returns true if the payload is a status code.
  virtual bool HasStatus() const = 0;

  // Returns the status code.
  // This method should be called only when HasStatus() returns true.
  virtual Status GetStatus() const = 0;

  // Returns the payload data.
  virtual const void* GetData() const = 0;

  // Returns the size of the payload.
  virtual size_t GetDataSize() const = 0;

  // Returns the offsets of objects stored in the payload.
  virtual const binder_uintptr_t* GetObjectOffsets() const = 0;

  // Returns the number of objects.
  virtual size_t GetNumObjectOffsets() const = 0;
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_TRANSACTION_DATA_H_
