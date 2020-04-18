// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_geolocator.h"

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {
namespace {

const char kTestGeolocationApiKey[] = "";

// Simple request context producer that immediately produces a
// TestURLRequestContextGetter.
void TestRequestContextProducer(
    const scoped_refptr<base::SingleThreadTaskRunner>& network_task_runner,
    base::OnceCallback<void(scoped_refptr<net::URLRequestContextGetter>)>
        response_callback) {
  std::move(response_callback)
      .Run(base::MakeRefCounted<net::TestURLRequestContextGetter>(
          network_task_runner));
}

class PublicIpAddressGeolocatorTest : public testing::Test {
 public:
  PublicIpAddressGeolocatorTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {
    notifier_.reset(new PublicIpAddressLocationNotifier(
        base::Bind(&TestRequestContextProducer,
                   scoped_task_environment_.GetMainThreadTaskRunner()),
        kTestGeolocationApiKey));
  }

  ~PublicIpAddressGeolocatorTest() override = default;

 protected:
  void SetUp() override {
    // Intercept Mojo bad-message errors.
    mojo::edk::SetDefaultProcessErrorCallback(
        base::Bind(&PublicIpAddressGeolocatorTest::OnMojoBadMessage,
                   base::Unretained(this)));

    binding_set_.AddBinding(
        std::make_unique<PublicIpAddressGeolocator>(
            PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS, notifier_.get(),
            base::Bind(&PublicIpAddressGeolocatorTest::OnGeolocatorBadMessage,
                       base::Unretained(this))),
        mojo::MakeRequest(&public_ip_address_geolocator_));
  }

  void TearDown() override {
    // Stop intercepting Mojo bad-message errors.
    mojo::edk::SetDefaultProcessErrorCallback(
        mojo::edk::ProcessErrorCallback());
  }

  // Deal with mojo bad message.
  void OnMojoBadMessage(const std::string& error) {
    bad_messages_.push_back(error);
  }

  // Deal with PublicIpAddressGeolocator bad message.
  void OnGeolocatorBadMessage(const std::string& message) {
    binding_set_.ReportBadMessage(message);
  }

  // Invokes QueryNextPosition on |public_ip_address_geolocator_|, and runs
  // |done_closure| when the response comes back.
  void QueryNextPosition(base::Closure done_closure) {
    public_ip_address_geolocator_->QueryNextPosition(base::BindOnce(
        &PublicIpAddressGeolocatorTest::OnQueryNextPositionResponse,
        base::Unretained(this), done_closure));
  }

  // Callback for QueryNextPosition() that records the result in |position_| and
  // then invokes |done_closure|.
  void OnQueryNextPositionResponse(base::Closure done_closure,
                                   mojom::GeopositionPtr position) {
    position_ = std::move(position);
    done_closure.Run();
  }

  // Result of the latest completed call to QueryNextPosition.
  mojom::GeopositionPtr position_;

  // StrongBindingSet to mojom::Geolocation.
  mojo::StrongBindingSet<mojom::Geolocation> binding_set_;

  // Test task runner.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // List of any Mojo bad-message errors raised.
  std::vector<std::string> bad_messages_;

  // PublicIpAddressGeolocator requires a notifier.
  std::unique_ptr<PublicIpAddressLocationNotifier> notifier_;

  // The object under test.
  mojom::GeolocationPtr public_ip_address_geolocator_;

  // PublicIpAddressGeolocator implementation.
  // std::unique_ptr<PublicIpAddressGeolocator> impl_;

  DISALLOW_COPY_AND_ASSIGN(PublicIpAddressGeolocatorTest);
};

// Observer that waits until a TestURLFetcher with the specified fetcher_id
// starts, after which it is made available through .fetcher().
class TestURLFetcherObserver : public net::TestURLFetcher::DelegateForTests {
 public:
  explicit TestURLFetcherObserver(int expected_fetcher_id)
      : expected_fetcher_id_(expected_fetcher_id) {
    factory_.SetDelegateForTests(this);
  }
  virtual ~TestURLFetcherObserver() {}

  void Wait() { loop_.Run(); }

  net::TestURLFetcher* fetcher() { return fetcher_; }

  // net::TestURLFetcher::DelegateForTests:
  void OnRequestStart(int fetcher_id) override {
    if (fetcher_id == expected_fetcher_id_) {
      fetcher_ = factory_.GetFetcherByID(fetcher_id);
      fetcher_->SetDelegateForTests(nullptr);
      factory_.SetDelegateForTests(nullptr);
      loop_.Quit();
    }
  }
  void OnChunkUpload(int fetcher_id) override {}
  void OnRequestEnd(int fetcher_id) override {}

 private:
  const int expected_fetcher_id_;
  net::TestURLFetcher* fetcher_ = nullptr;
  net::TestURLFetcherFactory factory_;
  base::RunLoop loop_;
};

// Basic test of a client invoking QueryNextPosition.
TEST_F(PublicIpAddressGeolocatorTest, BindAndQuery) {
  // Intercept the URLFetcher from network geolocation request.
  TestURLFetcherObserver observer(
      device::NetworkLocationRequest::url_fetcher_id_for_tests);

  // Invoke QueryNextPosition and wait for a URLFetcher.
  base::RunLoop loop;
  QueryNextPosition(loop.QuitClosure());
  observer.Wait();
  DCHECK(observer.fetcher());

  // Issue a valid response to the URLFetcher.
  observer.fetcher()->set_url(observer.fetcher()->GetOriginalURL());
  observer.fetcher()->set_status(net::URLRequestStatus());
  observer.fetcher()->set_response_code(200);
  observer.fetcher()->SetResponseString(R"({
        "accuracy": 100.0,
        "location": {
          "lat": 10.0,
          "lng": 20.0
        }
      })");
  scoped_task_environment_.GetMainThreadTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&net::URLFetcherDelegate::OnURLFetchComplete,
                     base::Unretained(observer.fetcher()->delegate()),
                     base::Unretained(observer.fetcher())));

  // Wait for QueryNextPosition to return.
  loop.Run();

  EXPECT_THAT(position_->accuracy, testing::Eq(100.0));
  EXPECT_THAT(position_->latitude, testing::Eq(10.0));
  EXPECT_THAT(position_->longitude, testing::Eq(20.0));
  EXPECT_THAT(bad_messages_, testing::IsEmpty());
}

// Tests that multiple overlapping calls to QueryNextPosition result in a
// connection error and reports a bad message.
TEST_F(PublicIpAddressGeolocatorTest, ProhibitedOverlappingCalls) {
  base::RunLoop loop;
  public_ip_address_geolocator_.set_connection_error_handler(
      loop.QuitClosure());

  // Issue two overlapping calls to QueryNextPosition.
  QueryNextPosition(base::Closure());
  QueryNextPosition(base::Closure());

  // This terminates only in case of connection error, which we expect.
  loop.Run();

  // Verify that the geolocator reported a bad message.
  EXPECT_THAT(bad_messages_, testing::SizeIs(1));
}

}  // namespace
}  // namespace device
