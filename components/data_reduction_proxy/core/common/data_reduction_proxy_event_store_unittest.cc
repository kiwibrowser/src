// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_store.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_configurator.h"
#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_event_creator.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params_test_utils.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_server.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/base/proxy_server.h"
#include "net/http/http_status_code.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source_type.h"
#include "net/log/net_log_with_source.h"
#include "net/log/test_net_log.h"
#include "net/log/test_net_log_entry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace data_reduction_proxy {

class DataReductionProxyEventStoreTest : public testing::Test {
 public:
  DataReductionProxyEventStoreTest() : net_log_(new net::TestNetLog()) {
    net_log_with_source_ = net::NetLogWithSource::Make(
        net_log_.get(), net::NetLogSourceType::DATA_REDUCTION_PROXY);
  }

  void SetUp() override {
    event_store_.reset(new DataReductionProxyEventStore());
    event_creator_.reset(
        new DataReductionProxyEventCreator(event_store_.get()));
  }

  net::TestNetLogEntry GetSingleEntry() const {
    net::TestNetLogEntry::List entries;
    net_log_->GetEntries(&entries);
    EXPECT_EQ(1u, entries.size());
    return entries[0];
  }

  net::TestNetLogEntry GetLatestEntry() const {
    net::TestNetLogEntry::List entries;
    net_log_->GetEntries(&entries);
    return entries.back();
  }

  DataReductionProxyEventStore* event_store() { return event_store_.get(); }

  DataReductionProxyEventCreator* event_creator() {
    return event_creator_.get();
  }

  net::TestNetLog* net_log() { return net_log_.get(); }

  const net::NetLogWithSource& net_log_with_source() {
    return net_log_with_source_;
  }

  size_t event_count() const { return event_store_->stored_events_.size(); }

  DataReductionProxyEventStorageDelegate::SecureProxyCheckState
  secure_proxy_check_state() const {
    return event_store_->secure_proxy_check_state_;
  }

  base::Value* last_bypass_event() const {
    return event_store_->last_bypass_event_.get();
  }

 private:
  std::unique_ptr<net::TestNetLog> net_log_;
  std::unique_ptr<DataReductionProxyEventStore> event_store_;
  std::unique_ptr<DataReductionProxyEventCreator> event_creator_;
  net::NetLogWithSource net_log_with_source_;
};

TEST_F(DataReductionProxyEventStoreTest, TestAddProxyEnabledEvent) {
  EXPECT_EQ(0u, event_count());
  std::vector<net::ProxyServer> proxies_for_http;
  event_creator()->AddProxyEnabledEvent(net_log(), false, proxies_for_http);
  EXPECT_EQ(1u, event_count());
  net::TestNetLogEntry entry = GetSingleEntry();
  EXPECT_EQ(net::NetLogEventType::DATA_REDUCTION_PROXY_ENABLED, entry.type);
}

TEST_F(DataReductionProxyEventStoreTest, TestAddProxyDisabledEvent) {
  EXPECT_EQ(0u, event_count());
  event_creator()->AddProxyDisabledEvent(net_log());
  EXPECT_EQ(1u, event_count());
  net::TestNetLogEntry entry = GetSingleEntry();
  EXPECT_EQ(net::NetLogEventType::DATA_REDUCTION_PROXY_ENABLED, entry.type);
}

TEST_F(DataReductionProxyEventStoreTest, TestAddBypassActionEvent) {
  EXPECT_EQ(0u, event_count());
  EXPECT_EQ(nullptr, last_bypass_event());
  event_creator()->AddBypassActionEvent(
      net_log_with_source(), BYPASS_ACTION_TYPE_BYPASS, "GET", GURL(), false,
      base::TimeDelta::FromMinutes(1));
  EXPECT_EQ(1u, event_count());
  net::TestNetLogEntry entry = GetSingleEntry();
  EXPECT_EQ(net::NetLogEventType::DATA_REDUCTION_PROXY_BYPASS_REQUESTED,
            entry.type);
  EXPECT_NE(nullptr, last_bypass_event());
}

TEST_F(DataReductionProxyEventStoreTest, TestAddBypassTypeEvent) {
  EXPECT_EQ(0u, event_count());
  EXPECT_EQ(nullptr, last_bypass_event());
  event_creator()->AddBypassTypeEvent(net_log_with_source(),
                                      BYPASS_EVENT_TYPE_LONG, "GET", GURL(),
                                      false, base::TimeDelta::FromMinutes(1));
  EXPECT_EQ(1u, event_count());
  EXPECT_EQ(1u, net_log()->GetSize());
  net::TestNetLogEntry entry = GetSingleEntry();
  EXPECT_EQ(net::NetLogEventType::DATA_REDUCTION_PROXY_BYPASS_REQUESTED,
            entry.type);
  EXPECT_NE(nullptr, last_bypass_event());
}

TEST_F(DataReductionProxyEventStoreTest, TestAddProxyFallbackEvent) {
  EXPECT_EQ(0u, event_count());
  event_creator()->AddProxyFallbackEvent(net_log(), "bad_proxy",
                                         net::ERR_PROXY_CONNECTION_FAILED);
  EXPECT_EQ(1u, event_count());
  net::TestNetLogEntry entry = GetSingleEntry();
  EXPECT_EQ(net::NetLogEventType::DATA_REDUCTION_PROXY_FALLBACK, entry.type);
}

TEST_F(DataReductionProxyEventStoreTest, TestBeginSecureProxyCheck) {
  EXPECT_EQ(0u, event_count());
  EXPECT_EQ(DataReductionProxyEventStorageDelegate::CHECK_UNKNOWN,
            secure_proxy_check_state());
  event_creator()->BeginSecureProxyCheck(net_log_with_source(), GURL());
  EXPECT_EQ(1u, event_count());
  EXPECT_EQ(1u, net_log()->GetSize());
  net::TestNetLogEntry entry = GetSingleEntry();
  EXPECT_EQ(net::NetLogEventType::DATA_REDUCTION_PROXY_CANARY_REQUEST,
            entry.type);
  EXPECT_EQ(DataReductionProxyEventStorageDelegate::CHECK_PENDING,
            secure_proxy_check_state());
}

TEST_F(DataReductionProxyEventStoreTest, TestEndSecureProxyCheck) {
  EXPECT_EQ(0u, event_count());
  EXPECT_EQ(DataReductionProxyEventStorageDelegate::CHECK_UNKNOWN,
            secure_proxy_check_state());
  event_creator()->EndSecureProxyCheck(net_log_with_source(), 0, net::HTTP_OK,
                                       true);
  EXPECT_EQ(1u, event_count());
  EXPECT_EQ(1u, net_log()->GetSize());
  net::TestNetLogEntry entry = GetSingleEntry();
  EXPECT_EQ(net::NetLogEventType::DATA_REDUCTION_PROXY_CANARY_REQUEST,
            entry.type);
  EXPECT_EQ(DataReductionProxyEventStorageDelegate::CHECK_SUCCESS,
            secure_proxy_check_state());
}

TEST_F(DataReductionProxyEventStoreTest, TestEndSecureProxyCheckFailed) {
  const struct {
    const char* test_case;
    int net_error;
    int http_response_code;
    bool secure_proxy_check_succeeded;
    bool expected_proxy_check_succeeded;
  } tests[] = {
      {
       "Succeeded", net::OK, net::HTTP_OK, true, true,
      },
      {
       "Net failure", net::ERR_CONNECTION_RESET, -1, false, false,
      },
      {
       "HTTP failure", net::OK, net::HTTP_NOT_FOUND, false, false,
      },
      {
       "Bad content", net::OK, net::HTTP_OK, false, false,
      },
  };
  size_t expected_event_count = 0;
  EXPECT_EQ(expected_event_count, net_log()->GetSize());
  EXPECT_EQ(expected_event_count++, event_count());
  EXPECT_EQ(DataReductionProxyEventStorageDelegate::CHECK_UNKNOWN,
            secure_proxy_check_state());
  for (const auto& test : tests) {
    event_creator()->EndSecureProxyCheck(net_log_with_source(), test.net_error,
                                         test.http_response_code,
                                         test.secure_proxy_check_succeeded);
    EXPECT_EQ(expected_event_count, net_log()->GetSize()) << test.test_case;
    EXPECT_EQ(expected_event_count++, event_count()) << test.test_case;
    net::TestNetLogEntry entry = GetLatestEntry();
    EXPECT_EQ(net::NetLogEventType::DATA_REDUCTION_PROXY_CANARY_REQUEST,
              entry.type)
        << test.test_case;
    EXPECT_EQ(test.secure_proxy_check_succeeded
                  ? DataReductionProxyEventStorageDelegate::CHECK_SUCCESS
                  : DataReductionProxyEventStorageDelegate::CHECK_FAILED,
              secure_proxy_check_state())
        << test.test_case;
  }
}

TEST_F(DataReductionProxyEventStoreTest, TestFeedbackMethods) {
  base::MessageLoopForIO loop;
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  NetworkPropertiesManager manager(base::DefaultClock::GetInstance(),
                                   &test_prefs,
                                   base::ThreadTaskRunnerHandle::Get());
  DataReductionProxyConfigurator configurator(net_log(), event_creator());
  EXPECT_EQ(std::string(), event_store()->GetHttpProxyList());
  EXPECT_EQ(std::string(), event_store()->SanitizedLastBypassEvent());

  std::vector<DataReductionProxyServer> http_proxies;
  http_proxies.push_back(DataReductionProxyServer(
      net::ProxyServer(net::ProxyServer::SCHEME_HTTP,
                       net::HostPortPair("foo.com", 80)),
      ProxyServer::CORE));
  http_proxies.push_back(DataReductionProxyServer(
      net::ProxyServer(net::ProxyServer::SCHEME_HTTPS,
                       net::HostPortPair("bar.com", 443)),
      ProxyServer::CORE));
  configurator.Enable(manager, http_proxies);
  EXPECT_EQ("foo.com:80;https://bar.com:443",
            event_store()->GetHttpProxyList());

  configurator.Disable();
  EXPECT_EQ(std::string(), event_store()->GetHttpProxyList());
}

TEST_F(DataReductionProxyEventStoreTest, TestFeedbackLastBypassEventFullURL) {
  base::MessageLoopForIO loop;
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  NetworkPropertiesManager manager(base::DefaultClock::GetInstance(),
                                   &test_prefs,
                                   base::ThreadTaskRunnerHandle::Get());
  DataReductionProxyConfigurator configurator(net_log(), event_creator());
  std::vector<DataReductionProxyServer> http_proxies;
  configurator.Enable(manager, http_proxies);

  std::unique_ptr<base::DictionaryValue> bypass_event(
      new base::DictionaryValue());
  std::unique_ptr<base::DictionaryValue> bypass_params(
      new base::DictionaryValue());
  std::unique_ptr<base::DictionaryValue> sanitized_event(
      new base::DictionaryValue());

  // Set bypass event time to be 4 minutes ago.
  std::string time = net::NetLog::TickCountToString(
      base::TimeTicks::Now() - base::TimeDelta::FromMinutes(4));
  bypass_event->SetString("time", time);
  sanitized_event->SetString("bypass_time", time);
  bypass_params->SetInteger("bypass_type", 4);
  sanitized_event->SetInteger("bypass_type", 4);
  bypass_params->SetString("bypass_duration_seconds", "40");
  sanitized_event->SetString("bypass_seconds", "40");
  bypass_params->SetString("url", "http://www.foo.com/bar?baz=1234");
  sanitized_event->SetString("url", "http://www.foo.com/bar");
  bypass_params->SetString("method", "GET");
  sanitized_event->SetString("method", "GET");

  bypass_event->Set("params", std::move(bypass_params));
  std::string sanitized_output;
  base::JSONWriter::Write(*sanitized_event, &sanitized_output);
  event_store()->AddAndSetLastBypassEvent(std::move(bypass_event), 0);
  EXPECT_EQ(sanitized_output, event_store()->SanitizedLastBypassEvent());
}

TEST_F(DataReductionProxyEventStoreTest, TestFeedbackLastBypassEventHostOnly) {
  base::MessageLoopForIO loop;
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  NetworkPropertiesManager manager(base::DefaultClock::GetInstance(),
                                   &test_prefs,
                                   base::ThreadTaskRunnerHandle::Get());
  DataReductionProxyConfigurator configurator(net_log(), event_creator());
  std::vector<DataReductionProxyServer> http_proxies;
  configurator.Enable(manager, http_proxies);

  std::unique_ptr<base::DictionaryValue> bypass_event(
      new base::DictionaryValue());
  std::unique_ptr<base::DictionaryValue> bypass_params(
      new base::DictionaryValue());
  std::unique_ptr<base::DictionaryValue> sanitized_event(
      new base::DictionaryValue());

  // Set bypass event time to be 6 minutes ago.
  std::string time = net::NetLog::TickCountToString(
      base::TimeTicks::Now() - base::TimeDelta::FromMinutes(6));
  bypass_event->SetString("time", time);
  sanitized_event->SetString("bypass_time", time);
  bypass_params->SetInteger("bypass_type", 4);
  sanitized_event->SetInteger("bypass_type", 4);
  bypass_params->SetString("bypass_duration_seconds", "40");
  sanitized_event->SetString("bypass_seconds", "40");
  bypass_params->SetString("url", "http://www.foo.com/bar?baz=1234");
  sanitized_event->SetString("url", "www.foo.com");
  bypass_params->SetString("method", "GET");
  sanitized_event->SetString("method", "GET");

  bypass_event->Set("params", std::move(bypass_params));
  std::string sanitized_output;
  base::JSONWriter::Write(*sanitized_event, &sanitized_output);
  event_store()->AddAndSetLastBypassEvent(std::move(bypass_event), 0);
  EXPECT_EQ(sanitized_output, event_store()->SanitizedLastBypassEvent());
}

}  // namespace data_reduction_proxy
