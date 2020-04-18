// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/compiler_specific.h"
#include "jingle/notifier/base/fake_base_task.h"
#include "jingle/notifier/base/weak_xmpp_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/libjingle_xmpp/xmpp/asyncsocket.h"

namespace notifier {

using ::testing::_;
using ::testing::Return;

class MockAsyncSocket : public buzz::AsyncSocket {
 public:
  ~MockAsyncSocket() override {}

  MOCK_METHOD0(state, State());
  MOCK_METHOD0(error, Error());
  MOCK_METHOD0(GetError, int());
  MOCK_METHOD1(Connect, bool(const rtc::SocketAddress&));
  MOCK_METHOD3(Read, bool(char*, size_t, size_t*));
  MOCK_METHOD2(Write, bool(const char*, size_t));
  MOCK_METHOD0(Close, bool());
  MOCK_METHOD1(StartTls, bool(const std::string&));
};

}  // namespace notifier

namespace {

// Extends WeakXmppClient to make jid() return a full jid, as required by
// PushNotificationsSubscribeTask.
class FakeWeakXmppClient : public notifier::WeakXmppClient {
 public:
  explicit FakeWeakXmppClient(rtc::TaskParent* parent)
      : notifier::WeakXmppClient(parent),
        jid_("test@example.com/testresource") {}

  ~FakeWeakXmppClient() override {}

  const buzz::Jid& jid() const override { return jid_; }

 private:
  buzz::Jid jid_;
};

}  // namespace

namespace notifier {

FakeBaseTask::FakeBaseTask() {
  // Owned by |task_pump_|.
  FakeWeakXmppClient* weak_xmpp_client =
      new FakeWeakXmppClient(&task_pump_);

  weak_xmpp_client->Start();
  buzz::XmppClientSettings settings;
  // Owned by |weak_xmpp_client|.
  MockAsyncSocket* mock_async_socket = new MockAsyncSocket();
  EXPECT_CALL(*mock_async_socket, Connect(_)).WillOnce(Return(true));
  weak_xmpp_client->Connect(settings, "en", mock_async_socket, NULL);
  // Initialize the XMPP client.
  task_pump_.RunTasks();

  base_task_ = weak_xmpp_client->AsWeakPtr();
}

FakeBaseTask::~FakeBaseTask() {}

base::WeakPtr<buzz::XmppTaskParentInterface> FakeBaseTask::AsWeakPtr() {
  return base_task_;
}

}  // namespace notifier
