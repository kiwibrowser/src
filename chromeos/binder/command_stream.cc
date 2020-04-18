// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/command_stream.h"

#include <stddef.h>
#include <stdint.h>

#include "base/bind.h"
#include "chromeos/binder/binder_driver_api.h"
#include "chromeos/binder/buffer_reader.h"
#include "chromeos/binder/driver.h"
#include "chromeos/binder/transaction_data.h"
#include "chromeos/binder/transaction_data_from_driver.h"
#include "chromeos/binder/util.h"

namespace binder {

CommandStream::CommandStream(Driver* driver,
                             IncomingCommandHandler* incoming_command_handler)
    : driver_(driver),
      incoming_command_handler_(incoming_command_handler),
      weak_ptr_factory_(this) {}

CommandStream::~CommandStream() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

bool CommandStream::Fetch() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!CanProcessIncomingCommand());
  // Use the same value as libbinder's IPCThreadState.
  const size_t kIncomingDataSize = 256;
  incoming_data_.resize(kIncomingDataSize);

  size_t written_bytes = 0, read_bytes = 0;
  if (!driver_->WriteRead(nullptr, 0, incoming_data_.data(),
                          incoming_data_.size(), &written_bytes, &read_bytes)) {
    LOG(ERROR) << "WriteRead() failed.";
    return false;
  }
  incoming_data_.resize(read_bytes);
  incoming_data_reader_.reset(
      new BufferReader(incoming_data_.data(), incoming_data_.size()));
  return true;
}

bool CommandStream::FetchBlocking() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return driver_->Poll() && Fetch();
}

bool CommandStream::CanProcessIncomingCommand() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return incoming_data_reader_ && incoming_data_reader_->HasMoreData();
}

bool CommandStream::ProcessIncomingCommand() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(CanProcessIncomingCommand());
  uint32_t command = 0;
  if (!incoming_data_reader_->Read(&command, sizeof(command)) ||
      !OnIncomingCommand(command, incoming_data_reader_.get())) {
    LOG(ERROR) << "Error while handling command: " << command;
    return false;
  }
  return true;
}

void CommandStream::AppendOutgoingCommand(uint32_t command,
                                          const void* data,
                                          size_t size) {
  DCHECK(thread_checker_.CalledOnValidThread());
  VLOG(1) << "Appending " << CommandToString(command) << ", this = " << this;

  DCHECK_EQ(0u, size % 4);  // Must be 4-byte aligned.
  outgoing_data_.insert(
      outgoing_data_.end(), reinterpret_cast<const char*>(&command),
      reinterpret_cast<const char*>(&command) + sizeof(command));
  outgoing_data_.insert(outgoing_data_.end(),
                        reinterpret_cast<const char*>(data),
                        reinterpret_cast<const char*>(data) + size);
}

bool CommandStream::Flush() {
  DCHECK(thread_checker_.CalledOnValidThread());
  for (size_t pos = 0; pos < outgoing_data_.size();) {
    size_t written_bytes = 0, read_bytes = 0;
    if (!driver_->WriteRead(outgoing_data_.data() + pos,
                            outgoing_data_.size() - pos, nullptr, 0,
                            &written_bytes, &read_bytes)) {
      LOG(ERROR) << "WriteRead() failed: pos = " << pos
                 << ", size = " << outgoing_data_.size();
      return false;
    }
    pos += written_bytes;
  }
  outgoing_data_.clear();
  return true;
}

bool CommandStream::OnIncomingCommand(uint32_t command, BufferReader* reader) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // TODO(hashimoto): Replace all NOTIMPLEMENTED with logic to handle incoming
  // commands.
  VLOG(1) << "Processing " << CommandToString(command) << ", this = " << this;
  switch (command) {
    case BR_ERROR: {
      int32_t error = 0;
      if (!reader->Read(&error, sizeof(error))) {
        LOG(ERROR) << "Failed to read error code.";
        return false;
      }
      break;
    }
    case BR_OK:
      break;
    case BR_TRANSACTION: {
      TransactionDataFromDriver data(
          base::Bind(&CommandStream::FreeTransactionBuffer,
                     weak_ptr_factory_.GetWeakPtr()));
      if (!reader->Read(data.mutable_data(), sizeof(*data.mutable_data()))) {
        LOG(ERROR) << "Failed to read transaction data.";
        return false;
      }
      if (!incoming_command_handler_->OnTransaction(data)) {
        LOG(ERROR) << "Failed to handle transaction.";
        return false;
      }
      break;
    }
    case BR_REPLY: {
      std::unique_ptr<TransactionDataFromDriver> data(
          new TransactionDataFromDriver(
              base::Bind(&CommandStream::FreeTransactionBuffer,
                         weak_ptr_factory_.GetWeakPtr())));
      binder_transaction_data* data_struct = data->mutable_data();
      if (!reader->Read(data_struct, sizeof(*data_struct))) {
        LOG(ERROR) << "Failed to read transaction data.";
        return false;
      }
      incoming_command_handler_->OnReply(std::move(data));
      break;
    }
    case BR_ACQUIRE_RESULT:
      // Kernel's binder.h says this is not currently supported.
      NOTREACHED();
      break;
    case BR_DEAD_REPLY:
      incoming_command_handler_->OnDeadReply();
      break;
    case BR_TRANSACTION_COMPLETE:
      incoming_command_handler_->OnTransactionComplete();
      break;
    case BR_INCREFS:
    case BR_ACQUIRE:
    case BR_RELEASE:
    case BR_DECREFS:
    case BR_ATTEMPT_ACQUIRE: {
      binder_ptr_cookie data = {};
      if (!reader->Read(&data, sizeof(data))) {
        LOG(ERROR) << "Failed to read arguments.";
        return false;
      }
      void* ptr = reinterpret_cast<void*>(data.ptr);
      void* cookie = reinterpret_cast<void*>(data.cookie);
      switch (command) {
        case BR_INCREFS:
          incoming_command_handler_->OnIncrementWeakReference(ptr, cookie);
          AppendOutgoingCommand(BC_INCREFS_DONE, &data, sizeof(data));
          break;
        case BR_ACQUIRE:
          incoming_command_handler_->OnIncrementStrongReference(ptr, cookie);
          AppendOutgoingCommand(BC_ACQUIRE_DONE, &data, sizeof(data));
          break;
        case BR_RELEASE:
          incoming_command_handler_->OnDecrementStrongReference(ptr, cookie);
          break;
        case BR_DECREFS:
          incoming_command_handler_->OnDecrementWeakReference(ptr, cookie);
          break;
        case BR_ATTEMPT_ACQUIRE:
          // Kernel's binder.h says this is not currently supported.
          NOTREACHED();
          break;
      }
      break;
    }
    case BR_NOOP:
      break;
    case BR_SPAWN_LOOPER:
      NOTIMPLEMENTED();
      break;
    case BR_FINISHED:
      // Kernel's binder.h says this is not currently supported.
      NOTREACHED();
      break;
    case BR_DEAD_BINDER:
      NOTIMPLEMENTED();
      break;
    case BR_CLEAR_DEATH_NOTIFICATION_DONE:
      NOTIMPLEMENTED();
      break;
    case BR_FAILED_REPLY:
      incoming_command_handler_->OnFailedReply();
      break;
    default:
      LOG(ERROR) << "Unexpected command: " << command;
      return false;
  }
  return true;
}

void CommandStream::FreeTransactionBuffer(const void* ptr) {
  DCHECK(thread_checker_.CalledOnValidThread());
  binder_uintptr_t p = reinterpret_cast<binder_uintptr_t>(ptr);
  AppendOutgoingCommand(BC_FREE_BUFFER, &p, sizeof(p));
}

}  // namespace binder
