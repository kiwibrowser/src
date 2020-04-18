// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_COMMAND_STREAM_H_
#define CHROMEOS_BINDER_COMMAND_STREAM_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "chromeos/chromeos_export.h"

namespace binder {

class BufferReader;
class Driver;
class TransactionData;

// Stream of incoming (binder driver to user process) BR_* commands and outgoing
// (user process to binder driver) BC_* commands.
class CHROMEOS_EXPORT CommandStream {
 public:
  // IncomingCommandHandler is responsible to handle incoming commands.
  class IncomingCommandHandler {
   public:
    virtual ~IncomingCommandHandler() {}
    // TODO(hashimoto): Add methods to handle incoming commands.

    // Called to handle BR_TRANSACTION.
    // |data| contains the parameters.
    virtual bool OnTransaction(const TransactionData& data) = 0;

    // Called to handle BR_REPLY.
    // |data| is the reply for the previous transaction.
    virtual void OnReply(std::unique_ptr<TransactionData> data) = 0;

    // Called to handle BR_DEAD_REPLY.
    virtual void OnDeadReply() = 0;

    // Called to handle BR_TRANSACTION_COMPLETE.
    virtual void OnTransactionComplete() = 0;

    // Called to handle BR_INCREFS.
    virtual void OnIncrementWeakReference(void* ptr, void* cookie) = 0;

    // Called to handle BR_ACQUIRE.
    virtual void OnIncrementStrongReference(void* ptr, void* cookie) = 0;

    // Called to handle BR_RELEASE.
    virtual void OnDecrementStrongReference(void* ptr, void* cookie) = 0;

    // Called to handle BR_DECREFS.
    virtual void OnDecrementWeakReference(void* ptr, void* cookie) = 0;

    // Called to handle BR_FAILED_REPLY.
    virtual void OnFailedReply() = 0;
  };

  CommandStream(Driver* driver,
                IncomingCommandHandler* incoming_command_handler);
  ~CommandStream();

  // Reads incoming commands from the driver to the buffer, and returns true on
  // success. If there is no data to read, returns true immediately.
  bool Fetch();

  // Does the same thing as Fetch(), but it also blocks until some data becomes
  // available for reading.
  bool FetchBlocking();

  // Returns true if any incoming commands are in the buffer.
  bool CanProcessIncomingCommand();

  // Processes an incoming command in the buffer, and returns true on success.
  bool ProcessIncomingCommand();

  // Appends a command to the outgoing command buffer.
  void AppendOutgoingCommand(uint32_t command, const void* data, size_t size);

  // Writes buffered outgoing commands to the driver, and returns true on
  // success.
  bool Flush();

 private:
  // Calls the appropriate delegate method to handle the incoming command.
  bool OnIncomingCommand(uint32_t command, BufferReader* reader);

  // Frees the buffer used by the driver to pass transaction data payload.
  void FreeTransactionBuffer(const void* ptr);

  base::ThreadChecker thread_checker_;

  Driver* driver_;
  IncomingCommandHandler* incoming_command_handler_;

  std::vector<char> outgoing_data_;  // Buffer for outgoing commands.
  std::vector<char> incoming_data_;  // Buffer for incoming commands.
  std::unique_ptr<BufferReader> incoming_data_reader_;

  base::WeakPtrFactory<CommandStream> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CommandStream);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_COMMAND_STREAM_H_
