// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_MESSAGE_H_
#define MOJO_PUBLIC_CPP_BINDINGS_MESSAGE_H_

#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/component_export.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "mojo/public/cpp/bindings/lib/buffer.h"
#include "mojo/public/cpp/bindings/lib/message_internal.h"
#include "mojo/public/cpp/bindings/lib/unserialized_message_context.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"
#include "mojo/public/cpp/system/message.h"

namespace mojo {

class AssociatedGroupController;

using ReportBadMessageCallback =
    base::OnceCallback<void(const std::string& error)>;

// Message is a holder for the data and handles to be sent over a MessagePipe.
// Message owns its data and handles, but a consumer of Message is free to
// mutate the data and handles. The message's data is comprised of a header
// followed by payload.
class COMPONENT_EXPORT(MOJO_CPP_BINDINGS_BASE) Message {
 public:
  static const uint32_t kFlagExpectsResponse = 1 << 0;
  static const uint32_t kFlagIsResponse = 1 << 1;
  static const uint32_t kFlagIsSync = 1 << 2;

  // Constructs an uninitialized Message object.
  Message();

  // See the move-assignment operator below.
  Message(Message&& other);

  // Constructs a new message with an unserialized context attached. This
  // message may be serialized later if necessary.
  explicit Message(
      std::unique_ptr<internal::UnserializedMessageContext> context);

  // Constructs a new serialized Message object with optional handles attached.
  // This message is fully functional and may be exchanged for a
  // ScopedMessageHandle for transit over a message pipe. See TakeMojoMessage().
  //
  // If |handles| is non-null, any handles in |*handles| are attached to the
  // newly constructed message.
  //
  // Note that |payload_size| is only the initially known size of the message
  // payload, if any. The payload can be expanded after construction using the
  // interface returned by |payload_buffer()|.
  Message(uint32_t name,
          uint32_t flags,
          size_t payload_size,
          size_t payload_interface_id_count,
          std::vector<ScopedHandle>* handles);

  // Constructs a new serialized Message object from an existing
  // ScopedMessageHandle; e.g., one read from a message pipe.
  //
  // If the message had any handles attached, they will be extracted and
  // retrievable via |handles()|. Such messages may NOT be sent back over
  // another message pipe, but are otherwise safe to inspect and pass around.
  Message(ScopedMessageHandle handle);

  ~Message();

  // Moves |other| into a new Message object. The moved-from Message becomes
  // invalid and is effectively in a default-constructed state after this call.
  Message& operator=(Message&& other);

  // Resets the Message to an uninitialized state. Upon reset, the Message
  // exists as if it were default-constructed: it has no data buffer and owns no
  // handles.
  void Reset();

  // Indicates whether this Message is uninitialized.
  bool IsNull() const { return !handle_.is_valid(); }

  // Indicates whether this Message is serialized.
  bool is_serialized() const { return serialized_; }

  // Access the raw bytes of the message.
  const uint8_t* data() const {
    DCHECK(payload_buffer_.is_valid());
    return static_cast<const uint8_t*>(payload_buffer_.data());
  }
  uint8_t* mutable_data() { return const_cast<uint8_t*>(data()); }

  size_t data_num_bytes() const {
    DCHECK(payload_buffer_.is_valid());
    return payload_buffer_.cursor();
  }

  // Access the header.
  const internal::MessageHeader* header() const {
    return reinterpret_cast<const internal::MessageHeader*>(data());
  }
  internal::MessageHeader* header() {
    return reinterpret_cast<internal::MessageHeader*>(mutable_data());
  }

  const internal::MessageHeaderV1* header_v1() const {
    DCHECK_GE(version(), 1u);
    return reinterpret_cast<const internal::MessageHeaderV1*>(data());
  }
  internal::MessageHeaderV1* header_v1() {
    DCHECK_GE(version(), 1u);
    return reinterpret_cast<internal::MessageHeaderV1*>(mutable_data());
  }

  const internal::MessageHeaderV2* header_v2() const {
    DCHECK_GE(version(), 2u);
    return reinterpret_cast<const internal::MessageHeaderV2*>(data());
  }
  internal::MessageHeaderV2* header_v2() {
    DCHECK_GE(version(), 2u);
    return reinterpret_cast<internal::MessageHeaderV2*>(mutable_data());
  }

  uint32_t version() const { return header()->version; }

  uint32_t interface_id() const { return header()->interface_id; }
  void set_interface_id(uint32_t id) { header()->interface_id = id; }

  uint32_t name() const { return header()->name; }
  bool has_flag(uint32_t flag) const { return !!(header()->flags & flag); }

  // Access the request_id field (if present).
  uint64_t request_id() const { return header_v1()->request_id; }
  void set_request_id(uint64_t request_id) {
    header_v1()->request_id = request_id;
  }

  // Access the payload.
  const uint8_t* payload() const;
  uint8_t* mutable_payload() { return const_cast<uint8_t*>(payload()); }
  uint32_t payload_num_bytes() const;

  uint32_t payload_num_interface_ids() const;
  const uint32_t* payload_interface_ids() const;

  internal::Buffer* payload_buffer() { return &payload_buffer_; }

  // Access the handles of a received message. Note that these are unused on
  // outgoing messages.
  const std::vector<ScopedHandle>* handles() const { return &handles_; }
  std::vector<ScopedHandle>* mutable_handles() { return &handles_; }

  const std::vector<ScopedInterfaceEndpointHandle>*
  associated_endpoint_handles() const {
    return &associated_endpoint_handles_;
  }
  std::vector<ScopedInterfaceEndpointHandle>*
  mutable_associated_endpoint_handles() {
    return &associated_endpoint_handles_;
  }

  // Takes ownership of any handles within |*context| and attaches them to this
  // Message.
  void AttachHandlesFromSerializationContext(
      internal::SerializationContext* context);

  // Takes a scoped MessageHandle which may be passed to |WriteMessageNew()| for
  // transmission. Note that this invalidates this Message object, taking
  // ownership of its internal storage and any attached handles.
  ScopedMessageHandle TakeMojoMessage();

  // Notifies the system that this message is "bad," in this case meaning it was
  // rejected by bindings validation code.
  void NotifyBadMessage(const std::string& error);

  // Serializes |associated_endpoint_handles_| into the payload_interface_ids
  // field.
  void SerializeAssociatedEndpointHandles(
      AssociatedGroupController* group_controller);

  // Deserializes |associated_endpoint_handles_| from the payload_interface_ids
  // field.
  bool DeserializeAssociatedEndpointHandles(
      AssociatedGroupController* group_controller);

  // If this Message has an unserialized message context attached, force it to
  // be serialized immediately. Otherwise this does nothing.
  void SerializeIfNecessary();

  // Takes the unserialized message context from this Message if its tag matches
  // |tag|.
  std::unique_ptr<internal::UnserializedMessageContext> TakeUnserializedContext(
      const internal::UnserializedMessageContext::Tag* tag);

  template <typename MessageType>
  std::unique_ptr<MessageType> TakeUnserializedContext() {
    auto generic_context = TakeUnserializedContext(&MessageType::kMessageTag);
    if (!generic_context)
      return nullptr;
    return base::WrapUnique(
        generic_context.release()->template SafeCast<MessageType>());
  }

 private:
  ScopedMessageHandle handle_;

  // A Buffer which may be used to allocate blocks of data within the message
  // payload for reading or writing.
  internal::Buffer payload_buffer_;

  std::vector<ScopedHandle> handles_;
  std::vector<ScopedInterfaceEndpointHandle> associated_endpoint_handles_;

  // Indicates whether this Message object is transferable, i.e. can be sent
  // elsewhere. In general this is true unless |handle_| is invalid or
  // serialized handles have been extracted from the serialized message object
  // identified by |handle_|.
  bool transferable_ = false;

  // Indicates whether this Message object is serialized.
  bool serialized_ = false;

  DISALLOW_COPY_AND_ASSIGN(Message);
};

class COMPONENT_EXPORT(MOJO_CPP_BINDINGS_BASE) MessageReceiver {
 public:
  virtual ~MessageReceiver() {}

  // Indicates whether the receiver prefers to receive serialized messages.
  virtual bool PrefersSerializedMessages();

  // The receiver may mutate the given message.  Returns true if the message
  // was accepted and false otherwise, indicating that the message was invalid
  // or malformed.
  virtual bool Accept(Message* message) WARN_UNUSED_RESULT = 0;
};

class MessageReceiverWithResponder : public MessageReceiver {
 public:
  ~MessageReceiverWithResponder() override {}

  // A variant on Accept that registers a MessageReceiver (known as the
  // responder) to handle the response message generated from the given
  // message. The responder's Accept method may be called during
  // AcceptWithResponder or some time after its return.
  virtual bool AcceptWithResponder(Message* message,
                                   std::unique_ptr<MessageReceiver> responder)
      WARN_UNUSED_RESULT = 0;
};

// A MessageReceiver that is also able to provide status about the state
// of the underlying MessagePipe to which it will be forwarding messages
// received via the |Accept()| call.
class MessageReceiverWithStatus : public MessageReceiver {
 public:
  ~MessageReceiverWithStatus() override {}

  // Returns |true| if this MessageReceiver is currently bound to a MessagePipe,
  // the pipe has not been closed, and the pipe has not encountered an error.
  virtual bool IsConnected() = 0;

  // Determines if this MessageReceiver is still bound to a message pipe and has
  // not encountered any errors. This is asynchronous but may be called from any
  // sequence. |callback| is eventually invoked from an arbitrary sequence with
  // the result of the query.
  virtual void IsConnectedAsync(base::OnceCallback<void(bool)> callback) = 0;
};

// An alternative to MessageReceiverWithResponder for cases in which it
// is necessary for the implementor of this interface to know about the status
// of the MessagePipe which will carry the responses.
class MessageReceiverWithResponderStatus : public MessageReceiver {
 public:
  ~MessageReceiverWithResponderStatus() override {}

  // A variant on Accept that registers a MessageReceiverWithStatus (known as
  // the responder) to handle the response message generated from the given
  // message. Any of the responder's methods (Accept or IsValid) may be called
  // during  AcceptWithResponder or some time after its return.
  virtual bool AcceptWithResponder(Message* message,
                                   std::unique_ptr<MessageReceiverWithStatus>
                                       responder) WARN_UNUSED_RESULT = 0;
};

class COMPONENT_EXPORT(MOJO_CPP_BINDINGS_BASE) PassThroughFilter
    : public MessageReceiver {
 public:
  PassThroughFilter();
  ~PassThroughFilter() override;

  // MessageReceiver:
  bool Accept(Message* message) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PassThroughFilter);
};

namespace internal {
class SyncMessageResponseSetup;
}

// An object which should be constructed on the stack immediately before making
// a sync request for which the caller wishes to perform custom validation of
// the response value(s). It is illegal to make more than one sync call during
// the lifetime of the topmost SyncMessageResponseContext, but it is legal to
// nest contexts to support reentrancy.
//
// Usage should look something like:
//
//     SyncMessageResponseContext response_context;
//     foo_interface->SomeSyncCall(&response_value);
//     if (response_value.IsBad())
//       response_context.ReportBadMessage("Bad response_value!");
//
class COMPONENT_EXPORT(MOJO_CPP_BINDINGS_BASE) SyncMessageResponseContext {
 public:
  SyncMessageResponseContext();
  ~SyncMessageResponseContext();

  static SyncMessageResponseContext* current();

  void ReportBadMessage(const std::string& error);

  ReportBadMessageCallback GetBadMessageCallback();

 private:
  friend class internal::SyncMessageResponseSetup;

  SyncMessageResponseContext* outer_context_;
  Message response_;

  DISALLOW_COPY_AND_ASSIGN(SyncMessageResponseContext);
};

// Read a single message from the pipe. The caller should have created the
// Message, but not called Initialize(). Returns MOJO_RESULT_SHOULD_WAIT if
// the caller should wait on the handle to become readable. Returns
// MOJO_RESULT_OK if the message was read successfully and should be
// dispatched, otherwise returns an error code if something went wrong.
//
// NOTE: The message hasn't been validated and may be malformed!
COMPONENT_EXPORT(MOJO_CPP_BINDINGS_BASE)
MojoResult ReadMessage(MessagePipeHandle handle, Message* message);

// Reports the currently dispatching Message as bad. Note that this is only
// legal to call from directly within the stack frame of a message dispatch. If
// you need to do asynchronous work before you can determine the legitimacy of
// a message, use GetBadMessageCallback() and retain its result until you're
// ready to invoke or discard it.
COMPONENT_EXPORT(MOJO_CPP_BINDINGS_BASE)
void ReportBadMessage(const std::string& error);

// Acquires a callback which may be run to report the currently dispatching
// Message as bad. Note that this is only legal to call from directly within the
// stack frame of a message dispatch, but the returned callback may be called
// exactly once any time thereafter to report the message as bad. This may only
// be called once per message.
COMPONENT_EXPORT(MOJO_CPP_BINDINGS_BASE)
ReportBadMessageCallback GetBadMessageCallback();

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_MESSAGE_H_
