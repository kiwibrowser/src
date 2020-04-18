// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/message.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/numerics/safe_math.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_local.h"
#include "mojo/public/cpp/bindings/associated_group_controller.h"
#include "mojo/public/cpp/bindings/lib/array_internal.h"
#include "mojo/public/cpp/bindings/lib/unserialized_message_context.h"

namespace mojo {

namespace {

base::LazyInstance<base::ThreadLocalPointer<internal::MessageDispatchContext>>::
    Leaky g_tls_message_dispatch_context = LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<base::ThreadLocalPointer<SyncMessageResponseContext>>::Leaky
    g_tls_sync_response_context = LAZY_INSTANCE_INITIALIZER;

void DoNotifyBadMessage(Message message, const std::string& error) {
  message.NotifyBadMessage(error);
}

template <typename HeaderType>
void AllocateHeaderFromBuffer(internal::Buffer* buffer, HeaderType** header) {
  *header = buffer->AllocateAndGet<HeaderType>();
  (*header)->num_bytes = sizeof(HeaderType);
}

void WriteMessageHeader(uint32_t name,
                        uint32_t flags,
                        size_t payload_interface_id_count,
                        internal::Buffer* payload_buffer) {
  if (payload_interface_id_count > 0) {
    // Version 2
    internal::MessageHeaderV2* header;
    AllocateHeaderFromBuffer(payload_buffer, &header);
    header->version = 2;
    header->name = name;
    header->flags = flags;
    // The payload immediately follows the header.
    header->payload.Set(header + 1);
  } else if (flags &
             (Message::kFlagExpectsResponse | Message::kFlagIsResponse)) {
    // Version 1
    internal::MessageHeaderV1* header;
    AllocateHeaderFromBuffer(payload_buffer, &header);
    header->version = 1;
    header->name = name;
    header->flags = flags;
  } else {
    internal::MessageHeader* header;
    AllocateHeaderFromBuffer(payload_buffer, &header);
    header->version = 0;
    header->name = name;
    header->flags = flags;
  }
}

void CreateSerializedMessageObject(uint32_t name,
                                   uint32_t flags,
                                   size_t payload_size,
                                   size_t payload_interface_id_count,
                                   std::vector<ScopedHandle>* handles,
                                   ScopedMessageHandle* out_handle,
                                   internal::Buffer* out_buffer) {
  ScopedMessageHandle handle;
  MojoResult rv = mojo::CreateMessage(&handle);
  DCHECK_EQ(MOJO_RESULT_OK, rv);
  DCHECK(handle.is_valid());

  void* buffer;
  uint32_t buffer_size;
  size_t total_size = internal::ComputeSerializedMessageSize(
      flags, payload_size, payload_interface_id_count);
  DCHECK(base::IsValueInRangeForNumericType<uint32_t>(total_size));
  DCHECK(!handles ||
         base::IsValueInRangeForNumericType<uint32_t>(handles->size()));
  rv = MojoAppendMessageData(
      handle->value(), static_cast<uint32_t>(total_size),
      handles ? reinterpret_cast<MojoHandle*>(handles->data()) : nullptr,
      handles ? static_cast<uint32_t>(handles->size()) : 0, nullptr, &buffer,
      &buffer_size);
  DCHECK_EQ(MOJO_RESULT_OK, rv);
  if (handles) {
    // Handle ownership has been taken by MojoAppendMessageData.
    for (size_t i = 0; i < handles->size(); ++i)
      ignore_result(handles->at(i).release());
  }

  internal::Buffer payload_buffer(handle.get(), total_size, buffer,
                                  buffer_size);

  // Make sure we zero the memory first!
  memset(payload_buffer.data(), 0, total_size);
  WriteMessageHeader(name, flags, payload_interface_id_count, &payload_buffer);

  *out_handle = std::move(handle);
  *out_buffer = std::move(payload_buffer);
}

void SerializeUnserializedContext(MojoMessageHandle message,
                                  uintptr_t context_value) {
  auto* context =
      reinterpret_cast<internal::UnserializedMessageContext*>(context_value);
  void* buffer;
  uint32_t buffer_size;
  MojoResult attach_result = MojoAppendMessageData(
      message, 0, nullptr, 0, nullptr, &buffer, &buffer_size);
  if (attach_result != MOJO_RESULT_OK)
    return;

  internal::Buffer payload_buffer(MessageHandle(message), 0, buffer,
                                  buffer_size);
  WriteMessageHeader(context->message_name(), context->message_flags(),
                     0 /* payload_interface_id_count */, &payload_buffer);

  // We need to copy additional header data which may have been set after
  // message construction, as this codepath may be reached at some arbitrary
  // time between message send and message dispatch.
  static_cast<internal::MessageHeader*>(buffer)->interface_id =
      context->header()->interface_id;
  if (context->header()->flags &
      (Message::kFlagExpectsResponse | Message::kFlagIsResponse)) {
    DCHECK_GE(context->header()->version, 1u);
    static_cast<internal::MessageHeaderV1*>(buffer)->request_id =
        context->header()->request_id;
  }

  internal::SerializationContext serialization_context;
  context->Serialize(&serialization_context, &payload_buffer);

  // TODO(crbug.com/753433): Support lazy serialization of associated endpoint
  // handles. See corresponding TODO in the bindings generator for proof that
  // this DCHECK is indeed valid.
  DCHECK(serialization_context.associated_endpoint_handles()->empty());
  if (!serialization_context.handles()->empty())
    payload_buffer.AttachHandles(serialization_context.mutable_handles());
  payload_buffer.Seal();
}

void DestroyUnserializedContext(uintptr_t context) {
  delete reinterpret_cast<internal::UnserializedMessageContext*>(context);
}

ScopedMessageHandle CreateUnserializedMessageObject(
    std::unique_ptr<internal::UnserializedMessageContext> context) {
  ScopedMessageHandle handle;
  MojoResult rv = mojo::CreateMessage(&handle);
  DCHECK_EQ(MOJO_RESULT_OK, rv);
  DCHECK(handle.is_valid());

  rv = MojoSetMessageContext(
      handle->value(), reinterpret_cast<uintptr_t>(context.release()),
      &SerializeUnserializedContext, &DestroyUnserializedContext, nullptr);
  DCHECK_EQ(MOJO_RESULT_OK, rv);
  return handle;
}

}  // namespace

Message::Message() = default;

Message::Message(Message&& other)
    : handle_(std::move(other.handle_)),
      payload_buffer_(std::move(other.payload_buffer_)),
      handles_(std::move(other.handles_)),
      associated_endpoint_handles_(
          std::move(other.associated_endpoint_handles_)),
      transferable_(other.transferable_),
      serialized_(other.serialized_) {
  other.transferable_ = false;
  other.serialized_ = false;
}

Message::Message(std::unique_ptr<internal::UnserializedMessageContext> context)
    : Message(CreateUnserializedMessageObject(std::move(context))) {}

Message::Message(uint32_t name,
                 uint32_t flags,
                 size_t payload_size,
                 size_t payload_interface_id_count,
                 std::vector<ScopedHandle>* handles) {
  CreateSerializedMessageObject(name, flags, payload_size,
                                payload_interface_id_count, handles, &handle_,
                                &payload_buffer_);
  transferable_ = true;
  serialized_ = true;
}

Message::Message(ScopedMessageHandle handle) {
  DCHECK(handle.is_valid());

  uintptr_t context_value = 0;
  MojoResult get_context_result =
      MojoGetMessageContext(handle->value(), nullptr, &context_value);
  if (get_context_result == MOJO_RESULT_NOT_FOUND) {
    // It's a serialized message. Extract handles if possible.
    uint32_t num_bytes;
    void* buffer;
    uint32_t num_handles = 0;
    MojoResult rv = MojoGetMessageData(handle->value(), nullptr, &buffer,
                                       &num_bytes, nullptr, &num_handles);
    if (rv == MOJO_RESULT_RESOURCE_EXHAUSTED) {
      handles_.resize(num_handles);
      rv = MojoGetMessageData(handle->value(), nullptr, &buffer, &num_bytes,
                              reinterpret_cast<MojoHandle*>(handles_.data()),
                              &num_handles);
    } else {
      // No handles, so it's safe to retransmit this message if the caller
      // really wants to.
      transferable_ = true;
    }

    if (rv != MOJO_RESULT_OK) {
      // Failed to deserialize handles. Leave the Message uninitialized.
      return;
    }

    payload_buffer_ = internal::Buffer(buffer, num_bytes, num_bytes);
    serialized_ = true;
  } else {
    DCHECK_EQ(MOJO_RESULT_OK, get_context_result);
    auto* context =
        reinterpret_cast<internal::UnserializedMessageContext*>(context_value);
    // Dummy data address so common header accessors still behave properly. The
    // choice is V1 reflects unserialized message capabilities: we may or may
    // not need to support request IDs (which require at least V1), but we never
    // (for now, anyway) need to support associated interface handles (V2).
    payload_buffer_ =
        internal::Buffer(context->header(), sizeof(internal::MessageHeaderV1),
                         sizeof(internal::MessageHeaderV1));
    transferable_ = true;
    serialized_ = false;
  }

  handle_ = std::move(handle);
}

Message::~Message() = default;

Message& Message::operator=(Message&& other) {
  handle_ = std::move(other.handle_);
  payload_buffer_ = std::move(other.payload_buffer_);
  handles_ = std::move(other.handles_);
  associated_endpoint_handles_ = std::move(other.associated_endpoint_handles_);
  transferable_ = other.transferable_;
  other.transferable_ = false;
  serialized_ = other.serialized_;
  other.serialized_ = false;
  return *this;
}

void Message::Reset() {
  handle_.reset();
  payload_buffer_.Reset();
  handles_.clear();
  associated_endpoint_handles_.clear();
  transferable_ = false;
  serialized_ = false;
}

const uint8_t* Message::payload() const {
  if (version() < 2)
    return data() + header()->num_bytes;

  DCHECK(!header_v2()->payload.is_null());
  return static_cast<const uint8_t*>(header_v2()->payload.Get());
}

uint32_t Message::payload_num_bytes() const {
  DCHECK_GE(data_num_bytes(), header()->num_bytes);
  size_t num_bytes;
  if (version() < 2) {
    num_bytes = data_num_bytes() - header()->num_bytes;
  } else {
    auto payload_begin =
        reinterpret_cast<uintptr_t>(header_v2()->payload.Get());
    auto payload_end =
        reinterpret_cast<uintptr_t>(header_v2()->payload_interface_ids.Get());
    if (!payload_end)
      payload_end = reinterpret_cast<uintptr_t>(data() + data_num_bytes());
    DCHECK_GE(payload_end, payload_begin);
    num_bytes = payload_end - payload_begin;
  }
  DCHECK(base::IsValueInRangeForNumericType<uint32_t>(num_bytes));
  return static_cast<uint32_t>(num_bytes);
}

uint32_t Message::payload_num_interface_ids() const {
  auto* array_pointer =
      version() < 2 ? nullptr : header_v2()->payload_interface_ids.Get();
  return array_pointer ? static_cast<uint32_t>(array_pointer->size()) : 0;
}

const uint32_t* Message::payload_interface_ids() const {
  auto* array_pointer =
      version() < 2 ? nullptr : header_v2()->payload_interface_ids.Get();
  return array_pointer ? array_pointer->storage() : nullptr;
}

void Message::AttachHandlesFromSerializationContext(
    internal::SerializationContext* context) {
  if (context->handles()->empty() &&
      context->associated_endpoint_handles()->empty()) {
    // No handles attached, so no extra serialization work.
    return;
  }

  if (context->associated_endpoint_handles()->empty()) {
    // Attaching only non-associated handles is easier since we don't have to
    // modify the message header. Faster path for that.
    payload_buffer_.AttachHandles(context->mutable_handles());
    return;
  }

  // Allocate a new message with enough space to hold all attached handles. Copy
  // this message's contents into the new one and use it to replace ourself.
  //
  // TODO(rockot): We could avoid the extra full message allocation by instead
  // growing the buffer and carefully moving its contents around. This errs on
  // the side of less complexity with probably only marginal performance cost.
  uint32_t payload_size = payload_num_bytes();
  mojo::Message new_message(name(), header()->flags, payload_size,
                            context->associated_endpoint_handles()->size(),
                            context->mutable_handles());
  std::swap(*context->mutable_associated_endpoint_handles(),
            new_message.associated_endpoint_handles_);
  memcpy(new_message.payload_buffer()->AllocateAndGet(payload_size), payload(),
         payload_size);
  *this = std::move(new_message);
}

ScopedMessageHandle Message::TakeMojoMessage() {
  // If there are associated endpoints transferred,
  // SerializeAssociatedEndpointHandles() must be called before this method.
  DCHECK(associated_endpoint_handles_.empty());
  DCHECK(transferable_);
  payload_buffer_.Seal();
  auto handle = std::move(handle_);
  Reset();
  return handle;
}

void Message::NotifyBadMessage(const std::string& error) {
  DCHECK(handle_.is_valid());
  mojo::NotifyBadMessage(handle_.get(), error);
}

void Message::SerializeAssociatedEndpointHandles(
    AssociatedGroupController* group_controller) {
  if (associated_endpoint_handles_.empty())
    return;

  DCHECK_GE(version(), 2u);
  DCHECK(header_v2()->payload_interface_ids.is_null());
  DCHECK(payload_buffer_.is_valid());
  DCHECK(handle_.is_valid());

  size_t size = associated_endpoint_handles_.size();

  internal::Array_Data<uint32_t>::BufferWriter handle_writer;
  handle_writer.Allocate(size, &payload_buffer_);
  header_v2()->payload_interface_ids.Set(handle_writer.data());

  for (size_t i = 0; i < size; ++i) {
    ScopedInterfaceEndpointHandle& handle = associated_endpoint_handles_[i];

    DCHECK(handle.pending_association());
    handle_writer->storage()[i] =
        group_controller->AssociateInterface(std::move(handle));
  }
  associated_endpoint_handles_.clear();
}

bool Message::DeserializeAssociatedEndpointHandles(
    AssociatedGroupController* group_controller) {
  if (!serialized_)
    return true;

  associated_endpoint_handles_.clear();

  uint32_t num_ids = payload_num_interface_ids();
  if (num_ids == 0)
    return true;

  associated_endpoint_handles_.reserve(num_ids);
  uint32_t* ids = header_v2()->payload_interface_ids.Get()->storage();
  bool result = true;
  for (uint32_t i = 0; i < num_ids; ++i) {
    auto handle = group_controller->CreateLocalEndpointHandle(ids[i]);
    if (IsValidInterfaceId(ids[i]) && !handle.is_valid()) {
      // |ids[i]| itself is valid but handle creation failed. In that case, mark
      // deserialization as failed but continue to deserialize the rest of
      // handles.
      result = false;
    }

    associated_endpoint_handles_.push_back(std::move(handle));
    ids[i] = kInvalidInterfaceId;
  }
  return result;
}

void Message::SerializeIfNecessary() {
  MojoResult rv = MojoSerializeMessage(handle_->value(), nullptr);
  if (rv == MOJO_RESULT_FAILED_PRECONDITION)
    return;

  // Reconstruct this Message instance from the serialized message's handle.
  *this = Message(std::move(handle_));
}

std::unique_ptr<internal::UnserializedMessageContext>
Message::TakeUnserializedContext(
    const internal::UnserializedMessageContext::Tag* tag) {
  DCHECK(handle_.is_valid());
  uintptr_t context_value = 0;
  MojoResult rv =
      MojoGetMessageContext(handle_->value(), nullptr, &context_value);
  if (rv == MOJO_RESULT_NOT_FOUND)
    return nullptr;
  DCHECK_EQ(MOJO_RESULT_OK, rv);

  auto* context =
      reinterpret_cast<internal::UnserializedMessageContext*>(context_value);
  if (context->tag() != tag)
    return nullptr;

  // Detach the context from the message.
  rv = MojoSetMessageContext(handle_->value(), 0, nullptr, nullptr, nullptr);
  DCHECK_EQ(MOJO_RESULT_OK, rv);
  return base::WrapUnique(context);
}

bool MessageReceiver::PrefersSerializedMessages() {
  return false;
}

PassThroughFilter::PassThroughFilter() {}

PassThroughFilter::~PassThroughFilter() {}

bool PassThroughFilter::Accept(Message* message) {
  return true;
}

SyncMessageResponseContext::SyncMessageResponseContext()
    : outer_context_(current()) {
  g_tls_sync_response_context.Get().Set(this);
}

SyncMessageResponseContext::~SyncMessageResponseContext() {
  DCHECK_EQ(current(), this);
  g_tls_sync_response_context.Get().Set(outer_context_);
}

// static
SyncMessageResponseContext* SyncMessageResponseContext::current() {
  return g_tls_sync_response_context.Get().Get();
}

void SyncMessageResponseContext::ReportBadMessage(const std::string& error) {
  GetBadMessageCallback().Run(error);
}

ReportBadMessageCallback SyncMessageResponseContext::GetBadMessageCallback() {
  DCHECK(!response_.IsNull());
  return base::BindOnce(&DoNotifyBadMessage, std::move(response_));
}

MojoResult ReadMessage(MessagePipeHandle handle, Message* message) {
  ScopedMessageHandle message_handle;
  MojoResult rv =
      ReadMessageNew(handle, &message_handle, MOJO_READ_MESSAGE_FLAG_NONE);
  if (rv != MOJO_RESULT_OK)
    return rv;

  *message = Message(std::move(message_handle));
  return MOJO_RESULT_OK;
}

void ReportBadMessage(const std::string& error) {
  internal::MessageDispatchContext* context =
      internal::MessageDispatchContext::current();
  DCHECK(context);
  context->GetBadMessageCallback().Run(error);
}

ReportBadMessageCallback GetBadMessageCallback() {
  internal::MessageDispatchContext* context =
      internal::MessageDispatchContext::current();
  DCHECK(context);
  return context->GetBadMessageCallback();
}

namespace internal {

MessageHeaderV2::MessageHeaderV2() = default;

MessageDispatchContext::MessageDispatchContext(Message* message)
    : outer_context_(current()), message_(message) {
  g_tls_message_dispatch_context.Get().Set(this);
}

MessageDispatchContext::~MessageDispatchContext() {
  DCHECK_EQ(current(), this);
  g_tls_message_dispatch_context.Get().Set(outer_context_);
}

// static
MessageDispatchContext* MessageDispatchContext::current() {
  return g_tls_message_dispatch_context.Get().Get();
}

ReportBadMessageCallback MessageDispatchContext::GetBadMessageCallback() {
  DCHECK(!message_->IsNull());
  return base::BindOnce(&DoNotifyBadMessage, std::move(*message_));
}

// static
void SyncMessageResponseSetup::SetCurrentSyncResponseMessage(Message* message) {
  SyncMessageResponseContext* context = SyncMessageResponseContext::current();
  if (context)
    context->response_ = std::move(*message);
}

}  // namespace internal

}  // namespace mojo
