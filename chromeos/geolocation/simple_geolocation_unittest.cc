// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/shill_manager_client.h"
#include "chromeos/geolocation/simple_geolocation_provider.h"
#include "chromeos/geolocation/simple_geolocation_request_test_monitor.h"
#include "chromeos/network/geolocation_handler.h"
#include "chromeos/network/network_handler.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_impl.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace {

constexpr int kRequestRetryIntervalMilliSeconds = 200;

// This should be different from default to prevent SimpleGeolocationRequest
// from modifying it.
constexpr char kTestGeolocationProviderUrl[] =
    "https://localhost/geolocation/v1/geolocate?";

constexpr char kSimpleResponseBody[] =
    "{\n"
    "  \"location\": {\n"
    "    \"lat\": 51.0,\n"
    "    \"lng\": -0.1\n"
    "  },\n"
    "  \"accuracy\": 1200.4\n"
    "}";
constexpr char kIPOnlyRequestBody[] = "{\"considerIp\": \"true\"}";
constexpr char kOneWiFiAPRequestBody[] =
    "{"
    "\"considerIp\":true,"
    "\"wifiAccessPoints\":["
    "{"
    "\"channel\":1,"
    "\"macAddress\":\"01:00:00:00:00:00\","
    "\"signalStrength\":10,"
    "\"signalToNoiseRatio\":0"
    "}"
    "]"
    "}";
constexpr char kOneCellTowerRequestBody[] =
    "{"
    "\"cellTowers\":["
    "{"
    "\"cellId\":\"1\","
    "\"locationAreaCode\":\"3\","
    "\"mobileCountryCode\":\"100\","
    "\"mobileNetworkCode\":\"101\""
    "}"
    "],"
    "\"considerIp\":true"
    "}";
constexpr char kExpectedPosition[] =
    "latitude=51.000000, longitude=-0.100000, accuracy=1200.400000, "
    "error_code=0, error_message='', status=1 (OK)";

constexpr char kWiFiAP1MacAddress[] = "01:00:00:00:00:00";
constexpr char kCellTower1MNC[] = "101";
}  // anonymous namespace

namespace chromeos {

// This is helper class for net::FakeURLFetcherFactory.
class TestGeolocationAPIURLFetcherCallback {
 public:
  TestGeolocationAPIURLFetcherCallback(const GURL& url,
                                       const size_t require_retries,
                                       const std::string& response,
                                       SimpleGeolocationProvider* provider)
      : url_(url),
        require_retries_(require_retries),
        response_(response),
        factory_(nullptr),
        attempts_(0),
        provider_(provider) {}

  std::unique_ptr<net::FakeURLFetcher> CreateURLFetcher(
      const GURL& url,
      net::URLFetcherDelegate* delegate,
      const std::string& response_data,
      net::HttpStatusCode response_code,
      net::URLRequestStatus::Status status) {
    EXPECT_EQ(provider_->requests_.size(), 1U);

    SimpleGeolocationRequest* geolocation_request =
        provider_->requests_[0].get();

    const base::TimeDelta base_retry_interval =
        base::TimeDelta::FromMilliseconds(kRequestRetryIntervalMilliSeconds);
    geolocation_request->set_retry_sleep_on_server_error_for_testing(
        base_retry_interval);
    geolocation_request->set_retry_sleep_on_bad_response_for_testing(
        base_retry_interval);

    ++attempts_;
    if (attempts_ > require_retries_) {
      response_code = net::HTTP_OK;
      status = net::URLRequestStatus::SUCCESS;
      factory_->SetFakeResponse(url, response_, response_code, status);
    }
    std::unique_ptr<net::FakeURLFetcher> fetcher(new net::FakeURLFetcher(
        url, delegate, response_, response_code, status));
    scoped_refptr<net::HttpResponseHeaders> download_headers =
        new net::HttpResponseHeaders(std::string());
    download_headers->AddHeader("Content-Type: application/json");
    fetcher->set_response_headers(download_headers);
    return fetcher;
  }

  void Initialize(net::FakeURLFetcherFactory* factory) {
    factory_ = factory;
    factory_->SetFakeResponse(url_,
                              std::string(),
                              net::HTTP_INTERNAL_SERVER_ERROR,
                              net::URLRequestStatus::FAILED);
  }

  size_t attempts() const { return attempts_; }

 private:
  const GURL url_;
  // Respond with OK on required retry attempt.
  const size_t require_retries_;
  std::string response_;
  net::FakeURLFetcherFactory* factory_;
  size_t attempts_;
  SimpleGeolocationProvider* provider_;

  DISALLOW_COPY_AND_ASSIGN(TestGeolocationAPIURLFetcherCallback);
};

// This implements fake Google MAPS Geolocation API remote endpoint.
// Response data is served to SimpleGeolocationProvider via
// net::FakeURLFetcher.
class GeolocationAPIFetcherFactory {
 public:
  GeolocationAPIFetcherFactory(const GURL& url,
                               const std::string& response,
                               const size_t require_retries,
                               SimpleGeolocationProvider* provider) {
    url_callback_.reset(new TestGeolocationAPIURLFetcherCallback(
        url, require_retries, response, provider));
    net::URLFetcherImpl::set_factory(nullptr);
    fetcher_factory_.reset(new net::FakeURLFetcherFactory(
        nullptr,
        base::Bind(&TestGeolocationAPIURLFetcherCallback::CreateURLFetcher,
                   base::Unretained(url_callback_.get()))));
    url_callback_->Initialize(fetcher_factory_.get());
  }

  size_t attempts() const { return url_callback_->attempts(); }

 private:
  std::unique_ptr<TestGeolocationAPIURLFetcherCallback> url_callback_;
  std::unique_ptr<net::FakeURLFetcherFactory> fetcher_factory_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationAPIFetcherFactory);
};

class GeolocationReceiver {
 public:
  GeolocationReceiver() : server_error_(false) {}

  void OnRequestDone(const Geoposition& position,
                     bool server_error,
                     const base::TimeDelta elapsed) {
    position_ = position;
    server_error_ = server_error;
    elapsed_ = elapsed;

    message_loop_runner_->Quit();
  }

  void WaitUntilRequestDone() {
    message_loop_runner_.reset(new base::RunLoop);
    message_loop_runner_->Run();
  }

  const Geoposition& position() const { return position_; }
  bool server_error() const { return server_error_; }
  base::TimeDelta elapsed() const { return elapsed_; }

 private:
  Geoposition position_;
  bool server_error_;
  base::TimeDelta elapsed_;
  std::unique_ptr<base::RunLoop> message_loop_runner_;
};

class WirelessTestMonitor : public SimpleGeolocationRequestTestMonitor {
 public:
  WirelessTestMonitor() = default;

  void OnRequestCreated(SimpleGeolocationRequest* request) override {}
  void OnStart(SimpleGeolocationRequest* request) override {
    last_request_body_ = request->FormatRequestBodyForTesting();
  }

  const std::string& last_request_body() const { return last_request_body_; }

 private:
  std::string last_request_body_;

  DISALLOW_COPY_AND_ASSIGN(WirelessTestMonitor);
};

class SimpleGeolocationTest : public testing::Test {
 private:
  base::MessageLoop message_loop_;
};

TEST_F(SimpleGeolocationTest, ResponseOK) {
  SimpleGeolocationProvider provider(nullptr,
                                     GURL(kTestGeolocationProviderUrl));

  GeolocationAPIFetcherFactory url_factory(GURL(kTestGeolocationProviderUrl),
                                           std::string(kSimpleResponseBody),
                                           0 /* require_retries */,
                                           &provider);

  GeolocationReceiver receiver;
  provider.RequestGeolocation(base::TimeDelta::FromSeconds(1), false, false,
                              base::Bind(&GeolocationReceiver::OnRequestDone,
                                         base::Unretained(&receiver)));
  receiver.WaitUntilRequestDone();

  EXPECT_EQ(kExpectedPosition, receiver.position().ToString());
  EXPECT_FALSE(receiver.server_error());
  EXPECT_EQ(1U, url_factory.attempts());
}

TEST_F(SimpleGeolocationTest, ResponseOKWithRetries) {
  SimpleGeolocationProvider provider(nullptr,
                                     GURL(kTestGeolocationProviderUrl));

  GeolocationAPIFetcherFactory url_factory(GURL(kTestGeolocationProviderUrl),
                                           std::string(kSimpleResponseBody),
                                           3 /* require_retries */,
                                           &provider);

  GeolocationReceiver receiver;
  provider.RequestGeolocation(base::TimeDelta::FromSeconds(1), false, false,
                              base::Bind(&GeolocationReceiver::OnRequestDone,
                                         base::Unretained(&receiver)));
  receiver.WaitUntilRequestDone();
  EXPECT_EQ(kExpectedPosition, receiver.position().ToString());
  EXPECT_FALSE(receiver.server_error());
  EXPECT_EQ(4U, url_factory.attempts());
}

TEST_F(SimpleGeolocationTest, InvalidResponse) {
  SimpleGeolocationProvider provider(nullptr,
                                     GURL(kTestGeolocationProviderUrl));

  GeolocationAPIFetcherFactory url_factory(GURL(kTestGeolocationProviderUrl),
                                           "invalid JSON string",
                                           0 /* require_retries */,
                                           &provider);

  GeolocationReceiver receiver;

  const int timeout_seconds = 1;
  size_t expected_retries = static_cast<size_t>(
      timeout_seconds * 1000 / kRequestRetryIntervalMilliSeconds);
  ASSERT_GE(expected_retries, 2U);

  provider.RequestGeolocation(base::TimeDelta::FromSeconds(timeout_seconds),
                              false, false,
                              base::Bind(&GeolocationReceiver::OnRequestDone,
                                         base::Unretained(&receiver)));
  receiver.WaitUntilRequestDone();

  EXPECT_EQ(
      "latitude=200.000000, longitude=200.000000, accuracy=-1.000000, "
      "error_code=0, error_message='SimpleGeolocation provider at "
      "'https://localhost/' : JSONReader failed: Line: 1, column: 1, "
      "Unexpected token..', status=4 (TIMEOUT)",
      receiver.position().ToString());
  EXPECT_TRUE(receiver.server_error());
  EXPECT_GE(url_factory.attempts(), 2U);
  if (url_factory.attempts() > expected_retries + 1) {
    LOG(WARNING)
        << "SimpleGeolocationTest::InvalidResponse: Too many attempts ("
        << url_factory.attempts() << "), no more than " << expected_retries + 1
        << " expected.";
  }
  if (url_factory.attempts() < expected_retries - 1) {
    LOG(WARNING)
        << "SimpleGeolocationTest::InvalidResponse: Too little attempts ("
        << url_factory.attempts() << "), greater than " << expected_retries - 1
        << " expected.";
  }
}

TEST_F(SimpleGeolocationTest, NoWiFi) {
  // This initializes DBusThreadManager and markes it "for tests only".
  DBusThreadManager::GetSetterForTesting();
  NetworkHandler::Initialize();

  WirelessTestMonitor requests_monitor;
  SimpleGeolocationRequest::SetTestMonitor(&requests_monitor);

  SimpleGeolocationProvider provider(nullptr,
                                     GURL(kTestGeolocationProviderUrl));

  GeolocationAPIFetcherFactory url_factory(GURL(kTestGeolocationProviderUrl),
                                           std::string(kSimpleResponseBody),
                                           0 /* require_retries */, &provider);

  GeolocationReceiver receiver;
  provider.RequestGeolocation(base::TimeDelta::FromSeconds(1), true, false,
                              base::Bind(&GeolocationReceiver::OnRequestDone,
                                         base::Unretained(&receiver)));
  receiver.WaitUntilRequestDone();
  EXPECT_EQ(kIPOnlyRequestBody, requests_monitor.last_request_body());

  EXPECT_EQ(kExpectedPosition, receiver.position().ToString());
  EXPECT_FALSE(receiver.server_error());
  EXPECT_EQ(1U, url_factory.attempts());

  NetworkHandler::Shutdown();
  DBusThreadManager::Shutdown();
}

// Test sending of WiFi Access points and Cell Towers.
// (This is mostly derived from GeolocationHandlerTest.)
class SimpleGeolocationWirelessTest : public ::testing::TestWithParam<bool> {
 public:
  SimpleGeolocationWirelessTest() : manager_test_(nullptr) {}

  ~SimpleGeolocationWirelessTest() override = default;

  void SetUp() override {
    // This initializes DBusThreadManager and markes it "for tests only".
    DBusThreadManager::GetSetterForTesting();
    // Get the test interface for manager / device.
    manager_test_ =
        DBusThreadManager::Get()->GetShillManagerClient()->GetTestInterface();
    ASSERT_TRUE(manager_test_);
    geolocation_handler_.reset(new GeolocationHandler());
    geolocation_handler_->Init();
    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override {
    geolocation_handler_.reset();
    DBusThreadManager::Shutdown();
  }

  bool GetWifiAccessPoints() {
    return geolocation_handler_->GetWifiAccessPoints(&wifi_access_points_,
                                                     nullptr);
  }

  bool GetCellTowers() {
    return geolocation_handler_->GetNetworkInformation(nullptr, &cell_towers_);
  }

  // This should remain in sync with the format of shill (chromeos) dict entries
  void AddAccessPoint(int idx) {
    base::DictionaryValue properties;
    std::string mac_address =
        base::StringPrintf("%02X:%02X:%02X:%02X:%02X:%02X", idx, 0, 0, 0, 0, 0);
    std::string channel = base::IntToString(idx);
    std::string strength = base::IntToString(idx * 10);
    properties.SetKey(shill::kGeoMacAddressProperty, base::Value(mac_address));
    properties.SetKey(shill::kGeoChannelProperty, base::Value(channel));
    properties.SetKey(shill::kGeoSignalStrengthProperty, base::Value(strength));
    manager_test_->AddGeoNetwork(shill::kGeoWifiAccessPointsProperty,
                                 properties);
    base::RunLoop().RunUntilIdle();
  }

  // This should remain in sync with the format of shill (chromeos) dict entries
  void AddCellTower(int idx) {
    base::DictionaryValue properties;
    std::string ci = base::IntToString(idx);
    std::string lac = base::IntToString(idx * 3);
    std::string mcc = base::IntToString(idx * 100);
    std::string mnc = base::IntToString(idx * 100 + 1);

    properties.SetKey(shill::kGeoCellIdProperty, base::Value(ci));
    properties.SetKey(shill::kGeoLocationAreaCodeProperty, base::Value(lac));
    properties.SetKey(shill::kGeoMobileCountryCodeProperty, base::Value(mcc));
    properties.SetKey(shill::kGeoMobileNetworkCodeProperty, base::Value(mnc));

    manager_test_->AddGeoNetwork(shill::kGeoCellTowersProperty, properties);
    base::RunLoop().RunUntilIdle();
  }

 protected:
  base::MessageLoopForUI message_loop_;
  std::unique_ptr<GeolocationHandler> geolocation_handler_;
  ShillManagerClient::TestInterface* manager_test_;
  WifiAccessPointVector wifi_access_points_;
  CellTowerVector cell_towers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SimpleGeolocationWirelessTest);
};

// Parameter is enable/disable sending of WiFi data.
TEST_P(SimpleGeolocationWirelessTest, WiFiExists) {
  NetworkHandler::Initialize();

  WirelessTestMonitor requests_monitor;
  SimpleGeolocationRequest::SetTestMonitor(&requests_monitor);

  SimpleGeolocationProvider provider(nullptr,
                                     GURL(kTestGeolocationProviderUrl));

  GeolocationAPIFetcherFactory url_factory(GURL(kTestGeolocationProviderUrl),
                                           std::string(kSimpleResponseBody),
                                           0 /* require_retries */, &provider);
  {
    GeolocationReceiver receiver;
    provider.RequestGeolocation(base::TimeDelta::FromSeconds(1), GetParam(),
                                false,
                                base::Bind(&GeolocationReceiver::OnRequestDone,
                                           base::Unretained(&receiver)));
    receiver.WaitUntilRequestDone();
    EXPECT_EQ(kIPOnlyRequestBody, requests_monitor.last_request_body());

    EXPECT_EQ(kExpectedPosition, receiver.position().ToString());
    EXPECT_FALSE(receiver.server_error());
    EXPECT_EQ(1U, url_factory.attempts());
  }

  // Add cell and wifi to ensure only wifi is sent when cellular disabled.
  AddAccessPoint(1);
  AddCellTower(1);
  base::RunLoop().RunUntilIdle();
  // Initial call should return false and request access points.
  EXPECT_FALSE(GetWifiAccessPoints());
  base::RunLoop().RunUntilIdle();
  // Second call should return true since we have an access point.
  EXPECT_TRUE(GetWifiAccessPoints());
  ASSERT_EQ(1u, wifi_access_points_.size());
  EXPECT_EQ(kWiFiAP1MacAddress, wifi_access_points_[0].mac_address);
  EXPECT_EQ(1, wifi_access_points_[0].channel);

  {
    GeolocationReceiver receiver;
    provider.RequestGeolocation(base::TimeDelta::FromSeconds(1), GetParam(),
                                false,
                                base::Bind(&GeolocationReceiver::OnRequestDone,
                                           base::Unretained(&receiver)));
    receiver.WaitUntilRequestDone();
    if (GetParam()) {
      // Sending WiFi data is enabled.
      EXPECT_EQ(kOneWiFiAPRequestBody, requests_monitor.last_request_body());
    } else {
      // Sending WiFi data is disabled.
      EXPECT_EQ(kIPOnlyRequestBody, requests_monitor.last_request_body());
    }

    EXPECT_EQ(kExpectedPosition, receiver.position().ToString());
    EXPECT_FALSE(receiver.server_error());
    // This is total.
    EXPECT_EQ(2U, url_factory.attempts());
  }
  NetworkHandler::Shutdown();
}

// This test verifies that WiFi data is sent only if sending was requested.
INSTANTIATE_TEST_CASE_P(EnableDisableSendingWifiData,
                        SimpleGeolocationWirelessTest,
                        testing::Bool());

TEST_P(SimpleGeolocationWirelessTest, CellularExists) {
  NetworkHandler::Initialize();

  WirelessTestMonitor requests_monitor;
  SimpleGeolocationRequest::SetTestMonitor(&requests_monitor);

  SimpleGeolocationProvider provider(nullptr,
                                     GURL(kTestGeolocationProviderUrl));

  GeolocationAPIFetcherFactory url_factory(GURL(kTestGeolocationProviderUrl),
                                           std::string(kSimpleResponseBody),
                                           0 /* require_retries */, &provider);
  {
    GeolocationReceiver receiver;
    provider.RequestGeolocation(base::TimeDelta::FromSeconds(1), false,
                                GetParam(),
                                base::Bind(&GeolocationReceiver::OnRequestDone,
                                           base::Unretained(&receiver)));
    receiver.WaitUntilRequestDone();
    EXPECT_EQ(kIPOnlyRequestBody, requests_monitor.last_request_body());

    EXPECT_EQ(kExpectedPosition, receiver.position().ToString());
    EXPECT_FALSE(receiver.server_error());
    EXPECT_EQ(1U, url_factory.attempts());
  }

  AddCellTower(1);
  base::RunLoop().RunUntilIdle();
  // Initial call should return false and request cell towers.
  EXPECT_FALSE(GetCellTowers());
  base::RunLoop().RunUntilIdle();
  // Second call should return true since we have a tower.
  EXPECT_TRUE(GetCellTowers());
  ASSERT_EQ(1u, cell_towers_.size());
  EXPECT_EQ(kCellTower1MNC, cell_towers_[0].mnc);
  EXPECT_EQ(base::IntToString(1), cell_towers_[0].ci);

  {
    GeolocationReceiver receiver;
    provider.RequestGeolocation(base::TimeDelta::FromSeconds(1), false,
                                GetParam(),
                                base::Bind(&GeolocationReceiver::OnRequestDone,
                                           base::Unretained(&receiver)));
    receiver.WaitUntilRequestDone();
    if (GetParam()) {
      // Sending Cellular data is enabled.
      EXPECT_EQ(kOneCellTowerRequestBody, requests_monitor.last_request_body());
    } else {
      // Sending Cellular data is disabled.
      EXPECT_EQ(kIPOnlyRequestBody, requests_monitor.last_request_body());
    }

    EXPECT_EQ(kExpectedPosition, receiver.position().ToString());
    EXPECT_FALSE(receiver.server_error());
    // This is total.
    EXPECT_EQ(2U, url_factory.attempts());
  }
  NetworkHandler::Shutdown();
}

}  // namespace chromeos
