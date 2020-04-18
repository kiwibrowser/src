// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/url_request_context_config.h"

#include <memory>

#include "base/json/json_writer.h"
#include "base/test/scoped_task_environment.h"
#include "base/values.h"
#include "net/cert/cert_verifier.h"
#include "net/http/http_network_session.h"
#include "net/log/net_log.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/url_request/http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cronet {

TEST(URLRequestContextConfigTest, TestExperimentalOptionParsing) {
  base::test::ScopedTaskEnvironment scoped_task_environment_(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);

  // Create JSON for experimental options.
  base::DictionaryValue options;
  options.SetPath({"QUIC", "max_server_configs_stored_in_properties"},
                  base::Value(2));
  options.SetPath({"QUIC", "user_agent_id"}, base::Value("Custom QUIC UAID"));
  options.SetPath({"QUIC", "idle_connection_timeout_seconds"},
                  base::Value(300));
  options.SetPath({"QUIC", "close_sessions_on_ip_change"}, base::Value(true));
  options.SetPath({"QUIC", "race_cert_verification"}, base::Value(true));
  options.SetPath({"QUIC", "connection_options"}, base::Value("TIME,TBBR,REJ"));
  options.SetPath({"AsyncDNS", "enable"}, base::Value(true));
  options.SetPath({"NetworkErrorLogging", "enable"}, base::Value(true));
  options.SetPath({"UnknownOption", "foo"}, base::Value(true));
  options.SetPath({"HostResolverRules", "host_resolver_rules"},
                  base::Value("MAP * 127.0.0.1"));
  // See http://crbug.com/696569.
  options.SetKey("disable_ipv6_on_wifi", base::Value(true));
  std::string options_json;
  EXPECT_TRUE(base::JSONWriter::Write(options, &options_json));

  URLRequestContextConfig config(
      // Enable QUIC.
      true,
      // QUIC User Agent ID.
      "Default QUIC User Agent ID",
      // Enable SPDY.
      true,
      // Enable Brotli.
      false,
      // Type of http cache.
      URLRequestContextConfig::HttpCacheType::DISK,
      // Max size of http cache in bytes.
      1024000,
      // Disable caching for HTTP responses. Other information may be stored in
      // the cache.
      false,
      // Storage path for http cache and cookie storage.
      "/data/data/org.chromium.net/app_cronet_test/test_storage",
      // Accept-Language request header field.
      "foreign-language",
      // User-Agent request header field.
      "fake agent",
      // JSON encoded experimental options.
      options_json,
      // MockCertVerifier to use for testing purposes.
      std::unique_ptr<net::CertVerifier>(),
      // Enable network quality estimator.
      false,
      // Enable Public Key Pinning bypass for local trust anchors.
      true,
      // Certificate verifier cache data.
      "");

  net::URLRequestContextBuilder builder;
  net::NetLog net_log;
  config.ConfigureURLRequestContextBuilder(&builder, &net_log);
  EXPECT_FALSE(config.effective_experimental_options->HasKey("UnknownOption"));
  // Set a ProxyConfigService to avoid DCHECK failure when building.
  builder.set_proxy_config_service(
      std::make_unique<net::ProxyConfigServiceFixed>(
          net::ProxyConfigWithAnnotation::CreateDirect()));
  std::unique_ptr<net::URLRequestContext> context(builder.Build());
  const net::HttpNetworkSession::Params* params =
      context->GetNetworkSessionParams();
  // Check Quic Connection options.
  net::QuicTagVector quic_connection_options;
  quic_connection_options.push_back(net::kTIME);
  quic_connection_options.push_back(net::kTBBR);
  quic_connection_options.push_back(net::kREJ);
  EXPECT_EQ(quic_connection_options, params->quic_connection_options);

  // Check Custom QUIC User Agent Id.
  EXPECT_EQ("Custom QUIC UAID", params->quic_user_agent_id);

  // Check max_server_configs_stored_in_properties.
  EXPECT_EQ(2u, params->quic_max_server_configs_stored_in_properties);

  // Check idle_connection_timeout_seconds.
  EXPECT_EQ(300, params->quic_idle_connection_timeout_seconds);

  EXPECT_TRUE(params->quic_close_sessions_on_ip_change);
  EXPECT_FALSE(params->quic_allow_server_migration);
  EXPECT_FALSE(params->quic_migrate_sessions_on_network_change);
  EXPECT_FALSE(params->quic_migrate_sessions_on_network_change_v2);
  EXPECT_FALSE(params->quic_migrate_sessions_early_v2);

  // Check race_cert_verification.
  EXPECT_TRUE(params->quic_race_cert_verification);

#if defined(ENABLE_BUILT_IN_DNS)
  // Check AsyncDNS resolver is enabled (not supported on iOS).
  EXPECT_TRUE(context->host_resolver()->GetDnsConfigAsValue());
#endif  // defined(ENABLE_BUILT_IN_DNS)

#if BUILDFLAG(ENABLE_REPORTING)
  // Check Reporting and Network Error Logging are enabled (can be disabled at
  // build time).
  EXPECT_TRUE(context->reporting_service());
  EXPECT_TRUE(context->network_error_logging_service());
#endif  // BUILDFLAG(ENABLE_REPORTING)

  // Check IPv6 is disabled when on wifi.
  EXPECT_TRUE(context->host_resolver()->GetNoIPv6OnWifi());

  net::HostResolver::RequestInfo info(net::HostPortPair("abcde", 80));
  net::AddressList addresses;
  EXPECT_EQ(net::OK, context->host_resolver()->ResolveFromCache(
                         info, &addresses, net::NetLogWithSource()));
}

TEST(URLRequestContextConfigTest, SetQuicServerMigrationOptions) {
  base::test::ScopedTaskEnvironment scoped_task_environment_(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);

  URLRequestContextConfig config(
      // Enable QUIC.
      true,
      // QUIC User Agent ID.
      "Default QUIC User Agent ID",
      // Enable SPDY.
      true,
      // Enable Brotli.
      false,
      // Type of http cache.
      URLRequestContextConfig::HttpCacheType::DISK,
      // Max size of http cache in bytes.
      1024000,
      // Disable caching for HTTP responses. Other information may be stored in
      // the cache.
      false,
      // Storage path for http cache and cookie storage.
      "/data/data/org.chromium.net/app_cronet_test/test_storage",
      // Accept-Language request header field.
      "foreign-language",
      // User-Agent request header field.
      "fake agent",
      // JSON encoded experimental options.
      "{\"QUIC\":{\"allow_server_migration\":true}}",
      // MockCertVerifier to use for testing purposes.
      std::unique_ptr<net::CertVerifier>(),
      // Enable network quality estimator.
      false,
      // Enable Public Key Pinning bypass for local trust anchors.
      true,
      // Certificate verifier cache data.
      "");

  net::URLRequestContextBuilder builder;
  net::NetLog net_log;
  config.ConfigureURLRequestContextBuilder(&builder, &net_log);
  // Set a ProxyConfigService to avoid DCHECK failure when building.
  builder.set_proxy_config_service(
      std::make_unique<net::ProxyConfigServiceFixed>(
          net::ProxyConfigWithAnnotation::CreateDirect()));
  std::unique_ptr<net::URLRequestContext> context(builder.Build());
  const net::HttpNetworkSession::Params* params =
      context->GetNetworkSessionParams();

  EXPECT_FALSE(params->quic_close_sessions_on_ip_change);
  EXPECT_TRUE(params->quic_allow_server_migration);
}

TEST(URLRequestContextConfigTest, SetQuicConnectionMigrationV2Options) {
  base::test::ScopedTaskEnvironment scoped_task_environment_(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);

  URLRequestContextConfig config(
      // Enable QUIC.
      true,
      // QUIC User Agent ID.
      "Default QUIC User Agent ID",
      // Enable SPDY.
      true,
      // Enable Brotli.
      false,
      // Type of http cache.
      URLRequestContextConfig::HttpCacheType::DISK,
      // Max size of http cache in bytes.
      1024000,
      // Disable caching for HTTP responses. Other information may be stored in
      // the cache.
      false,
      // Storage path for http cache and cookie storage.
      "/data/data/org.chromium.net/app_cronet_test/test_storage",
      // Accept-Language request header field.
      "foreign-language",
      // User-Agent request header field.
      "fake agent",
      // JSON encoded experimental options.
      "{\"QUIC\":{\"migrate_sessions_on_network_change_v2\":true,"
      "\"migrate_sessions_early_v2\":true,"
      "\"max_time_on_non_default_network_seconds\":10,"
      "\"max_migrations_to_non_default_network_on_path_degrading\":4}}",
      // MockCertVerifier to use for testing purposes.
      std::unique_ptr<net::CertVerifier>(),
      // Enable network quality estimator.
      false,
      // Enable Public Key Pinning bypass for local trust anchors.
      true,
      // Certificate verifier cache data.
      "");

  net::URLRequestContextBuilder builder;
  net::NetLog net_log;
  config.ConfigureURLRequestContextBuilder(&builder, &net_log);
  // Set a ProxyConfigService to avoid DCHECK failure when building.
  builder.set_proxy_config_service(
      std::make_unique<net::ProxyConfigServiceFixed>(
          net::ProxyConfigWithAnnotation::CreateDirect()));
  std::unique_ptr<net::URLRequestContext> context(builder.Build());
  const net::HttpNetworkSession::Params* params =
      context->GetNetworkSessionParams();

  EXPECT_TRUE(params->quic_migrate_sessions_on_network_change_v2);
  EXPECT_TRUE(params->quic_migrate_sessions_early_v2);
  EXPECT_EQ(base::TimeDelta::FromSeconds(10),
            params->quic_max_time_on_non_default_network);
  EXPECT_EQ(
      4, params->quic_max_migrations_to_non_default_network_on_path_degrading);
}

TEST(URLRequestContextConfigTest, SetQuicHostWhitelist) {
  base::test::ScopedTaskEnvironment scoped_task_environment_(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);

  URLRequestContextConfig config(
      // Enable QUIC.
      true,
      // QUIC User Agent ID.
      "Default QUIC User Agent ID",
      // Enable SPDY.
      true,
      // Enable Brotli.
      false,
      // Type of http cache.
      URLRequestContextConfig::HttpCacheType::DISK,
      // Max size of http cache in bytes.
      1024000,
      // Disable caching for HTTP responses. Other information may be stored in
      // the cache.
      false,
      // Storage path for http cache and cookie storage.
      "/data/data/org.chromium.net/app_cronet_test/test_storage",
      // Accept-Language request header field.
      "foreign-language",
      // User-Agent request header field.
      "fake agent",
      // JSON encoded experimental options.
      "{\"QUIC\":{\"host_whitelist\":\"www.example.com,www.example.org\"}}",
      // MockCertVerifier to use for testing purposes.
      std::unique_ptr<net::CertVerifier>(),
      // Enable network quality estimator.
      false,
      // Enable Public Key Pinning bypass for local trust anchors.
      true,
      // Certificate verifier cache data.
      "");

  net::URLRequestContextBuilder builder;
  net::NetLog net_log;
  config.ConfigureURLRequestContextBuilder(&builder, &net_log);
  // Set a ProxyConfigService to avoid DCHECK failure when building.
  builder.set_proxy_config_service(
      std::make_unique<net::ProxyConfigServiceFixed>(
          net::ProxyConfigWithAnnotation::CreateDirect()));
  std::unique_ptr<net::URLRequestContext> context(builder.Build());
  const net::HttpNetworkSession::Params* params =
      context->GetNetworkSessionParams();

  EXPECT_TRUE(
      base::ContainsKey(params->quic_host_whitelist, "www.example.com"));
  EXPECT_TRUE(
      base::ContainsKey(params->quic_host_whitelist, "www.example.org"));
}

TEST(URLRequestContextConfigTest, SetQuicMaxTimeBeforeCryptoHandshake) {
  base::test::ScopedTaskEnvironment scoped_task_environment_(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);

  URLRequestContextConfig config(
      // Enable QUIC.
      true,
      // QUIC User Agent ID.
      "Default QUIC User Agent ID",
      // Enable SPDY.
      true,
      // Enable Brotli.
      false,
      // Type of http cache.
      URLRequestContextConfig::HttpCacheType::DISK,
      // Max size of http cache in bytes.
      1024000,
      // Disable caching for HTTP responses. Other information may be stored in
      // the cache.
      false,
      // Storage path for http cache and cookie storage.
      "/data/data/org.chromium.net/app_cronet_test/test_storage",
      // Accept-Language request header field.
      "foreign-language",
      // User-Agent request header field.
      "fake agent",
      // JSON encoded experimental options.
      "{\"QUIC\":{\"max_time_before_crypto_handshake_seconds\":7,"
      "\"max_idle_time_before_crypto_handshake_seconds\":11}}",
      // MockCertVerifier to use for testing purposes.
      std::unique_ptr<net::CertVerifier>(),
      // Enable network quality estimator.
      false,
      // Enable Public Key Pinning bypass for local trust anchors.
      true,
      // Certificate verifier cache data.
      "");

  net::URLRequestContextBuilder builder;
  net::NetLog net_log;
  config.ConfigureURLRequestContextBuilder(&builder, &net_log);
  // Set a ProxyConfigService to avoid DCHECK failure when building.
  builder.set_proxy_config_service(
      std::make_unique<net::ProxyConfigServiceFixed>(
          net::ProxyConfigWithAnnotation::CreateDirect()));
  std::unique_ptr<net::URLRequestContext> context(builder.Build());
  const net::HttpNetworkSession::Params* params =
      context->GetNetworkSessionParams();

  EXPECT_EQ(7, params->quic_max_time_before_crypto_handshake_seconds);
  EXPECT_EQ(11, params->quic_max_idle_time_before_crypto_handshake_seconds);
}

TEST(URLURLRequestContextConfigTest, SetQuicConnectionOptions) {
  base::test::ScopedTaskEnvironment scoped_task_environment_(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);

  URLRequestContextConfig config(
      // Enable QUIC.
      true,
      // QUIC User Agent ID.
      "Default QUIC User Agent ID",
      // Enable SPDY.
      true,
      // Enable Brotli.
      false,
      // Type of http cache.
      URLRequestContextConfig::HttpCacheType::DISK,
      // Max size of http cache in bytes.
      1024000,
      // Disable caching for HTTP responses. Other information may be stored in
      // the cache.
      false,
      // Storage path for http cache and cookie storage.
      "/data/data/org.chromium.net/app_cronet_test/test_storage",
      // Accept-Language request header field.
      "foreign-language",
      // User-Agent request header field.
      "fake agent",
      // JSON encoded experimental options.
      "{\"QUIC\":{\"connection_options\":\"TIME,TBBR,REJ\","
      "\"client_connection_options\":\"TBBR,1RTT\"}}",
      // MockCertVerifier to use for testing purposes.
      std::unique_ptr<net::CertVerifier>(),
      // Enable network quality estimator.
      false,
      // Enable Public Key Pinning bypass for local trust anchors.
      true,
      // Certificate verifier cache data.
      "");

  net::URLRequestContextBuilder builder;
  net::NetLog net_log;
  config.ConfigureURLRequestContextBuilder(&builder, &net_log);
  // Set a ProxyConfigService to avoid DCHECK failure when building.
  builder.set_proxy_config_service(
      std::make_unique<net::ProxyConfigServiceFixed>(
          net::ProxyConfigWithAnnotation::CreateDirect()));
  std::unique_ptr<net::URLRequestContext> context(builder.Build());
  const net::HttpNetworkSession::Params* params =
      context->GetNetworkSessionParams();

  net::QuicTagVector connection_options;
  connection_options.push_back(net::kTIME);
  connection_options.push_back(net::kTBBR);
  connection_options.push_back(net::kREJ);
  EXPECT_EQ(connection_options, params->quic_connection_options);

  net::QuicTagVector client_connection_options;
  client_connection_options.push_back(net::kTBBR);
  client_connection_options.push_back(net::k1RTT);
  EXPECT_EQ(client_connection_options, params->quic_client_connection_options);
}

TEST(URLURLRequestContextConfigTest, SetAcceptLanguageAndUserAgent) {
  base::test::ScopedTaskEnvironment scoped_task_environment_(
      base::test::ScopedTaskEnvironment::MainThreadType::IO);

  URLRequestContextConfig config(
      // Enable QUIC.
      true,
      // QUIC User Agent ID.
      "Default QUIC User Agent ID",
      // Enable SPDY.
      true,
      // Enable Brotli.
      false,
      // Type of http cache.
      URLRequestContextConfig::HttpCacheType::DISK,
      // Max size of http cache in bytes.
      1024000,
      // Disable caching for HTTP responses. Other information may be stored in
      // the cache.
      false,
      // Storage path for http cache and cookie storage.
      "/data/data/org.chromium.net/app_cronet_test/test_storage",
      // Accept-Language request header field.
      "foreign-language",
      // User-Agent request header field.
      "fake agent",
      // JSON encoded experimental options.
      "{}",
      // MockCertVerifier to use for testing purposes.
      std::unique_ptr<net::CertVerifier>(),
      // Enable network quality estimator.
      false,
      // Enable Public Key Pinning bypass for local trust anchors.
      true,
      // Certificate verifier cache data.
      "");

  net::URLRequestContextBuilder builder;
  net::NetLog net_log;
  config.ConfigureURLRequestContextBuilder(&builder, &net_log);
  // Set a ProxyConfigService to avoid DCHECK failure when building.
  builder.set_proxy_config_service(
      std::make_unique<net::ProxyConfigServiceFixed>(
          net::ProxyConfigWithAnnotation::CreateDirect()));
  std::unique_ptr<net::URLRequestContext> context(builder.Build());
  EXPECT_EQ("foreign-language",
            context->http_user_agent_settings()->GetAcceptLanguage());
  EXPECT_EQ("fake agent", context->http_user_agent_settings()->GetUserAgent());
}

// See stale_host_resolver_unittest.cc for test of StaleDNS options.

}  // namespace cronet
