// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_notifications.h"

#include <memory>

#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/notifications/notification_display_service_tester.h"
#include "chrome/browser/notifications/notification_test_util.h"
#include "chrome/browser/printing/cloud_print/privet_http_asynchronous_factory.h"
#include "chrome/browser/printing/cloud_print/privet_http_impl.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::StrictMock;

using ::testing::_;
using ::testing::SaveArg;

namespace cloud_print {

namespace {

const char kExampleDeviceName[] = "test._privet._tcp.local";
const char kExampleDeviceHumanName[] = "Test device";
const char kExampleDeviceDescription[] = "Testing testing";
const char kExampleDeviceID[] = "__test__id";
const char kDeviceInfoURL[] = "http://1.2.3.4:8080/privet/info";

const char kInfoResponseUptime20[] = "{\"uptime\": 20}";
const char kInfoResponseUptime3600[] = "{\"uptime\": 3600}";
const char kInfoResponseNoUptime[] = "{}";

class MockPrivetNotificationsListenerDeleagate
    : public PrivetNotificationsListener::Delegate {
 public:
  MOCK_METHOD2(PrivetNotify, void(int devices_active, bool added));
  MOCK_METHOD0(PrivetRemoveNotification, void());
};

class MockPrivetHttpFactory : public PrivetHTTPAsynchronousFactory {
 public:
  class MockResolution : public PrivetHTTPResolution {
   public:
    MockResolution(const std::string& name,
                   net::URLRequestContextGetter* request_context)
        : name_(name), request_context_(request_context) {}

    ~MockResolution() override {}

    void Start(const net::HostPortPair& address,
               const ResultCallback& callback) override {
      callback.Run(std::unique_ptr<PrivetHTTPClient>(new PrivetHTTPClientImpl(
          name_, net::HostPortPair("1.2.3.4", 8080), request_context_.get())));
    }

    void Start(const ResultCallback& callback) override {
      Start(net::HostPortPair(), callback);
    }

    const std::string& GetName() override { return name_; }

   private:
    std::string name_;
    scoped_refptr<net::URLRequestContextGetter> request_context_;
  };

  explicit MockPrivetHttpFactory(net::URLRequestContextGetter* request_context)
      : request_context_(request_context) {}

  std::unique_ptr<PrivetHTTPResolution> CreatePrivetHTTP(
      const std::string& name) override {
    return std::make_unique<MockResolution>(name, request_context_.get());
  }

 private:
  scoped_refptr<net::URLRequestContextGetter> request_context_;
};

class PrivetNotificationsListenerTest : public testing::Test {
 public:
  PrivetNotificationsListenerTest()
      : request_context_(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())) {
    notification_listener_.reset(new PrivetNotificationsListener(
        std::unique_ptr<PrivetHTTPAsynchronousFactory>(
            new MockPrivetHttpFactory(request_context_.get())),
        &mock_delegate_));

    description_.name = kExampleDeviceHumanName;
    description_.description = kExampleDeviceDescription;
  }

  ~PrivetNotificationsListenerTest() override {}

  bool SuccessfulResponseToInfo(const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    if (!fetcher || fetcher->GetOriginalURL() != kDeviceInfoURL)
      return false;

    fetcher->SetResponseString(response);
    fetcher->set_status(
        net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
    fetcher->set_response_code(200);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    return true;
  }

 protected:
  StrictMock<MockPrivetNotificationsListenerDeleagate> mock_delegate_;
  std::unique_ptr<PrivetNotificationsListener> notification_listener_;
  base::MessageLoop message_loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  net::TestURLFetcherFactory fetcher_factory_;
  DeviceDescription description_;
};

TEST_F(PrivetNotificationsListenerTest, DisappearReappearTest) {
  EXPECT_CALL(mock_delegate_, PrivetNotify(1, true));
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
  EXPECT_TRUE(SuccessfulResponseToInfo(kInfoResponseUptime20));

  EXPECT_CALL(mock_delegate_, PrivetRemoveNotification());
  notification_listener_->DeviceRemoved(kExampleDeviceName);
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
  description_.id = kExampleDeviceID;
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
}

TEST_F(PrivetNotificationsListenerTest, RegisterTest) {
  EXPECT_CALL(mock_delegate_, PrivetNotify(1, true));
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
  EXPECT_TRUE(SuccessfulResponseToInfo(kInfoResponseUptime20));

  EXPECT_CALL(mock_delegate_, PrivetRemoveNotification());
  description_.id = kExampleDeviceID;
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
}

TEST_F(PrivetNotificationsListenerTest, RepeatedNotification) {
  EXPECT_CALL(mock_delegate_, PrivetNotify(1, true));
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
  EXPECT_TRUE(SuccessfulResponseToInfo(kInfoResponseUptime20));

  EXPECT_CALL(mock_delegate_, PrivetNotify(_, _)).Times(0);
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);

  EXPECT_CALL(mock_delegate_, PrivetRemoveNotification());
  notification_listener_->DeviceRemoved(kExampleDeviceName);

  EXPECT_CALL(mock_delegate_, PrivetNotify(_, _)).Times(0);
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);

  EXPECT_CALL(mock_delegate_, PrivetRemoveNotification()).Times(0);
  notification_listener_->DeviceRemoved(kExampleDeviceName);
}

TEST_F(PrivetNotificationsListenerTest, HighUptimeTest) {
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
  EXPECT_TRUE(SuccessfulResponseToInfo(kInfoResponseUptime3600));
  description_.id = kExampleDeviceID;
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
}

TEST_F(PrivetNotificationsListenerTest, HTTPErrorTest) {
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  fetcher->set_response_code(200);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

TEST_F(PrivetNotificationsListenerTest, DictionaryErrorTest) {
  notification_listener_->DeviceChanged(kExampleDeviceName, description_);
  SuccessfulResponseToInfo(kInfoResponseNoUptime);
}

class TestPrivetNotificationService;

class TestPrivetNotificationDelegate : public PrivetNotificationDelegate {
 public:
  TestPrivetNotificationDelegate(TestPrivetNotificationService* service,
                                 Profile* profile)
      : PrivetNotificationDelegate(profile), service_(service) {}

 private:
  // Refcounted.
  ~TestPrivetNotificationDelegate() override {}

  // PrivetNotificationDelegate:
  void OpenTab(const GURL& url) override;
  void DisableNotifications() override;

  TestPrivetNotificationService* const service_;

  DISALLOW_COPY_AND_ASSIGN(TestPrivetNotificationDelegate);
};

class TestPrivetNotificationService : public PrivetNotificationService {
 public:
  explicit TestPrivetNotificationService(Profile* profile)
      : PrivetNotificationService(profile) {}
  ~TestPrivetNotificationService() override {}

  const GURL& open_tab_url() const { return open_tab_url_; }
  size_t open_tab_count() const { return open_tab_count_; }
  size_t disable_notifications_count() const {
    return disable_notifications_count_;
  }

  void OpenTab(const GURL& url) {
    open_tab_url_ = url;
    ++open_tab_count_;
  }

  void DisableNotifications() { ++disable_notifications_count_; }

 private:
  // PrivetNotificationService:
  PrivetNotificationDelegate* CreateNotificationDelegate(
      Profile* profile) override {
    return new TestPrivetNotificationDelegate(this, profile);
  }

  GURL open_tab_url_;
  size_t open_tab_count_ = 0;
  size_t disable_notifications_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestPrivetNotificationService);
};

void TestPrivetNotificationDelegate::OpenTab(const GURL& url) {
  service_->OpenTab(url);
}

void TestPrivetNotificationDelegate::DisableNotifications() {
  service_->DisableNotifications();
}

class PrivetNotificationsNotificationTest : public testing::Test {
 public:
  PrivetNotificationsNotificationTest() {}
  ~PrivetNotificationsNotificationTest() override {}

  void SetUp() override {
    testing::Test::SetUp();

    profile_manager_ = std::make_unique<TestingProfileManager>(
        TestingBrowserProcess::GetGlobal());
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("test-user");
    display_service_ =
        std::make_unique<NotificationDisplayServiceTester>(profile_);

    TestingBrowserProcess::GetGlobal()->SetNotificationUIManager(
        std::make_unique<StubNotificationUIManager>());
  }

  void TearDown() override {
    profile_manager_.reset();
    testing::Test::TearDown();
  }

  Profile* profile() { return profile_; }

  // The thread bundle must be first so it is destroyed last.
  content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<NotificationDisplayServiceTester> display_service_;

  std::unique_ptr<TestingProfileManager> profile_manager_;
  Profile* profile_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PrivetNotificationsNotificationTest);
};

TEST_F(PrivetNotificationsNotificationTest, AddToCloudPrint) {
  TestPrivetNotificationService service(profile());
  service.PrivetNotify(1 /* devices_active */, true /* added */);
  // The notification is added asynchronously.
  base::RunLoop().RunUntilIdle();

  auto notifications = display_service_->GetDisplayedNotificationsForType(
      NotificationHandler::Type::TRANSIENT);
  ASSERT_EQ(1U, notifications.size());
  display_service_->SimulateClick(NotificationHandler::Type::TRANSIENT,
                                  notifications[0].id(), 0 /* add */,
                                  base::nullopt);

  EXPECT_EQ("chrome://devices/", service.open_tab_url().spec());
  EXPECT_EQ(1U, service.open_tab_count());
  EXPECT_EQ(0U, service.disable_notifications_count());
  EXPECT_EQ(0U, display_service_
                    ->GetDisplayedNotificationsForType(
                        NotificationHandler::Type::TRANSIENT)
                    .size());
}

TEST_F(PrivetNotificationsNotificationTest, DontShowAgain) {
  TestPrivetNotificationService service(profile());
  service.PrivetNotify(1 /* devices_active */, true /* added */);
  // The notification is added asynchronously.
  base::RunLoop().RunUntilIdle();

  auto notifications = display_service_->GetDisplayedNotificationsForType(
      NotificationHandler::Type::TRANSIENT);
  ASSERT_EQ(1U, notifications.size());
  display_service_->SimulateClick(NotificationHandler::Type::TRANSIENT,
                                  notifications[0].id(),
                                  1 /* don't show again */, base::nullopt);

  EXPECT_EQ("", service.open_tab_url().spec());
  EXPECT_EQ(0U, service.open_tab_count());
  EXPECT_EQ(1U, service.disable_notifications_count());
  EXPECT_EQ(0U, display_service_
                    ->GetDisplayedNotificationsForType(
                        NotificationHandler::Type::TRANSIENT)
                    .size());
}

}  // namespace

}  // namespace cloud_print
