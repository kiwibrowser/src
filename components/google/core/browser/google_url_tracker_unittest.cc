// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/google/core/browser/google_url_tracker.h"

#include <memory>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/google/core/browser/google_pref_names.h"
#include "components/google/core/browser/google_url_tracker_client.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// TestCallbackListener ---------------------------------------------------

class TestCallbackListener {
 public:
  TestCallbackListener();
  virtual ~TestCallbackListener();

  bool HasRegisteredCallback();
  void RegisterCallback(GoogleURLTracker* google_url_tracker);

  bool notified() const { return notified_; }
  void clear_notified() { notified_ = false; }

 private:
  void OnGoogleURLUpdated();

  bool notified_;
  std::unique_ptr<GoogleURLTracker::Subscription>
      google_url_updated_subscription_;
};

TestCallbackListener::TestCallbackListener() : notified_(false) {
}

TestCallbackListener::~TestCallbackListener() {
}

void TestCallbackListener::OnGoogleURLUpdated() {
  notified_ = true;
}

bool TestCallbackListener::HasRegisteredCallback() {
  return google_url_updated_subscription_.get();
}

void TestCallbackListener::RegisterCallback(
    GoogleURLTracker* google_url_tracker) {
  google_url_updated_subscription_ =
      google_url_tracker->RegisterCallback(base::Bind(
          &TestCallbackListener::OnGoogleURLUpdated, base::Unretained(this)));
}


// TestGoogleURLTrackerClient -------------------------------------------------

class TestGoogleURLTrackerClient : public GoogleURLTrackerClient {
 public:
  TestGoogleURLTrackerClient(
      PrefService* prefs_,
      network::TestURLLoaderFactory* test_url_loader_factory);
  ~TestGoogleURLTrackerClient() override;

  bool IsBackgroundNetworkingEnabled() override;
  PrefService* GetPrefs() override;
  network::mojom::URLLoaderFactory* GetURLLoaderFactory() override;

 private:
  PrefService* prefs_;
  scoped_refptr<network::SharedURLLoaderFactory> test_shared_loader_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestGoogleURLTrackerClient);
};

TestGoogleURLTrackerClient::TestGoogleURLTrackerClient(
    PrefService* prefs,
    network::TestURLLoaderFactory* test_url_loader_factory)
    : prefs_(prefs),
      test_shared_loader_factory_(
          base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
              test_url_loader_factory)) {}

TestGoogleURLTrackerClient::~TestGoogleURLTrackerClient() {
}

bool TestGoogleURLTrackerClient::IsBackgroundNetworkingEnabled() {
  return true;
}

PrefService* TestGoogleURLTrackerClient::GetPrefs() {
  return prefs_;
}

network::mojom::URLLoaderFactory*
TestGoogleURLTrackerClient::GetURLLoaderFactory() {
  return test_shared_loader_factory_.get();
}

}  // namespace

// GoogleURLTrackerTest -------------------------------------------------------

class GoogleURLTrackerTest : public testing::Test {
 protected:
  GoogleURLTrackerTest();
  ~GoogleURLTrackerTest() override;

  // testing::Test
  void SetUp() override;
  void TearDown() override;

  void MockSearchDomainCheckResponse(const std::string& domain);
  void RequestServerCheck();
  void FinishSleep();
  void NotifyNetworkChanged();
  void set_google_url(const GURL& url) {
    google_url_tracker_->google_url_ = url;
  }
  GURL google_url() const { return google_url_tracker_->google_url(); }
  bool listener_notified() const { return listener_.notified(); }
  void clear_listener_notified() { listener_.clear_notified(); }
  bool handled_request() const { return handled_request_; }

  GoogleURLTrackerClient* client() const { return client_; }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  TestingPrefServiceSimple prefs_;
  network::TestURLLoaderFactory test_url_loader_factory_;

  // Creating this allows us to call
  // net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests().
  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
  GoogleURLTrackerClient* client_;

  std::unique_ptr<GoogleURLTracker> google_url_tracker_;
  TestCallbackListener listener_;

  bool handled_request_ = false;
};

GoogleURLTrackerTest::GoogleURLTrackerTest() {
  prefs_.registry()->RegisterStringPref(
      prefs::kLastKnownGoogleURL,
      GoogleURLTracker::kDefaultGoogleHomepage);
}

GoogleURLTrackerTest::~GoogleURLTrackerTest() {
}

void GoogleURLTrackerTest::SetUp() {
  network_change_notifier_.reset(net::NetworkChangeNotifier::CreateMock());
  // Ownership is passed to google_url_tracker_, but a weak pointer is kept;
  // this is safe since GoogleURLTracker keeps the client for its lifetime.
  client_ = new TestGoogleURLTrackerClient(&prefs_, &test_url_loader_factory_);
  std::unique_ptr<GoogleURLTrackerClient> client(client_);
  google_url_tracker_.reset(new GoogleURLTracker(
      std::move(client), GoogleURLTracker::ALWAYS_DOT_COM_MODE));
}

void GoogleURLTrackerTest::TearDown() {
  google_url_tracker_->Shutdown();
}

void GoogleURLTrackerTest::MockSearchDomainCheckResponse(
    const std::string& domain) {
  handled_request_ = false;
  test_url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        handled_request_ = true;
      }));
  test_url_loader_factory_.AddResponse(GoogleURLTracker::kSearchDomainCheckURL,
                                       domain);
}

void GoogleURLTrackerTest::RequestServerCheck() {
  if (!listener_.HasRegisteredCallback())
    listener_.RegisterCallback(google_url_tracker_.get());
  google_url_tracker_->SetNeedToLoad();
  base::RunLoop().RunUntilIdle();
}

void GoogleURLTrackerTest::FinishSleep() {
  google_url_tracker_->FinishSleep();
  base::RunLoop().RunUntilIdle();
}

void GoogleURLTrackerTest::NotifyNetworkChanged() {
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  // For thread safety, the NCN queues tasks to do the actual notifications, so
  // we need to spin the message loop so the tracker will actually be notified.
  base::RunLoop().RunUntilIdle();
}

// Tests ----------------------------------------------------------------------

TEST_F(GoogleURLTrackerTest, DontFetchWhenNoOneRequestsCheck) {
  EXPECT_EQ(GURL(GoogleURLTracker::kDefaultGoogleHomepage), google_url());
  FinishSleep();
  // No one called RequestServerCheck() so nothing should have happened.
  EXPECT_FALSE(handled_request());
  MockSearchDomainCheckResponse(".google.co.uk");
  EXPECT_EQ(GURL(GoogleURLTracker::kDefaultGoogleHomepage), google_url());
  EXPECT_FALSE(listener_notified());
}

TEST_F(GoogleURLTrackerTest, Update) {
  MockSearchDomainCheckResponse(".google.co.uk");

  RequestServerCheck();
  EXPECT_FALSE(handled_request());
  EXPECT_EQ(GURL(GoogleURLTracker::kDefaultGoogleHomepage), google_url());
  EXPECT_FALSE(listener_notified());

  FinishSleep();
  EXPECT_EQ(GURL("https://www.google.co.uk/"), google_url());
  EXPECT_TRUE(listener_notified());
}

TEST_F(GoogleURLTrackerTest, DontUpdateWhenUnchanged) {
  MockSearchDomainCheckResponse(".google.co.uk");

  GURL original_google_url("https://www.google.co.uk/");
  set_google_url(original_google_url);

  RequestServerCheck();
  EXPECT_FALSE(handled_request());
  EXPECT_EQ(original_google_url, google_url());
  EXPECT_FALSE(listener_notified());

  FinishSleep();
  EXPECT_EQ(original_google_url, google_url());
  // No one should be notified, because the new URL matches the old.
  EXPECT_FALSE(listener_notified());
}

TEST_F(GoogleURLTrackerTest, DontUpdateOnBadReplies) {
  GURL original_google_url("https://www.google.co.uk/");
  set_google_url(original_google_url);

  RequestServerCheck();
  EXPECT_FALSE(handled_request());
  EXPECT_EQ(original_google_url, google_url());
  EXPECT_FALSE(listener_notified());

  // Old-style URL string.
  MockSearchDomainCheckResponse("https://www.google.com/");
  FinishSleep();
  EXPECT_EQ(original_google_url, google_url());
  EXPECT_FALSE(listener_notified());

  // Not a Google domain.
  MockSearchDomainCheckResponse(".google.evil.com");
  FinishSleep();
  EXPECT_EQ(original_google_url, google_url());
  EXPECT_FALSE(listener_notified());

  // Doesn't start with .google.
  MockSearchDomainCheckResponse(".mail.google.com");
  NotifyNetworkChanged();
  EXPECT_EQ(original_google_url, google_url());
  EXPECT_FALSE(listener_notified());

  // Non-empty path.
  MockSearchDomainCheckResponse(".google.com/search");
  NotifyNetworkChanged();
  EXPECT_EQ(original_google_url, google_url());
  EXPECT_FALSE(listener_notified());

  // Non-empty query.
  MockSearchDomainCheckResponse(".google.com/?q=foo");
  NotifyNetworkChanged();
  EXPECT_EQ(original_google_url, google_url());
  EXPECT_FALSE(listener_notified());

  // Non-empty ref.
  MockSearchDomainCheckResponse(".google.com/#anchor");
  NotifyNetworkChanged();
  EXPECT_EQ(original_google_url, google_url());
  EXPECT_FALSE(listener_notified());

  // Complete garbage.
  MockSearchDomainCheckResponse("HJ)*qF)_*&@f1");
  NotifyNetworkChanged();
  EXPECT_EQ(original_google_url, google_url());
  EXPECT_FALSE(listener_notified());
}

TEST_F(GoogleURLTrackerTest, RefetchOnNetworkChange) {
  MockSearchDomainCheckResponse(".google.co.uk");
  RequestServerCheck();
  FinishSleep();
  EXPECT_EQ(GURL("https://www.google.co.uk/"), google_url());
  EXPECT_TRUE(listener_notified());
  clear_listener_notified();

  MockSearchDomainCheckResponse(".google.co.in");
  NotifyNetworkChanged();
  EXPECT_EQ(GURL("https://www.google.co.in/"), google_url());
  EXPECT_TRUE(listener_notified());
}

TEST_F(GoogleURLTrackerTest, DontRefetchWhenNoOneRequestsCheck) {
  MockSearchDomainCheckResponse(".google.co.uk");
  FinishSleep();
  NotifyNetworkChanged();
  // No one called RequestServerCheck() so nothing should have happened.
  EXPECT_FALSE(handled_request());
  EXPECT_EQ(GURL(GoogleURLTracker::kDefaultGoogleHomepage), google_url());
  EXPECT_FALSE(listener_notified());
}

TEST_F(GoogleURLTrackerTest, FetchOnLateRequest) {
  MockSearchDomainCheckResponse(".google.co.jp");
  FinishSleep();
  NotifyNetworkChanged();

  MockSearchDomainCheckResponse(".google.co.uk");
  RequestServerCheck();
  // The first request for a check should trigger a fetch if it hasn't happened
  // already.
  EXPECT_EQ(GURL("https://www.google.co.uk/"), google_url());
  EXPECT_TRUE(listener_notified());
}

TEST_F(GoogleURLTrackerTest, DontFetchTwiceOnLateRequests) {
  MockSearchDomainCheckResponse(".google.co.jp");
  FinishSleep();
  NotifyNetworkChanged();

  MockSearchDomainCheckResponse(".google.co.uk");
  RequestServerCheck();
  // The first request for a check should trigger a fetch if it hasn't happened
  // already.
  EXPECT_EQ(GURL("https://www.google.co.uk/"), google_url());
  EXPECT_TRUE(listener_notified());
  clear_listener_notified();

  MockSearchDomainCheckResponse(".google.co.in");
  RequestServerCheck();
  // The second request should be ignored.
  EXPECT_FALSE(handled_request());
  EXPECT_EQ(GURL("https://www.google.co.uk/"), google_url());
  EXPECT_FALSE(listener_notified());
}
