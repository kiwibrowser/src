// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_usage/core/data_use_aggregator.h"

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/span.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/time/time.h"
#include "components/data_usage/core/data_use.h"
#include "components/data_usage/core/data_use_amortizer.h"
#include "components/data_usage/core/data_use_annotator.h"
#include "net/base/load_timing_info.h"
#include "net/base/network_change_notifier.h"
#include "net/base/network_delegate_impl.h"
#include "net/socket/socket_test_util.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace data_usage {

namespace {

base::TimeTicks GetRequestStart(const net::URLRequest& request) {
  net::LoadTimingInfo load_timing_info;
  request.GetLoadTimingInfo(&load_timing_info);
  return load_timing_info.request_start;
}

// Test class that can set the network operator's MCCMNC.
class TestDataUseAggregator : public DataUseAggregator {
 public:
  TestDataUseAggregator(std::unique_ptr<DataUseAnnotator> annotator,
                        std::unique_ptr<DataUseAmortizer> amortizer)
      : DataUseAggregator(std::move(annotator), std::move(amortizer)) {}

  ~TestDataUseAggregator() override {}

 private:
  friend class TestNetworkChangeNotifier;
  using DataUseAggregator::OnNetworkChanged;
  using DataUseAggregator::SetMccMncForTests;
};

// Override NetworkChangeNotifier to simulate connection type changes for tests.
class TestNetworkChangeNotifier : public net::NetworkChangeNotifier {
 public:
  explicit TestNetworkChangeNotifier(TestDataUseAggregator* data_use_aggregator)
      : data_use_aggregator_(data_use_aggregator),
        connection_type_to_return_(
            net::NetworkChangeNotifier::CONNECTION_UNKNOWN) {}

  // Simulates a change of the connection type to |type|.
  void SimulateNetworkConnectionChange(ConnectionType type,
                                       const std::string& mcc_mnc) {
    connection_type_to_return_ = type;
    data_use_aggregator_->OnNetworkChanged(type);
    data_use_aggregator_->SetMccMncForTests(mcc_mnc);
  }

  ConnectionType GetCurrentConnectionType() const override {
    return connection_type_to_return_;
  }

 private:
  TestDataUseAggregator* data_use_aggregator_;

  // The currently simulated network connection type.
  ConnectionType connection_type_to_return_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkChangeNotifier);
};

// A fake DataUseAnnotator that sets the tab ID of DataUse objects to a
// predetermined fake tab ID.
class FakeDataUseAnnotator : public DataUseAnnotator {
 public:
  FakeDataUseAnnotator() : tab_id_(SessionID::InvalidValue()) {}
  ~FakeDataUseAnnotator() override {}

  void Annotate(
      net::URLRequest* request,
      std::unique_ptr<DataUse> data_use,
      const base::Callback<void(std::unique_ptr<DataUse>)>& callback) override {
    data_use->tab_id = tab_id_;
    callback.Run(std::move(data_use));
  }

  void set_tab_id(SessionID tab_id) { tab_id_ = tab_id; }

 private:
  SessionID tab_id_;

  DISALLOW_COPY_AND_ASSIGN(FakeDataUseAnnotator);
};

// Test DataUseAmortizer that doubles the bytes of all DataUse objects it sees.
class DoublingAmortizer : public DataUseAmortizer {
 public:
  DoublingAmortizer() {}
  ~DoublingAmortizer() override {}

  void AmortizeDataUse(std::unique_ptr<DataUse> data_use,
                       const AmortizationCompleteCallback& callback) override {
    data_use->tx_bytes *= 2;
    data_use->rx_bytes *= 2;
    callback.Run(std::move(data_use));
  }

  void OnExtraBytes(int64_t extra_tx_bytes, int64_t extra_rx_bytes) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DoublingAmortizer);
};

// A network delegate that reports all received and sent network bytes to a
// DataUseAggregator.
class ReportingNetworkDelegate : public net::NetworkDelegateImpl {
 public:
  // The simulated context for the data usage of a net::URLRequest.
  struct DataUseContext {
    DataUseContext()
        : tab_id(SessionID::InvalidValue()),
          connection_type(net::NetworkChangeNotifier::CONNECTION_UNKNOWN) {}

    DataUseContext(SessionID tab_id,
                   net::NetworkChangeNotifier::ConnectionType connection_type,
                   const std::string& mcc_mnc)
        : tab_id(tab_id), connection_type(connection_type), mcc_mnc(mcc_mnc) {}

    SessionID tab_id;
    net::NetworkChangeNotifier::ConnectionType connection_type;
    std::string mcc_mnc;
  };

  typedef std::map<const net::URLRequest*, DataUseContext> DataUseContextMap;

  // Constructs a ReportingNetworkDelegate. |fake_data_use_annotator| can be
  // NULL, indicating that no annotator is in use and no requests should be
  // annotated with tab IDs.
  ReportingNetworkDelegate(
      TestDataUseAggregator* data_use_aggregator,
      FakeDataUseAnnotator* fake_data_use_annotator,
      TestNetworkChangeNotifier* test_network_change_notifier)
      : data_use_aggregator_(data_use_aggregator),
        fake_data_use_annotator_(fake_data_use_annotator),
        test_network_change_notifier_(test_network_change_notifier) {}

  ~ReportingNetworkDelegate() override {}

  void set_data_use_context_map(const DataUseContextMap& data_use_context_map) {
    data_use_context_map_ = data_use_context_map;
  }

 private:
  void UpdateDataUseContext(const net::URLRequest& request) {
    DataUseContextMap::const_iterator data_use_context_it =
        data_use_context_map_.find(&request);
    DataUseContext data_use_context =
        data_use_context_it == data_use_context_map_.end()
            ? DataUseContext()
            : data_use_context_it->second;

    if (fake_data_use_annotator_)
      fake_data_use_annotator_->set_tab_id(data_use_context.tab_id);

    if (test_network_change_notifier_->GetCurrentConnectionType() !=
        data_use_context.connection_type) {
      test_network_change_notifier_->SimulateNetworkConnectionChange(
          data_use_context.connection_type, data_use_context.mcc_mnc);
    }
  }

  void OnNetworkBytesReceived(net::URLRequest* request,
                              int64_t bytes_received) override {
    UpdateDataUseContext(*request);
    data_use_aggregator_->ReportDataUse(request, 0 /* tx_bytes */,
                                        bytes_received);
  }

  void OnNetworkBytesSent(net::URLRequest* request,
                          int64_t bytes_sent) override {
    UpdateDataUseContext(*request);
    data_use_aggregator_->ReportDataUse(request, bytes_sent, 0 /* rx_bytes */);
  }

  TestDataUseAggregator* data_use_aggregator_;
  FakeDataUseAnnotator* fake_data_use_annotator_;
  TestNetworkChangeNotifier* test_network_change_notifier_;
  DataUseContextMap data_use_context_map_;

  DISALLOW_COPY_AND_ASSIGN(ReportingNetworkDelegate);
};

// An observer that keeps track of all the data use it observed.
class TestObserver : public DataUseAggregator::Observer {
 public:
  explicit TestObserver(DataUseAggregator* data_use_aggregator)
      : data_use_aggregator_(data_use_aggregator) {
    data_use_aggregator_->AddObserver(this);
  }

  ~TestObserver() override { data_use_aggregator_->RemoveObserver(this); }

  void OnDataUse(const DataUse& data_use) override {
    observed_data_use_.push_back(data_use);
  }

  const std::vector<DataUse>& observed_data_use() const {
    return observed_data_use_;
  }

 private:
  DataUseAggregator* data_use_aggregator_;
  std::vector<DataUse> observed_data_use_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

class DataUseAggregatorTest : public testing::Test {
 public:
  DataUseAggregatorTest() {}
  ~DataUseAggregatorTest() override {}

  void Initialize(std::unique_ptr<FakeDataUseAnnotator> annotator,
                  std::unique_ptr<DataUseAmortizer> amortizer) {
    // Destroy objects that have dependencies on other objects here in the
    // reverse order that they are created.
    context_.reset();
    reporting_network_delegate_.reset();
    mock_socket_factory_.reset();
    test_network_change_notifier_.reset();
    test_observer_.reset();

    // Initialize testing objects.
    FakeDataUseAnnotator* fake_data_use_annotator = annotator.get();
    data_use_aggregator_.reset(
        new TestDataUseAggregator(std::move(annotator), std::move(amortizer)));
    test_observer_.reset(new TestObserver(data_use_aggregator_.get()));
    test_network_change_notifier_.reset(
        new TestNetworkChangeNotifier(data_use_aggregator_.get()));
    mock_socket_factory_.reset(new net::MockClientSocketFactory());
    reporting_network_delegate_.reset(new ReportingNetworkDelegate(
        data_use_aggregator_.get(), fake_data_use_annotator,
        test_network_change_notifier_.get()));

    context_.reset(new net::TestURLRequestContext(true));
    context_->set_client_socket_factory(mock_socket_factory_.get());
    context_->set_network_delegate(reporting_network_delegate_.get());
    context_->Init();
  }

  std::unique_ptr<net::URLRequest> ExecuteRequest(
      const GURL& url,
      const GURL& site_for_cookies,
      SessionID tab_id,
      net::NetworkChangeNotifier::ConnectionType connection_type,
      const std::string& mcc_mnc) {
    net::MockRead reads[] = {
        net::MockRead("HTTP/1.1 200 OK\r\n\r\n"), net::MockRead("hello world"),
        net::MockRead(net::SYNCHRONOUS, net::OK),
    };
    net::StaticSocketDataProvider socket(reads, base::span<net::MockWrite>());
    mock_socket_factory_->AddSocketDataProvider(&socket);

    net::TestDelegate delegate;
    std::unique_ptr<net::URLRequest> request = context_->CreateRequest(
        url, net::IDLE, &delegate, TRAFFIC_ANNOTATION_FOR_TESTS);
    request->set_site_for_cookies(site_for_cookies);

    ReportingNetworkDelegate::DataUseContextMap data_use_context_map;
    data_use_context_map[request.get()] =
        ReportingNetworkDelegate::DataUseContext(tab_id, connection_type,
                                                 mcc_mnc);
    reporting_network_delegate_->set_data_use_context_map(data_use_context_map);

    request->Start();
    base::RunLoop().RunUntilIdle();

    return request;
  }

  ReportingNetworkDelegate* reporting_network_delegate() {
    return reporting_network_delegate_.get();
  }

  DataUseAggregator* data_use_aggregator() {
    return data_use_aggregator_.get();
  }

  net::MockClientSocketFactory* mock_socket_factory() {
    return mock_socket_factory_.get();
  }

  net::TestURLRequestContext* context() { return context_.get(); }

  TestObserver* test_observer() { return test_observer_.get(); }

 private:
  base::MessageLoopForIO loop_;
  std::unique_ptr<TestDataUseAggregator> data_use_aggregator_;
  std::unique_ptr<TestObserver> test_observer_;
  std::unique_ptr<TestNetworkChangeNotifier> test_network_change_notifier_;
  std::unique_ptr<net::MockClientSocketFactory> mock_socket_factory_;
  std::unique_ptr<ReportingNetworkDelegate> reporting_network_delegate_;
  std::unique_ptr<net::TestURLRequestContext> context_;

  DISALLOW_COPY_AND_ASSIGN(DataUseAggregatorTest);
};

TEST_F(DataUseAggregatorTest, ReportDataUse) {
  const struct {
    bool use_annotator;
    bool use_amortizer;
    bool expect_tab_ids;
    int64_t expected_amortization_multiple;
  } kTestCases[] = {
      {false, false, false, 1},
      {false, true, false, 2},
      {true, false, true, 1},
      {true, true, true, 2},
  };

  for (const auto& test_case : kTestCases) {
    std::unique_ptr<FakeDataUseAnnotator> annotator(
        test_case.use_annotator ? new FakeDataUseAnnotator() : nullptr);
    std::unique_ptr<DataUseAmortizer> amortizer(
        test_case.use_amortizer ? new DoublingAmortizer() : nullptr);

    Initialize(std::move(annotator), std::move(amortizer));

    const SessionID kFooTabId = SessionID::FromSerializedValue(10);
    const net::NetworkChangeNotifier::ConnectionType kFooConnectionType =
        net::NetworkChangeNotifier::CONNECTION_2G;
    const std::string kFooMccMnc = "foo_mcc_mnc";
    std::unique_ptr<net::URLRequest> foo_request =
        ExecuteRequest(GURL("http://foo.com"), GURL("http://foofirstparty.com"),
                       kFooTabId, kFooConnectionType, kFooMccMnc);

    const SessionID kBarTabId = SessionID::FromSerializedValue(20);
    const net::NetworkChangeNotifier::ConnectionType kBarConnectionType =
        net::NetworkChangeNotifier::CONNECTION_WIFI;
    const std::string kBarMccMnc = "bar_mcc_mnc";
    std::unique_ptr<net::URLRequest> bar_request =
        ExecuteRequest(GURL("http://bar.com"), GURL("http://barfirstparty.com"),
                       kBarTabId, kBarConnectionType, kBarMccMnc);

    auto data_use_it = test_observer()->observed_data_use().begin();

    // First, the |foo_request| data use should have happened.
    int64_t observed_foo_tx_bytes = 0, observed_foo_rx_bytes = 0;
    while (data_use_it != test_observer()->observed_data_use().end() &&
           data_use_it->url == "http://foo.com/") {
      EXPECT_EQ(GetRequestStart(*foo_request), data_use_it->request_start);
      EXPECT_EQ(GURL("http://foofirstparty.com"),
                data_use_it->site_for_cookies);

      if (test_case.expect_tab_ids)
        EXPECT_EQ(kFooTabId, data_use_it->tab_id);
      else
        EXPECT_FALSE(data_use_it->tab_id.is_valid());

      EXPECT_EQ(kFooConnectionType, data_use_it->connection_type);
      EXPECT_EQ(kFooMccMnc, data_use_it->mcc_mnc);

      observed_foo_tx_bytes += data_use_it->tx_bytes;
      observed_foo_rx_bytes += data_use_it->rx_bytes;
      ++data_use_it;
    }
    EXPECT_EQ(foo_request->GetTotalSentBytes() *
                  test_case.expected_amortization_multiple,
              observed_foo_tx_bytes);
    EXPECT_EQ(foo_request->GetTotalReceivedBytes() *
                  test_case.expected_amortization_multiple,
              observed_foo_rx_bytes);

    // Then, the |bar_request| data use should have happened.
    int64_t observed_bar_tx_bytes = 0, observed_bar_rx_bytes = 0;
    while (data_use_it != test_observer()->observed_data_use().end()) {
      EXPECT_EQ(GURL("http://bar.com"), data_use_it->url);
      EXPECT_EQ(GetRequestStart(*bar_request), data_use_it->request_start);
      EXPECT_EQ(GURL("http://barfirstparty.com"),
                data_use_it->site_for_cookies);

      if (test_case.expect_tab_ids)
        EXPECT_EQ(kBarTabId, data_use_it->tab_id);
      else
        EXPECT_FALSE(data_use_it->tab_id.is_valid());

      EXPECT_EQ(kBarConnectionType, data_use_it->connection_type);
      EXPECT_EQ(kBarMccMnc, data_use_it->mcc_mnc);

      observed_bar_tx_bytes += data_use_it->tx_bytes;
      observed_bar_rx_bytes += data_use_it->rx_bytes;
      ++data_use_it;
    }
    EXPECT_EQ(bar_request->GetTotalSentBytes() *
                  test_case.expected_amortization_multiple,
              observed_bar_tx_bytes);
    EXPECT_EQ(bar_request->GetTotalReceivedBytes() *
                  test_case.expected_amortization_multiple,
              observed_bar_rx_bytes);
  }
}

TEST_F(DataUseAggregatorTest, ReportOffTheRecordDataUse) {
  Initialize(std::unique_ptr<FakeDataUseAnnotator>(new FakeDataUseAnnotator()),
             std::unique_ptr<DataUseAmortizer>(new DoublingAmortizer()));

  // Off the record data use should not be reported to observers.
  data_use_aggregator()->ReportOffTheRecordDataUse(1000, 1000);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(static_cast<size_t>(0),
            test_observer()->observed_data_use().size());
}

}  // namespace

}  // namespace data_usage
