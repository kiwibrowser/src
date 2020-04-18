// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/host_change_notification_listener.h"

#include <set>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "remoting/base/constants.h"
#include "remoting/signaling/mock_signal_strategy.h"
#include "remoting/signaling/signaling_address.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmpp/constants.h"

using buzz::QName;
using buzz::XmlElement;

using testing::NotNull;
using testing::Return;

namespace remoting {

namespace {
const char kHostId[] = "0";
const char kTestJid[] = "user@gmail.com/chromoting123";
const char kTestBotJid[] = "remotingunittest@bot.talk.google.com";
}  // namespace

ACTION_P(AddListener, list) {
  list->insert(arg0);
}

ACTION_P(RemoveListener, list) {
  EXPECT_TRUE(list->find(arg0) != list->end());
  list->erase(arg0);
}


class HostChangeNotificationListenerTest : public testing::Test {
 protected:
  HostChangeNotificationListenerTest()
      : signal_strategy_(SignalingAddress(kTestJid)) {}
  class MockListener : public HostChangeNotificationListener::Listener {
   public:
    MOCK_METHOD0(OnHostDeleted, void());
  };

  void SetUp() override {
    EXPECT_CALL(signal_strategy_, AddListener(NotNull()))
        .WillRepeatedly(AddListener(&signal_strategy_listeners_));
    EXPECT_CALL(signal_strategy_, RemoveListener(NotNull()))
        .WillRepeatedly(RemoveListener(&signal_strategy_listeners_));

    host_change_notification_listener_.reset(new HostChangeNotificationListener(
        &mock_listener_, kHostId, &signal_strategy_, kTestBotJid));
  }

  void TearDown() override {
    host_change_notification_listener_.reset();
    EXPECT_TRUE(signal_strategy_listeners_.empty());
  }

  std::unique_ptr<XmlElement> GetNotificationStanza(std::string operation,
                                                    std::string hostId,
                                                    std::string botJid) {
    std::unique_ptr<XmlElement> stanza(new XmlElement(buzz::QN_IQ));
    stanza->AddAttr(QName(std::string(), "type"), "set");
    XmlElement* host_changed =
        new XmlElement(QName(kChromotingXmlNamespace, "host-changed"));
    host_changed->AddAttr(QName(kChromotingXmlNamespace, "operation"),
                          operation);
    host_changed->AddAttr(QName(kChromotingXmlNamespace, "hostid"), hostId);
    stanza->AddElement(host_changed);
    stanza->AddAttr(buzz::QN_FROM, botJid);
    stanza->AddAttr(buzz::QN_TO, kTestJid);
    return stanza;
  }

  MockListener mock_listener_;
  MockSignalStrategy signal_strategy_;
  std::set<SignalStrategy::Listener*> signal_strategy_listeners_;
  std::unique_ptr<HostChangeNotificationListener>
      host_change_notification_listener_;
  base::MessageLoop message_loop_;
};

TEST_F(HostChangeNotificationListenerTest, ReceiveValidNotification) {
  EXPECT_CALL(mock_listener_, OnHostDeleted())
      .WillOnce(Return());
  std::unique_ptr<XmlElement> stanza =
      GetNotificationStanza("delete", kHostId, kTestBotJid);
  host_change_notification_listener_->OnSignalStrategyIncomingStanza(
      stanza.get());
  message_loop_.task_runner()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  base::RunLoop().Run();
}

TEST_F(HostChangeNotificationListenerTest, ReceiveNotificationBeforeDelete) {
  EXPECT_CALL(mock_listener_, OnHostDeleted())
      .Times(0);
  std::unique_ptr<XmlElement> stanza =
      GetNotificationStanza("delete", kHostId, kTestBotJid);
  host_change_notification_listener_->OnSignalStrategyIncomingStanza(
      stanza.get());
  host_change_notification_listener_.reset();
  message_loop_.task_runner()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  base::RunLoop().Run();
}


TEST_F(HostChangeNotificationListenerTest, ReceiveInvalidHostIdNotification) {
  EXPECT_CALL(mock_listener_, OnHostDeleted())
      .Times(0);
  std::unique_ptr<XmlElement> stanza =
      GetNotificationStanza("delete", "1", kTestBotJid);
  host_change_notification_listener_->OnSignalStrategyIncomingStanza(
      stanza.get());
  message_loop_.task_runner()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  base::RunLoop().Run();
}

TEST_F(HostChangeNotificationListenerTest, ReceiveInvalidBotJidNotification) {
  EXPECT_CALL(mock_listener_, OnHostDeleted())
      .Times(0);
  std::unique_ptr<XmlElement> stanza = GetNotificationStanza(
      "delete", kHostId, "notremotingbot@bot.talk.google.com");
  host_change_notification_listener_->OnSignalStrategyIncomingStanza(
      stanza.get());
  message_loop_.task_runner()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  base::RunLoop().Run();
}

TEST_F(HostChangeNotificationListenerTest, ReceiveNonDeleteNotification) {
  EXPECT_CALL(mock_listener_, OnHostDeleted())
      .Times(0);
  std::unique_ptr<XmlElement> stanza =
      GetNotificationStanza("update", kHostId, kTestBotJid);
  host_change_notification_listener_->OnSignalStrategyIncomingStanza(
      stanza.get());
  message_loop_.task_runner()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  base::RunLoop().Run();
}

}  // namespace remoting
