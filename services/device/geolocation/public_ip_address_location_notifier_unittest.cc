// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_location_notifier.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/test_mock_time_task_runner.h"
#include "device/geolocation/public/cpp/geoposition.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "services/device/public/mojom/geoposition.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

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

class PublicIpAddressLocationNotifierTest : public testing::Test {
 protected:
  // Helps test a single call to
  // PublicIpAddressLocationNotifier::QueryNextPositionAfterTimestamp.
  class TestPositionQuery {
   public:
    // Provides a callback suitable to pass to QueryNextPositionAfterTimestamp.
    PublicIpAddressLocationNotifier::QueryNextPositionCallback MakeCallback() {
      return base::BindOnce(&TestPositionQuery::OnQueryNextPositionResponse,
                            base::Unretained(this));
    }

    // Optional. Wait until the callback from MakeCallback() is called.
    void Wait() { loop_.Run(); }

    const base::Optional<mojom::Geoposition>& position() const {
      return position_;
    }

   private:
    void OnQueryNextPositionResponse(const mojom::Geoposition& position) {
      position_ = position;
      loop_.Quit();
    }

    base::RunLoop loop_;
    base::Optional<mojom::Geoposition> position_;
  };

  PublicIpAddressLocationNotifierTest()
      : mock_time_task_runner_(
            base::MakeRefCounted<base::TestMockTimeTaskRunner>(
                base::TestMockTimeTaskRunner::Type::kBoundToThread)),
        mock_time_scoped_context_(mock_time_task_runner_.get()),
        network_change_notifier_(net::NetworkChangeNotifier::CreateMock()),
        notifier_(
            base::Bind(&TestRequestContextProducer, mock_time_task_runner_),
            std::string() /* api_key */) {}

  // Returns the current TestURLFetcher (if any) and advances the test fetcher
  // id for disambiguation from subsequent tests.
  net::TestURLFetcher* GetTestUrlFetcher() {
    net::TestURLFetcher* const fetcher = url_fetcher_factory_.GetFetcherByID(
        NetworkLocationRequest::url_fetcher_id_for_tests);
    if (fetcher)
      ++NetworkLocationRequest::url_fetcher_id_for_tests;
    return fetcher;
  }

  // Gives a valid JSON reponse to the specified URLFetcher.
  // For disambiguation purposes, the specified |latitude| is included in the
  // response.
  void RespondToFetchWithLatitude(net::TestURLFetcher* const fetcher,
                                  const float latitude) {
    // Issue a valid response including the specified latitude.
    fetcher->set_url(fetcher->GetOriginalURL());
    fetcher->set_status(net::URLRequestStatus());
    fetcher->set_response_code(200);
    const char kNetworkResponseFormatString[] =
        R"({
            "accuracy": 100.0,
            "location": {
              "lat": %f,
              "lng": 90.0
            }
          })";
    fetcher->SetResponseString(
        base::StringPrintf(kNetworkResponseFormatString, latitude));
    fetcher->delegate()->OnURLFetchComplete(fetcher);
  }

  void RespondToFetchWithServerError(net::TestURLFetcher* const fetcher) {
    fetcher->set_url(fetcher->GetOriginalURL());
    fetcher->set_status(net::URLRequestStatus());
    fetcher->set_response_code(500);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
  }

  // Expects a non-empty and valid Geoposition, including the specified
  // |latitude|.
  void ExpectValidPosition(const base::Optional<mojom::Geoposition>& position,
                           const float latitude) {
    ASSERT_TRUE(position);
    EXPECT_TRUE(ValidateGeoposition(*position));
    EXPECT_FLOAT_EQ(position->latitude, latitude);
  }

  void ExpectError(const base::Optional<mojom::Geoposition>& position) {
    ASSERT_TRUE(position);
    EXPECT_THAT(position->error_code,
                mojom::Geoposition::ErrorCode::POSITION_UNAVAILABLE);
  }
  // Use a TaskRunner on which we can fast-forward time.
  const scoped_refptr<base::TestMockTimeTaskRunner> mock_time_task_runner_;
  const base::TestMockTimeTaskRunner::ScopedContext mock_time_scoped_context_;

  // notifier_ requires a NetworkChangeNotifier to exist.
  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;

  // Intercept URL fetchers.
  net::TestURLFetcherFactory url_fetcher_factory_;

  // The object under test.
  PublicIpAddressLocationNotifier notifier_;
};

// Tests that a single initial query makes a URL fetch and returns a position.
TEST_F(PublicIpAddressLocationNotifierTest, SingleQueryReturns) {
  // Make query.
  TestPositionQuery query;
  notifier_.QueryNextPosition(base::Time::Now(),
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query.MakeCallback());
  // Expect a URL fetch & send a valid response.
  net::TestURLFetcher* const fetcher = GetTestUrlFetcher();
  EXPECT_THAT(fetcher, testing::NotNull());
  RespondToFetchWithLatitude(fetcher, 1.0f);
  // Expect the query to return.
  ExpectValidPosition(query.position(), 1.0f);
}

// Tests that a second query asking for an older timestamp gets a cached result.
TEST_F(PublicIpAddressLocationNotifierTest, OlderQueryReturnsCached) {
  const auto time = base::Time::Now();

  // Initial query.
  TestPositionQuery query_1;
  notifier_.QueryNextPosition(time, PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query_1.MakeCallback());
  net::TestURLFetcher* const fetcher = GetTestUrlFetcher();
  EXPECT_THAT(fetcher, testing::NotNull());
  RespondToFetchWithLatitude(fetcher, 1.0f);
  ExpectValidPosition(query_1.position(), 1.0f);

  // Second query for an earlier time.
  TestPositionQuery query_2;
  notifier_.QueryNextPosition(time - base::TimeDelta::FromMinutes(5),
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query_2.MakeCallback());
  // Expect a cached result, so no new network request.
  EXPECT_THAT(GetTestUrlFetcher(), testing::IsNull());
  // Expect the same result as query_1.
  ExpectValidPosition(query_2.position(), 1.0f);
}

// Tests that a subsequent query seeking a newer geoposition does not return,
// until a network change occurs.
TEST_F(PublicIpAddressLocationNotifierTest,
       SubsequentQueryWaitsForNetworkChange) {
  // Initial query.
  TestPositionQuery query_1;
  notifier_.QueryNextPosition(base::Time::Now(),
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query_1.MakeCallback());
  net::TestURLFetcher* const fetcher = GetTestUrlFetcher();
  EXPECT_THAT(fetcher, testing::NotNull());
  RespondToFetchWithLatitude(fetcher, 1.0f);
  ExpectValidPosition(query_1.position(), 1.0f);

  // Second query seeking a position newer than the result of query_1.
  TestPositionQuery query_2;
  notifier_.QueryNextPosition(query_1.position()->timestamp,
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query_2.MakeCallback());
  // Expect no network request or callback.
  EXPECT_THAT(GetTestUrlFetcher(), testing::IsNull());
  EXPECT_FALSE(query_2.position().has_value());

  // Fake a network change notification.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  // Wait for the notifier to complete its delayed reaction.
  mock_time_task_runner_->FastForwardUntilNoTasksRemain();

  // Now expect a network request and query_2 to return.
  net::TestURLFetcher* const fetcher_2 = GetTestUrlFetcher();
  EXPECT_THAT(fetcher_2, testing::NotNull());
  RespondToFetchWithLatitude(fetcher_2, 2.0f);
  ExpectValidPosition(query_2.position(), 2.0f);
}

// Tests that multiple network changes in a short time result in only one
// network request.
TEST_F(PublicIpAddressLocationNotifierTest,
       ConsecutiveNetworkChangesRequestsOnlyOnce) {
  // Initial query.
  TestPositionQuery query_1;
  notifier_.QueryNextPosition(base::Time::Now(),
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query_1.MakeCallback());
  net::TestURLFetcher* const fetcher = GetTestUrlFetcher();
  EXPECT_THAT(fetcher, testing::NotNull());
  RespondToFetchWithLatitude(fetcher, 1.0f);
  ExpectValidPosition(query_1.position(), 1.0f);

  // Second query seeking a position newer than the result of query_1.
  TestPositionQuery query_2;
  notifier_.QueryNextPosition(query_1.position()->timestamp,
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query_2.MakeCallback());
  // Expect no network request or callback since network has not changed.
  EXPECT_THAT(GetTestUrlFetcher(), testing::IsNull());
  EXPECT_FALSE(query_2.position().has_value());

  // Fake several consecutive network changes notification.
  for (int i = 0; i < 10; ++i) {
    net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
        net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
    mock_time_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(5));
  }
  // Expect still no network request or callback.
  EXPECT_THAT(GetTestUrlFetcher(), testing::IsNull());
  EXPECT_FALSE(query_2.position().has_value());

  // Wait longer.
  mock_time_task_runner_->FastForwardUntilNoTasksRemain();

  // Now expect a network request & query_2 to return.
  net::TestURLFetcher* const fetcher_2 = GetTestUrlFetcher();
  EXPECT_THAT(fetcher_2, testing::NotNull());
  RespondToFetchWithLatitude(fetcher_2, 2.0f);
  ExpectValidPosition(query_2.position(), 2.0f);
}

// Tests multiple waiting queries.
TEST_F(PublicIpAddressLocationNotifierTest, MutipleWaitingQueries) {
  // Initial query.
  TestPositionQuery query_1;
  notifier_.QueryNextPosition(base::Time::Now(),
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query_1.MakeCallback());
  net::TestURLFetcher* const fetcher = GetTestUrlFetcher();
  EXPECT_THAT(fetcher, testing::NotNull());
  RespondToFetchWithLatitude(fetcher, 1.0f);
  ExpectValidPosition(query_1.position(), 1.0f);

  // Multiple queries seeking positions newer than the result of query_1.
  TestPositionQuery query_2;
  notifier_.QueryNextPosition(query_1.position()->timestamp,
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query_2.MakeCallback());
  TestPositionQuery query_3;
  notifier_.QueryNextPosition(query_1.position()->timestamp,
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query_3.MakeCallback());

  // Expect no network requests or callback since network has not changed.
  EXPECT_THAT(GetTestUrlFetcher(), testing::IsNull());
  EXPECT_FALSE(query_2.position().has_value());
  EXPECT_FALSE(query_3.position().has_value());

  // Fake a network change notification.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  // Wait for the notifier to complete its delayed reaction.
  mock_time_task_runner_->FastForwardUntilNoTasksRemain();

  // Now expect a network request & fake a valid response.
  net::TestURLFetcher* const fetcher_2 = GetTestUrlFetcher();
  EXPECT_THAT(fetcher_2, testing::NotNull());
  RespondToFetchWithLatitude(fetcher_2, 2.0f);
  // Expect all queries to now return.
  ExpectValidPosition(query_2.position(), 2.0f);
  ExpectValidPosition(query_3.position(), 2.0f);
}

// Tests that server error is propogated to the client.
TEST_F(PublicIpAddressLocationNotifierTest, ServerError) {
  // Make query.
  TestPositionQuery query;
  notifier_.QueryNextPosition(base::Time::Now(),
                              PARTIAL_TRAFFIC_ANNOTATION_FOR_TESTS,
                              query.MakeCallback());
  // Expect a URL fetch & send a valid response.
  net::TestURLFetcher* const fetcher = GetTestUrlFetcher();
  EXPECT_THAT(fetcher, testing::NotNull());
  RespondToFetchWithServerError(fetcher);
  // Expect the query to return.
  ExpectError(query.position());
}

}  // namespace device
