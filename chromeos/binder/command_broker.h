// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_COMMAND_BROKER_H_
#define CHROMEOS_BINDER_COMMAND_BROKER_H_

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "chromeos/binder/command_stream.h"
#include "chromeos/chromeos_export.h"

namespace binder {

class Driver;
class TransactionData;

// Issues appropriate outgoing commands to perform required tasks, and
// dispatches incoming commands to appropriate objects.
// Usually this class lives as long as the corresponding thread.
// TODO(hashimoto): Add code to handle incoming commands (e.g. transactions).
class CHROMEOS_EXPORT CommandBroker
    : public CommandStream::IncomingCommandHandler {
 public:
  explicit CommandBroker(Driver* driver);
  ~CommandBroker() override;

  // Tells the driver that the current thread entered command handling loop.
  // Returns true on success.
  // This method must be used by the main thread which owns the Driver instance,
  // sub threads should use RegisterLooper().
  bool EnterLooper();

  // Tells the driver that the current thread entered command handling loop.
  // Returns true on success.
  // This method must be used by sub threads, the main thread should use
  // EnterLooper().
  bool RegisterLooper();

  // Tells the driver that the current thread exited command handling loop.
  // Returns true on success.
  bool ExitLooper();

  // Fetches incoming commands and handles them.
  // Returns true on success.
  bool PollCommands();

  // Performs transaction with the remote object specified by the handle.
  // Returns true on success. If not one-way transaction, this method blocks
  // until the target object sends a reply.
  bool Transact(int32_t handle,
                const TransactionData& request,
                std::unique_ptr<TransactionData>* reply);

  // Increments the ref-count of a remote object specified by |handle|.
  void AddReference(int32_t handle);

  // Decrements the ref-count of a remote object specified by |handle|.
  void ReleaseReference(int32_t handle);

  // Returns a closure which decrements the ref-count of a remote object.
  // It's safe to run the returned closure even after the destruction of this
  // object.
  base::Closure GetReleaseReferenceClosure(int32_t handle);

  // CommandStream::IncomingCommandHandler override:
  bool OnTransaction(const TransactionData& data) override;
  void OnReply(std::unique_ptr<TransactionData> data) override;
  void OnDeadReply() override;
  void OnTransactionComplete() override;
  void OnIncrementWeakReference(void* ptr, void* cookie) override;
  void OnIncrementStrongReference(void* ptr, void* cookie) override;
  void OnDecrementStrongReference(void* ptr, void* cookie) override;
  void OnDecrementWeakReference(void* ptr, void* cookie) override;
  void OnFailedReply() override;

 private:
  enum ResponseType {
    RESPONSE_TYPE_NONE,
    RESPONSE_TYPE_TRANSACTION_COMPLETE,
    RESPONSE_TYPE_TRANSACTION_REPLY,
    RESPONSE_TYPE_FAILED,
    RESPONSE_TYPE_DEAD,
  };

  // Waits for a response to the previous transaction.
  ResponseType WaitForResponse(std::unique_ptr<TransactionData>* data);

  base::ThreadChecker thread_checker_;
  CommandStream command_stream_;

  ResponseType response_type_ = RESPONSE_TYPE_NONE;
  std::unique_ptr<TransactionData> response_data_;

  base::WeakPtrFactory<CommandBroker> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CommandBroker);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_COMMAND_BROKER_H_
