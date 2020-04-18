// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/bytes_consumer_test_util.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace {
using Result = BytesConsumer::Result;
using testing::_;
using testing::ByMove;
using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;
}  // namespace

BytesConsumerTestUtil::MockBytesConsumer::MockBytesConsumer() {
  ON_CALL(*this, BeginRead(_, _))
      .WillByDefault(DoAll(SetArgPointee<0>(nullptr), SetArgPointee<1>(0),
                           Return(Result::kError)));
  ON_CALL(*this, EndRead(_)).WillByDefault(Return(Result::kError));
  ON_CALL(*this, GetPublicState()).WillByDefault(Return(PublicState::kErrored));
  ON_CALL(*this, DrainAsBlobDataHandle(_))
      .WillByDefault(Return(ByMove(nullptr)));
  ON_CALL(*this, DrainAsFormData()).WillByDefault(Return(ByMove(nullptr)));
}

BytesConsumerTestUtil::ReplayingBytesConsumer::ReplayingBytesConsumer(
    ExecutionContext* execution_context)
    : execution_context_(execution_context) {}

BytesConsumerTestUtil::ReplayingBytesConsumer::~ReplayingBytesConsumer() {}

Result BytesConsumerTestUtil::ReplayingBytesConsumer::BeginRead(
    const char** buffer,
    size_t* available) {
  ++notification_token_;
  if (commands_.IsEmpty()) {
    switch (state_) {
      case BytesConsumer::InternalState::kReadable:
      case BytesConsumer::InternalState::kWaiting:
        return Result::kShouldWait;
      case BytesConsumer::InternalState::kClosed:
        return Result::kDone;
      case BytesConsumer::InternalState::kErrored:
        return Result::kError;
    }
  }
  const Command& command = commands_[0];
  switch (command.GetName()) {
    case Command::kData:
      DCHECK_LE(offset_, command.Body().size());
      *buffer = command.Body().data() + offset_;
      *available = command.Body().size() - offset_;
      return Result::kOk;
    case Command::kDone:
      commands_.pop_front();
      Close();
      return Result::kDone;
    case Command::kError: {
      Error e(String::FromUTF8(command.Body().data(), command.Body().size()));
      commands_.pop_front();
      MakeErrored(std::move(e));
      return Result::kError;
    }
    case Command::kWait:
      commands_.pop_front();
      state_ = InternalState::kWaiting;
      execution_context_->GetTaskRunner(TaskType::kNetworking)
          ->PostTask(FROM_HERE,
                     WTF::Bind(&ReplayingBytesConsumer::NotifyAsReadable,
                               WrapPersistent(this), notification_token_));
      return Result::kShouldWait;
  }
  NOTREACHED();
  return Result::kError;
}

Result BytesConsumerTestUtil::ReplayingBytesConsumer::EndRead(size_t read) {
  DCHECK(!commands_.IsEmpty());
  const Command& command = commands_[0];
  DCHECK_EQ(Command::kData, command.GetName());
  offset_ += read;
  DCHECK_LE(offset_, command.Body().size());
  if (offset_ < command.Body().size())
    return Result::kOk;

  offset_ = 0;
  commands_.pop_front();
  return Result::kOk;
}

void BytesConsumerTestUtil::ReplayingBytesConsumer::SetClient(Client* client) {
  DCHECK(!client_);
  DCHECK(client);
  client_ = client;
  ++notification_token_;
}

void BytesConsumerTestUtil::ReplayingBytesConsumer::ClearClient() {
  DCHECK(client_);
  client_ = nullptr;
  ++notification_token_;
}

void BytesConsumerTestUtil::ReplayingBytesConsumer::Cancel() {
  Close();
  is_cancelled_ = true;
}

BytesConsumer::PublicState
BytesConsumerTestUtil::ReplayingBytesConsumer::GetPublicState() const {
  return GetPublicStateFromInternalState(state_);
}

BytesConsumer::Error BytesConsumerTestUtil::ReplayingBytesConsumer::GetError()
    const {
  return error_;
}

void BytesConsumerTestUtil::ReplayingBytesConsumer::NotifyAsReadable(
    int notification_token) {
  if (notification_token_ != notification_token) {
    // The notification is cancelled.
    return;
  }
  DCHECK(client_);
  DCHECK_NE(InternalState::kClosed, state_);
  DCHECK_NE(InternalState::kErrored, state_);
  client_->OnStateChange();
}

void BytesConsumerTestUtil::ReplayingBytesConsumer::Close() {
  commands_.clear();
  offset_ = 0;
  state_ = InternalState::kClosed;
  ++notification_token_;
}

void BytesConsumerTestUtil::ReplayingBytesConsumer::MakeErrored(
    const Error& e) {
  commands_.clear();
  offset_ = 0;
  error_ = e;
  state_ = InternalState::kErrored;
  ++notification_token_;
}

void BytesConsumerTestUtil::ReplayingBytesConsumer::Trace(
    blink::Visitor* visitor) {
  visitor->Trace(execution_context_);
  visitor->Trace(client_);
  BytesConsumer::Trace(visitor);
}

BytesConsumerTestUtil::TwoPhaseReader::TwoPhaseReader(BytesConsumer* consumer)
    : consumer_(consumer) {
  consumer_->SetClient(this);
}

void BytesConsumerTestUtil::TwoPhaseReader::OnStateChange() {
  while (true) {
    const char* buffer = nullptr;
    size_t available = 0;
    auto result = consumer_->BeginRead(&buffer, &available);
    if (result == BytesConsumer::Result::kShouldWait)
      return;
    if (result == BytesConsumer::Result::kOk) {
      // We don't use |available| as-is to test cases where endRead
      // is called with a number smaller than |available|. We choose 3
      // because of the same reasons as Reader::onStateChange.
      size_t read = std::min(static_cast<size_t>(3), available);
      data_.Append(buffer, read);
      result = consumer_->EndRead(read);
    }
    DCHECK_NE(result, BytesConsumer::Result::kShouldWait);
    if (result != BytesConsumer::Result::kOk) {
      result_ = result;
      return;
    }
  }
}

std::pair<BytesConsumer::Result, Vector<char>>
BytesConsumerTestUtil::TwoPhaseReader::Run() {
  OnStateChange();
  while (result_ != BytesConsumer::Result::kDone &&
         result_ != BytesConsumer::Result::kError)
    test::RunPendingTasks();
  test::RunPendingTasks();
  return std::make_pair(result_, std::move(data_));
}

String BytesConsumerTestUtil::CharVectorToString(const Vector<char>& v) {
  return String(v.data(), v.size());
}

}  // namespace blink
