// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/bytes_consumer_for_data_consumer_handle.h"

#include <algorithm>

#include "base/location.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

BytesConsumerForDataConsumerHandle::BytesConsumerForDataConsumerHandle(
    ExecutionContext* execution_context,
    std::unique_ptr<WebDataConsumerHandle> handle)
    : execution_context_(execution_context),
      reader_(handle->ObtainReader(
          this,
          execution_context->GetTaskRunner(TaskType::kNetworking))) {}

BytesConsumerForDataConsumerHandle::~BytesConsumerForDataConsumerHandle() {}

BytesConsumer::Result BytesConsumerForDataConsumerHandle::BeginRead(
    const char** buffer,
    size_t* available) {
  DCHECK(!is_in_two_phase_read_);
  *buffer = nullptr;
  *available = 0;
  if (state_ == InternalState::kClosed)
    return Result::kDone;
  if (state_ == InternalState::kErrored)
    return Result::kError;

  WebDataConsumerHandle::Result r =
      reader_->BeginRead(reinterpret_cast<const void**>(buffer),
                         WebDataConsumerHandle::kFlagNone, available);
  switch (r) {
    case WebDataConsumerHandle::kOk:
      is_in_two_phase_read_ = true;
      return Result::kOk;
    case WebDataConsumerHandle::kShouldWait:
      return Result::kShouldWait;
    case WebDataConsumerHandle::kDone:
      Close();
      return Result::kDone;
    case WebDataConsumerHandle::kBusy:
    case WebDataConsumerHandle::kResourceExhausted:
    case WebDataConsumerHandle::kUnexpectedError:
      SetError();
      return Result::kError;
  }
  NOTREACHED();
  return Result::kError;
}

BytesConsumer::Result BytesConsumerForDataConsumerHandle::EndRead(size_t read) {
  DCHECK(is_in_two_phase_read_);
  is_in_two_phase_read_ = false;
  DCHECK(state_ == InternalState::kReadable ||
         state_ == InternalState::kWaiting);
  WebDataConsumerHandle::Result r = reader_->EndRead(read);
  if (r != WebDataConsumerHandle::kOk) {
    has_pending_notification_ = false;
    SetError();
    return Result::kError;
  }
  if (has_pending_notification_) {
    has_pending_notification_ = false;
    execution_context_->GetTaskRunner(TaskType::kNetworking)
        ->PostTask(FROM_HERE,
                   WTF::Bind(&BytesConsumerForDataConsumerHandle::Notify,
                             WrapPersistent(this)));
  }
  return Result::kOk;
}

void BytesConsumerForDataConsumerHandle::SetClient(
    BytesConsumer::Client* client) {
  DCHECK(!client_);
  DCHECK(client);
  if (state_ == InternalState::kReadable || state_ == InternalState::kWaiting)
    client_ = client;
}

void BytesConsumerForDataConsumerHandle::ClearClient() {
  client_ = nullptr;
}

void BytesConsumerForDataConsumerHandle::Cancel() {
  DCHECK(!is_in_two_phase_read_);
  if (state_ == InternalState::kReadable || state_ == InternalState::kWaiting) {
    // We don't want the client to be notified in this case.
    BytesConsumer::Client* client = client_;
    client_ = nullptr;
    Close();
    client_ = client;
  }
}

BytesConsumer::PublicState BytesConsumerForDataConsumerHandle::GetPublicState()
    const {
  return GetPublicStateFromInternalState(state_);
}

void BytesConsumerForDataConsumerHandle::DidGetReadable() {
  DCHECK(state_ == InternalState::kReadable ||
         state_ == InternalState::kWaiting);
  if (is_in_two_phase_read_) {
    has_pending_notification_ = true;
    return;
  }
  // Perform zero-length read to call check handle's status.
  size_t read_size;
  WebDataConsumerHandle::Result result =
      reader_->Read(nullptr, 0, WebDataConsumerHandle::kFlagNone, &read_size);
  BytesConsumer::Client* client = client_;
  switch (result) {
    case WebDataConsumerHandle::kOk:
    case WebDataConsumerHandle::kShouldWait:
      if (client)
        client->OnStateChange();
      return;
    case WebDataConsumerHandle::kDone:
      Close();
      if (client)
        client->OnStateChange();
      return;
    case WebDataConsumerHandle::kBusy:
    case WebDataConsumerHandle::kResourceExhausted:
    case WebDataConsumerHandle::kUnexpectedError:
      SetError();
      if (client)
        client->OnStateChange();
      return;
  }
  return;
}

void BytesConsumerForDataConsumerHandle::Trace(blink::Visitor* visitor) {
  visitor->Trace(execution_context_);
  visitor->Trace(client_);
  BytesConsumer::Trace(visitor);
}

void BytesConsumerForDataConsumerHandle::Close() {
  DCHECK(!is_in_two_phase_read_);
  if (state_ == InternalState::kClosed)
    return;
  DCHECK(state_ == InternalState::kReadable ||
         state_ == InternalState::kWaiting);
  state_ = InternalState::kClosed;
  reader_ = nullptr;
  ClearClient();
}

void BytesConsumerForDataConsumerHandle::SetError() {
  DCHECK(!is_in_two_phase_read_);
  if (state_ == InternalState::kErrored)
    return;
  DCHECK(state_ == InternalState::kReadable ||
         state_ == InternalState::kWaiting);
  state_ = InternalState::kErrored;
  reader_ = nullptr;
  error_ = Error("error");
  ClearClient();
}

void BytesConsumerForDataConsumerHandle::Notify() {
  if (state_ == InternalState::kClosed || state_ == InternalState::kErrored)
    return;
  DidGetReadable();
}

}  // namespace blink
