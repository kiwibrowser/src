// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/transaction_data_from_driver.h"

#include <stddef.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/binder/command_stream.h"

namespace binder {

TransactionDataFromDriver::TransactionDataFromDriver(
    const BufferDeleter& buffer_deleter)
    : delete_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      buffer_deleter_(buffer_deleter) {
  memset(&data_, 0, sizeof(data_));
}

TransactionDataFromDriver::~TransactionDataFromDriver() {
  if (!HasStatus()) {  // Need to free the payload.
    // Close FDs.
    for (size_t i = 0; i < GetNumObjectOffsets(); ++i) {
      const auto& object = *reinterpret_cast<const flat_binder_object*>(
          reinterpret_cast<const char*>(GetData()) + GetObjectOffsets()[i]);
      if (object.type == BINDER_TYPE_FD) {
        close(object.handle);
      }
    }
    // Free data buffer.
    if (delete_task_runner_->BelongsToCurrentThread()) {
      buffer_deleter_.Run(GetData());
    } else {
      delete_task_runner_->PostTask(FROM_HERE,
                                    base::BindOnce(buffer_deleter_, GetData()));
    }
  }
}

uintptr_t TransactionDataFromDriver::GetCookie() const {
  return data_.cookie;
}

uint32_t TransactionDataFromDriver::GetCode() const {
  return data_.code;
}

pid_t TransactionDataFromDriver::GetSenderPID() const {
  return data_.sender_pid;
}

uid_t TransactionDataFromDriver::GetSenderEUID() const {
  return data_.sender_euid;
}

bool TransactionDataFromDriver::IsOneWay() const {
  return data_.flags & TF_ONE_WAY;
}

bool TransactionDataFromDriver::HasStatus() const {
  return data_.flags & TF_STATUS_CODE;
}

Status TransactionDataFromDriver::GetStatus() const {
  DCHECK(HasStatus());
  return *reinterpret_cast<const Status*>(data_.data.ptr.buffer);
}

const void* TransactionDataFromDriver::GetData() const {
  return reinterpret_cast<const void*>(data_.data.ptr.buffer);
}

size_t TransactionDataFromDriver::GetDataSize() const {
  return data_.data_size;
}

const binder_uintptr_t* TransactionDataFromDriver::GetObjectOffsets() const {
  return reinterpret_cast<const binder_uintptr_t*>(data_.data.ptr.offsets);
}

size_t TransactionDataFromDriver::GetNumObjectOffsets() const {
  DCHECK_EQ(0u, data_.offsets_size % sizeof(binder_size_t));
  return data_.offsets_size / sizeof(binder_size_t);
}

}  // namespace binder
