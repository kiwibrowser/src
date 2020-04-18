// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/presentation/presentation_receiver.h"

#include <memory>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/modules/presentation/presentation_connection.h"
#include "third_party/blink/renderer/modules/presentation/presentation_connection_list.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"
#include "v8/include/v8.h"

namespace blink {

class MockEventListenerForPresentationReceiver : public EventListener {
 public:
  MockEventListenerForPresentationReceiver()
      : EventListener(kCPPEventListenerType) {}

  bool operator==(const EventListener& other) const final {
    return this == &other;
  }

  MOCK_METHOD2(handleEvent, void(ExecutionContext* executionContext, Event*));
};

class PresentationReceiverTest : public testing::Test {
 public:
  PresentationReceiverTest()
      : connection_info_(KURL("http://example.com"), "id") {}
  void AddConnectionavailableEventListener(EventListener*,
                                           const PresentationReceiver*);
  void VerifyConnectionListPropertyState(ScriptPromisePropertyBase::State,
                                         const PresentationReceiver*);
  void VerifyConnectionListSize(size_t expected_size,
                                const PresentationReceiver*);

 protected:
  void SetUp() override {
    controller_connection_request_ = mojo::MakeRequest(&controller_connection_);
    receiver_connection_request_ = mojo::MakeRequest(&receiver_connection_);
  }

  mojom::blink::PresentationInfo connection_info_;
  mojom::blink::PresentationConnectionRequest controller_connection_request_;
  mojom::blink::PresentationConnectionPtr controller_connection_;
  mojom::blink::PresentationConnectionRequest receiver_connection_request_;
  mojom::blink::PresentationConnectionPtr receiver_connection_;
};

void PresentationReceiverTest::AddConnectionavailableEventListener(
    EventListener* event_handler,
    const PresentationReceiver* receiver) {
  receiver->connection_list_->addEventListener(
      EventTypeNames::connectionavailable, event_handler);
}

void PresentationReceiverTest::VerifyConnectionListPropertyState(
    ScriptPromisePropertyBase::State expected_state,
    const PresentationReceiver* receiver) {
  EXPECT_EQ(expected_state, receiver->connection_list_property_->GetState());
}

void PresentationReceiverTest::VerifyConnectionListSize(
    size_t expected_size,
    const PresentationReceiver* receiver) {
  EXPECT_EQ(expected_size, receiver->connection_list_->connections_.size());
}

using testing::StrictMock;

TEST_F(PresentationReceiverTest, NoConnectionUnresolvedConnectionList) {
  V8TestingScope scope;
  auto* receiver = new PresentationReceiver(&scope.GetFrame());

  auto* event_handler =
      new StrictMock<MockEventListenerForPresentationReceiver>();
  AddConnectionavailableEventListener(event_handler, receiver);
  EXPECT_CALL(*event_handler, handleEvent(testing::_, testing::_)).Times(0);

  receiver->connectionList(scope.GetScriptState());

  VerifyConnectionListPropertyState(ScriptPromisePropertyBase::kPending,
                                    receiver);
  VerifyConnectionListSize(0, receiver);
}

TEST_F(PresentationReceiverTest, OneConnectionResolvedConnectionListNoEvent) {
  V8TestingScope scope;
  auto* receiver = new PresentationReceiver(&scope.GetFrame());

  auto* event_handler =
      new StrictMock<MockEventListenerForPresentationReceiver>();
  AddConnectionavailableEventListener(event_handler, receiver);
  EXPECT_CALL(*event_handler, handleEvent(testing::_, testing::_)).Times(0);

  receiver->connectionList(scope.GetScriptState());

  // Receive first connection.
  receiver->OnReceiverConnectionAvailable(
      connection_info_.Clone(), std::move(controller_connection_),
      std::move(receiver_connection_request_));

  VerifyConnectionListPropertyState(ScriptPromisePropertyBase::kResolved,
                                    receiver);
  VerifyConnectionListSize(1, receiver);
}

TEST_F(PresentationReceiverTest, TwoConnectionsFireOnconnectionavailableEvent) {
  V8TestingScope scope;
  auto* receiver = new PresentationReceiver(&scope.GetFrame());

  StrictMock<MockEventListenerForPresentationReceiver>* event_handler =
      new StrictMock<MockEventListenerForPresentationReceiver>();
  AddConnectionavailableEventListener(event_handler, receiver);
  EXPECT_CALL(*event_handler, handleEvent(testing::_, testing::_)).Times(1);

  receiver->connectionList(scope.GetScriptState());

  // Receive first connection.
  receiver->OnReceiverConnectionAvailable(
      connection_info_.Clone(), std::move(controller_connection_),
      std::move(receiver_connection_request_));

  mojom::blink::PresentationConnectionPtr controller_connection_2;
  mojom::blink::PresentationConnectionPtr receiver_connection_2;
  mojom::blink::PresentationConnectionRequest controller_connection_request_2 =
      mojo::MakeRequest(&controller_connection_2);
  mojom::blink::PresentationConnectionRequest receiver_connection_request_2 =
      mojo::MakeRequest(&receiver_connection_2);

  // Receive second connection.
  receiver->OnReceiverConnectionAvailable(
      connection_info_.Clone(), std::move(controller_connection_2),
      std::move(receiver_connection_request_2));

  VerifyConnectionListSize(2, receiver);
}

TEST_F(PresentationReceiverTest, TwoConnectionsNoEvent) {
  V8TestingScope scope;
  auto* receiver = new PresentationReceiver(&scope.GetFrame());

  StrictMock<MockEventListenerForPresentationReceiver>* event_handler =
      new StrictMock<MockEventListenerForPresentationReceiver>();
  AddConnectionavailableEventListener(event_handler, receiver);
  EXPECT_CALL(*event_handler, handleEvent(testing::_, testing::_)).Times(0);

  // Receive first connection.
  receiver->OnReceiverConnectionAvailable(
      connection_info_.Clone(), std::move(controller_connection_),
      std::move(receiver_connection_request_));

  mojom::blink::PresentationConnectionPtr controller_connection_2;
  mojom::blink::PresentationConnectionPtr receiver_connection_2;
  mojom::blink::PresentationConnectionRequest controller_connection_request_2 =
      mojo::MakeRequest(&controller_connection_2);
  mojom::blink::PresentationConnectionRequest receiver_connection_request_2 =
      mojo::MakeRequest(&receiver_connection_2);

  // Receive second connection.
  receiver->OnReceiverConnectionAvailable(
      connection_info_.Clone(), std::move(controller_connection_2),
      std::move(receiver_connection_request_2));

  receiver->connectionList(scope.GetScriptState());
  VerifyConnectionListPropertyState(ScriptPromisePropertyBase::kResolved,
                                    receiver);
  VerifyConnectionListSize(2, receiver);
}

}  // namespace blink
