// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/command_broker.h"

#include <stddef.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/logging.h"
#include "chromeos/binder/binder_driver_api.h"
#include "chromeos/binder/driver.h"
#include "chromeos/binder/local_object.h"
#include "chromeos/binder/transaction_data.h"
#include "chromeos/binder/transaction_status.h"

namespace binder {

namespace {

// Converts TransactionData to binder_transaction_data struct.
binder_transaction_data ConvertTransactionDataToStruct(
    const TransactionData& data) {
  binder_transaction_data result = {};
  result.code = data.GetCode();
  result.flags = TF_ACCEPT_FDS;
  if (data.IsOneWay()) {
    result.flags |= TF_ONE_WAY;
  }
  if (data.HasStatus()) {
    result.flags |= TF_STATUS_CODE;
  }
  result.data_size = data.GetDataSize();
  result.data.ptr.buffer = reinterpret_cast<binder_uintptr_t>(data.GetData());
  result.offsets_size = data.GetNumObjectOffsets() * sizeof(binder_size_t);
  result.data.ptr.offsets =
      reinterpret_cast<binder_uintptr_t>(data.GetObjectOffsets());
  return result;
}

}  // namespace

CommandBroker::CommandBroker(Driver* driver)
    : command_stream_(driver, this), weak_ptr_factory_(this) {}

CommandBroker::~CommandBroker() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

bool CommandBroker::EnterLooper() {
  command_stream_.AppendOutgoingCommand(BC_ENTER_LOOPER, nullptr, 0);
  return command_stream_.Flush();
}

bool CommandBroker::RegisterLooper() {
  command_stream_.AppendOutgoingCommand(BC_REGISTER_LOOPER, nullptr, 0);
  return command_stream_.Flush();
}

bool CommandBroker::ExitLooper() {
  command_stream_.AppendOutgoingCommand(BC_EXIT_LOOPER, nullptr, 0);
  return command_stream_.Flush();
}

bool CommandBroker::PollCommands() {
  // Fetch and process commands.
  if (!command_stream_.Fetch()) {
    LOG(ERROR) << "Failed to fetch commands.";
    return false;
  }
  while (command_stream_.CanProcessIncomingCommand()) {
    if (!command_stream_.ProcessIncomingCommand()) {
      LOG(ERROR) << "Failed to process command.";
      return false;
    }
  }
  // Flush outgoing commands.
  if (!command_stream_.Flush()) {
    LOG(ERROR) << "Failed to flush commands.";
    return false;
  }
  return true;
}

bool CommandBroker::Transact(int32_t handle,
                             const TransactionData& request,
                             std::unique_ptr<TransactionData>* reply) {
  DCHECK(thread_checker_.CalledOnValidThread());
  // Send transaction.
  binder_transaction_data tr = ConvertTransactionDataToStruct(request);
  tr.target.handle = handle;
  command_stream_.AppendOutgoingCommand(BC_TRANSACTION, &tr, sizeof(tr));
  if (!command_stream_.Flush()) {
    LOG(ERROR) << "Failed to write";
    return false;
  }
  // Wait for response.
  std::unique_ptr<TransactionData> response_data;
  ResponseType response_type = WaitForResponse(&response_data);
  if (response_type != RESPONSE_TYPE_TRANSACTION_COMPLETE) {
    LOG(ERROR) << "Failed to wait for response: response_type = "
               << response_type;
    return false;
  }
  if (!request.IsOneWay()) {
    // Wait for reply.
    std::unique_ptr<TransactionData> response_data;
    ResponseType response_type = WaitForResponse(&response_data);
    if (response_type != RESPONSE_TYPE_TRANSACTION_REPLY) {
      LOG(ERROR) << "Failed to wait for response: response_type = "
                 << response_type;
      return false;
    }
    *reply = std::move(response_data);
  }
  return true;
}

void CommandBroker::AddReference(int32_t handle) {
  // Increment weak reference count.
  command_stream_.AppendOutgoingCommand(BC_INCREFS, &handle, sizeof(handle));
  // Increment strong reference count.
  command_stream_.AppendOutgoingCommand(BC_ACQUIRE, &handle, sizeof(handle));
}

void CommandBroker::ReleaseReference(int32_t handle) {
  // Decrement strong reference count.
  command_stream_.AppendOutgoingCommand(BC_RELEASE, &handle, sizeof(handle));
  // Decrement weak reference count.
  command_stream_.AppendOutgoingCommand(BC_DECREFS, &handle, sizeof(handle));
}

base::Closure CommandBroker::GetReleaseReferenceClosure(int32_t handle) {
  return base::Bind(&CommandBroker::ReleaseReference,
                    weak_ptr_factory_.GetWeakPtr(), handle);
}

bool CommandBroker::OnTransaction(const TransactionData& data) {
  LocalObject* object = reinterpret_cast<LocalObject*>(data.GetCookie());
  std::unique_ptr<TransactionData> reply;
  if (!object->Transact(this, data, &reply)) {
    LOG(ERROR) << "Failed to transact.";
    return false;
  }
  if (!data.IsOneWay()) {
    // Send reply.
    if (!reply) {
      reply.reset(new TransactionStatus(Status::FAILED_TRANSACTION));
    }
    binder_transaction_data tr = ConvertTransactionDataToStruct(*reply);
    tr.target.handle = -1;  // This value will be ignored. Set invalid handle.
    command_stream_.AppendOutgoingCommand(BC_REPLY, &tr, sizeof(tr));
    if (!command_stream_.Flush()) {
      LOG(ERROR) << "Failed to write";
      return false;
    }
    std::unique_ptr<TransactionData> response_data;
    ResponseType response_type = WaitForResponse(&response_data);
    // Not returning false for errors here, as doing it can result in letting
    // another process abort the loop in PollCommands() (e.g. any process can
    // cause a "dead binder" error with crash). We should return false only for
    // fundamental errors like binder protocol errors.
    LOG_IF(ERROR, response_type != RESPONSE_TYPE_TRANSACTION_COMPLETE)
        << "Error on the other end when sending reply: " << response_type;
  }
  return true;
}

void CommandBroker::OnReply(std::unique_ptr<TransactionData> data) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(response_type_, RESPONSE_TYPE_NONE);
  DCHECK(!response_data_);
  response_type_ = RESPONSE_TYPE_TRANSACTION_REPLY;
  response_data_ = std::move(data);
}

void CommandBroker::OnDeadReply() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(response_type_, RESPONSE_TYPE_NONE);
  response_type_ = RESPONSE_TYPE_DEAD;
}

void CommandBroker::OnTransactionComplete() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(response_type_, RESPONSE_TYPE_NONE);
  response_type_ = RESPONSE_TYPE_TRANSACTION_COMPLETE;
}

void CommandBroker::OnIncrementWeakReference(void* ptr, void* cookie) {
  // Do nothing.
}

void CommandBroker::OnIncrementStrongReference(void* ptr, void* cookie) {
  reinterpret_cast<LocalObject*>(cookie)->AddRef();
}

void CommandBroker::OnDecrementStrongReference(void* ptr, void* cookie) {
  reinterpret_cast<LocalObject*>(cookie)->Release();
}

void CommandBroker::OnDecrementWeakReference(void* ptr, void* cookie) {
  // Do nothing.
}

void CommandBroker::OnFailedReply() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(response_type_, RESPONSE_TYPE_NONE);
  response_type_ = RESPONSE_TYPE_FAILED;
}

CommandBroker::ResponseType CommandBroker::WaitForResponse(
    std::unique_ptr<TransactionData>* data) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK_EQ(response_type_, RESPONSE_TYPE_NONE);
  DCHECK(!response_data_);
  while (response_type_ == RESPONSE_TYPE_NONE) {
    if (command_stream_.CanProcessIncomingCommand()) {
      if (!command_stream_.ProcessIncomingCommand()) {
        LOG(ERROR) << "Failed to process command.";
        return RESPONSE_TYPE_NONE;
      }
    } else {
      // Block until response is received.
      if (!command_stream_.FetchBlocking()) {
        LOG(ERROR) << "Failed to fetch.";
        return RESPONSE_TYPE_NONE;
      }
    }
  }
  ResponseType response_type = response_type_;
  response_type_ = RESPONSE_TYPE_NONE;
  *data = std::move(response_data_);
  return response_type;
}

}  // namespace binder
