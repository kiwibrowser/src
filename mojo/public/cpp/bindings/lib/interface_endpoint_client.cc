// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/interface_endpoint_client.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/stl_util.h"
#include "mojo/public/cpp/bindings/associated_group.h"
#include "mojo/public/cpp/bindings/associated_group_controller.h"
#include "mojo/public/cpp/bindings/interface_endpoint_controller.h"
#include "mojo/public/cpp/bindings/lib/task_runner_helper.h"
#include "mojo/public/cpp/bindings/lib/validation_util.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"

namespace mojo {

// ----------------------------------------------------------------------------

namespace {

void DetermineIfEndpointIsConnected(
    const base::WeakPtr<InterfaceEndpointClient>& client,
    base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(client && !client->encountered_error());
}

// When receiving an incoming message which expects a repsonse,
// InterfaceEndpointClient creates a ResponderThunk object and passes it to the
// incoming message receiver. When the receiver finishes processing the message,
// it can provide a response using this object.
class ResponderThunk : public MessageReceiverWithStatus {
 public:
  explicit ResponderThunk(
      const base::WeakPtr<InterfaceEndpointClient>& endpoint_client,
      scoped_refptr<base::SequencedTaskRunner> runner)
      : endpoint_client_(endpoint_client),
        accept_was_invoked_(false),
        task_runner_(std::move(runner)) {}
  ~ResponderThunk() override {
    if (!accept_was_invoked_) {
      // The Service handled a message that was expecting a response
      // but did not send a response.
      // We raise an error to signal the calling application that an error
      // condition occurred. Without this the calling application would have no
      // way of knowing it should stop waiting for a response.
      if (task_runner_->RunsTasksInCurrentSequence()) {
        // Please note that even if this code is run from a different task
        // runner on the same thread as |task_runner_|, it is okay to directly
        // call InterfaceEndpointClient::RaiseError(), because it will raise
        // error from the correct task runner asynchronously.
        if (endpoint_client_) {
          endpoint_client_->RaiseError();
        }
      } else {
        task_runner_->PostTask(
            FROM_HERE,
            base::Bind(&InterfaceEndpointClient::RaiseError, endpoint_client_));
      }
    }
  }

  // MessageReceiver implementation:
  bool PrefersSerializedMessages() override {
    return endpoint_client_ && endpoint_client_->PrefersSerializedMessages();
  }

  bool Accept(Message* message) override {
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    accept_was_invoked_ = true;
    DCHECK(message->has_flag(Message::kFlagIsResponse));

    bool result = false;

    if (endpoint_client_)
      result = endpoint_client_->Accept(message);

    return result;
  }

  // MessageReceiverWithStatus implementation:
  bool IsConnected() override {
    DCHECK(task_runner_->RunsTasksInCurrentSequence());
    return endpoint_client_ && !endpoint_client_->encountered_error();
  }

  void IsConnectedAsync(base::OnceCallback<void(bool)> callback) override {
    if (task_runner_->RunsTasksInCurrentSequence()) {
      DetermineIfEndpointIsConnected(endpoint_client_, std::move(callback));
    } else {
      task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&DetermineIfEndpointIsConnected,
                                    endpoint_client_, std::move(callback)));
    }
  }

 private:
  base::WeakPtr<InterfaceEndpointClient> endpoint_client_;
  bool accept_was_invoked_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ResponderThunk);
};

}  // namespace

// ----------------------------------------------------------------------------

InterfaceEndpointClient::SyncResponseInfo::SyncResponseInfo(
    bool* in_response_received)
    : response_received(in_response_received) {}

InterfaceEndpointClient::SyncResponseInfo::~SyncResponseInfo() {}

// ----------------------------------------------------------------------------

InterfaceEndpointClient::HandleIncomingMessageThunk::HandleIncomingMessageThunk(
    InterfaceEndpointClient* owner)
    : owner_(owner) {}

InterfaceEndpointClient::HandleIncomingMessageThunk::
    ~HandleIncomingMessageThunk() {}

bool InterfaceEndpointClient::HandleIncomingMessageThunk::Accept(
    Message* message) {
  return owner_->HandleValidatedMessage(message);
}

// ----------------------------------------------------------------------------

InterfaceEndpointClient::InterfaceEndpointClient(
    ScopedInterfaceEndpointHandle handle,
    MessageReceiverWithResponderStatus* receiver,
    std::unique_ptr<MessageReceiver> payload_validator,
    bool expect_sync_requests,
    scoped_refptr<base::SequencedTaskRunner> runner,
    uint32_t interface_version)
    : expect_sync_requests_(expect_sync_requests),
      handle_(std::move(handle)),
      incoming_receiver_(receiver),
      thunk_(this),
      filters_(&thunk_),
      task_runner_(std::move(runner)),
      control_message_proxy_(this),
      control_message_handler_(interface_version),
      weak_ptr_factory_(this) {
  DCHECK(handle_.is_valid());

  // TODO(yzshen): the way to use validator (or message filter in general)
  // directly is a little awkward.
  if (payload_validator)
    filters_.Append(std::move(payload_validator));

  if (handle_.pending_association()) {
    handle_.SetAssociationEventHandler(base::Bind(
        &InterfaceEndpointClient::OnAssociationEvent, base::Unretained(this)));
  } else {
    InitControllerIfNecessary();
  }
}

InterfaceEndpointClient::~InterfaceEndpointClient() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (controller_)
    handle_.group_controller()->DetachEndpointClient(handle_);
}

AssociatedGroup* InterfaceEndpointClient::associated_group() {
  if (!associated_group_)
    associated_group_ = std::make_unique<AssociatedGroup>(handle_);
  return associated_group_.get();
}

ScopedInterfaceEndpointHandle InterfaceEndpointClient::PassHandle() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!has_pending_responders());

  if (!handle_.is_valid())
    return ScopedInterfaceEndpointHandle();

  handle_.SetAssociationEventHandler(
      ScopedInterfaceEndpointHandle::AssociationEventCallback());

  if (controller_) {
    controller_ = nullptr;
    handle_.group_controller()->DetachEndpointClient(handle_);
  }

  return std::move(handle_);
}

void InterfaceEndpointClient::AddFilter(
    std::unique_ptr<MessageReceiver> filter) {
  filters_.Append(std::move(filter));
}

void InterfaceEndpointClient::RaiseError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!handle_.pending_association())
    handle_.group_controller()->RaiseError();
}

void InterfaceEndpointClient::CloseWithReason(uint32_t custom_reason,
                                              const std::string& description) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto handle = PassHandle();
  handle.ResetWithReason(custom_reason, description);
}

bool InterfaceEndpointClient::PrefersSerializedMessages() {
  auto* controller = handle_.group_controller();
  return controller && controller->PrefersSerializedMessages();
}

bool InterfaceEndpointClient::Accept(Message* message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!message->has_flag(Message::kFlagExpectsResponse));
  DCHECK(!handle_.pending_association());

  // This has to been done even if connection error has occurred. For example,
  // the message contains a pending associated request. The user may try to use
  // the corresponding associated interface pointer after sending this message.
  // That associated interface pointer has to join an associated group in order
  // to work properly.
  if (!message->associated_endpoint_handles()->empty())
    message->SerializeAssociatedEndpointHandles(handle_.group_controller());

  if (encountered_error_)
    return false;

  InitControllerIfNecessary();

  return controller_->SendMessage(message);
}

bool InterfaceEndpointClient::AcceptWithResponder(
    Message* message,
    std::unique_ptr<MessageReceiver> responder) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(message->has_flag(Message::kFlagExpectsResponse));
  DCHECK(!handle_.pending_association());

  // Please see comments in Accept().
  if (!message->associated_endpoint_handles()->empty())
    message->SerializeAssociatedEndpointHandles(handle_.group_controller());

  if (encountered_error_)
    return false;

  InitControllerIfNecessary();

  // Reserve 0 in case we want it to convey special meaning in the future.
  uint64_t request_id = next_request_id_++;
  if (request_id == 0)
    request_id = next_request_id_++;

  message->set_request_id(request_id);

  bool is_sync = message->has_flag(Message::kFlagIsSync);
  if (!controller_->SendMessage(message))
    return false;

  if (!is_sync) {
    async_responders_[request_id] = std::move(responder);
    return true;
  }

  SyncCallRestrictions::AssertSyncCallAllowed();

  bool response_received = false;
  sync_responses_.insert(std::make_pair(
      request_id, std::make_unique<SyncResponseInfo>(&response_received)));

  base::WeakPtr<InterfaceEndpointClient> weak_self =
      weak_ptr_factory_.GetWeakPtr();
  controller_->SyncWatch(&response_received);
  // Make sure that this instance hasn't been destroyed.
  if (weak_self) {
    DCHECK(base::ContainsKey(sync_responses_, request_id));
    auto iter = sync_responses_.find(request_id);
    DCHECK_EQ(&response_received, iter->second->response_received);
    if (response_received) {
      ignore_result(responder->Accept(&iter->second->response));
    } else {
      DVLOG(1) << "Mojo sync call returns without receiving a response. "
               << "Typcially it is because the interface has been "
               << "disconnected.";
    }
    sync_responses_.erase(iter);
  }

  return true;
}

bool InterfaceEndpointClient::HandleIncomingMessage(Message* message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return filters_.Accept(message);
}

void InterfaceEndpointClient::NotifyError(
    const base::Optional<DisconnectReason>& reason) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (encountered_error_)
    return;
  encountered_error_ = true;

  // Response callbacks may hold on to resource, and there's no need to keep
  // them alive any longer. Note that it's allowed that a pending response
  // callback may own this endpoint, so we simply move the responders onto the
  // stack here and let them be destroyed when the stack unwinds.
  AsyncResponderMap responders = std::move(async_responders_);

  control_message_proxy_.OnConnectionError();

  if (error_handler_) {
    std::move(error_handler_).Run();
  } else if (error_with_reason_handler_) {
    if (reason) {
      std::move(error_with_reason_handler_)
          .Run(reason->custom_reason, reason->description);
    } else {
      std::move(error_with_reason_handler_).Run(0, std::string());
    }
  }
}

void InterfaceEndpointClient::QueryVersion(
    const base::Callback<void(uint32_t)>& callback) {
  control_message_proxy_.QueryVersion(callback);
}

void InterfaceEndpointClient::RequireVersion(uint32_t version) {
  control_message_proxy_.RequireVersion(version);
}

void InterfaceEndpointClient::FlushForTesting() {
  control_message_proxy_.FlushForTesting();
}

void InterfaceEndpointClient::InitControllerIfNecessary() {
  if (controller_ || handle_.pending_association())
    return;

  controller_ = handle_.group_controller()->AttachEndpointClient(handle_, this,
                                                                 task_runner_);
  if (expect_sync_requests_)
    controller_->AllowWokenUpBySyncWatchOnSameThread();
}

void InterfaceEndpointClient::OnAssociationEvent(
    ScopedInterfaceEndpointHandle::AssociationEvent event) {
  if (event == ScopedInterfaceEndpointHandle::ASSOCIATED) {
    InitControllerIfNecessary();
  } else if (event ==
             ScopedInterfaceEndpointHandle::PEER_CLOSED_BEFORE_ASSOCIATION) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&InterfaceEndpointClient::NotifyError,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      handle_.disconnect_reason()));
  }
}

bool InterfaceEndpointClient::HandleValidatedMessage(Message* message) {
  DCHECK_EQ(handle_.id(), message->interface_id());

  if (encountered_error_) {
    // This message is received after error has been encountered. For associated
    // interfaces, this means the remote side sends a
    // PeerAssociatedEndpointClosed event but continues to send more messages
    // for the same interface. Close the pipe because this shouldn't happen.
    DVLOG(1) << "A message is received for an interface after it has been "
             << "disconnected. Closing the pipe.";
    return false;
  }

  if (message->has_flag(Message::kFlagExpectsResponse)) {
    std::unique_ptr<MessageReceiverWithStatus> responder =
        std::make_unique<ResponderThunk>(weak_ptr_factory_.GetWeakPtr(),
                                         task_runner_);
    if (mojo::internal::ControlMessageHandler::IsControlMessage(message)) {
      return control_message_handler_.AcceptWithResponder(message,
                                                          std::move(responder));
    } else {
      return incoming_receiver_->AcceptWithResponder(message,
                                                     std::move(responder));
    }
  } else if (message->has_flag(Message::kFlagIsResponse)) {
    uint64_t request_id = message->request_id();

    if (message->has_flag(Message::kFlagIsSync)) {
      auto it = sync_responses_.find(request_id);
      if (it == sync_responses_.end())
        return false;
      it->second->response = std::move(*message);
      *it->second->response_received = true;
      return true;
    }

    auto it = async_responders_.find(request_id);
    if (it == async_responders_.end())
      return false;
    std::unique_ptr<MessageReceiver> responder = std::move(it->second);
    async_responders_.erase(it);
    return responder->Accept(message);
  } else {
    if (mojo::internal::ControlMessageHandler::IsControlMessage(message))
      return control_message_handler_.Accept(message);

    return incoming_receiver_->Accept(message);
  }
}

}  // namespace mojo
