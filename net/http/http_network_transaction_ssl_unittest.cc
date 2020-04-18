// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <vector>

#include "base/deferred_sequenced_task_runner.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "net/base/request_priority.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/mock_cert_verifier.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_auth_handler_mock.h"
#include "net/http/http_network_session.h"
#include "net/http/http_network_transaction.h"
#include "net/http/http_request_info.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/transport_security_state.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/socket/socket_test_util.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/test/gtest_util.h"
#include "net/test/test_with_scoped_task_environment.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::test::IsError;
using net::test::IsOk;

namespace net {

namespace {

class TokenBindingSSLConfigService : public SSLConfigService {
 public:
  TokenBindingSSLConfigService() {
    ssl_config_.token_binding_params.push_back(TB_PARAM_ECDSAP256);
  }

  void GetSSLConfig(SSLConfig* config) override { *config = ssl_config_; }

 private:
  ~TokenBindingSSLConfigService() override = default;

  SSLConfig ssl_config_;
};

}  // namespace

class HttpNetworkTransactionSSLTest : public TestWithScopedTaskEnvironment {
 protected:
  HttpNetworkTransactionSSLTest() = default;

  void SetUp() override {
    ssl_config_service_ = new TokenBindingSSLConfigService;
    session_context_.ssl_config_service = ssl_config_service_.get();

    auth_handler_factory_.reset(new HttpAuthHandlerMock::Factory());
    session_context_.http_auth_handler_factory = auth_handler_factory_.get();

    proxy_resolution_service_ = ProxyResolutionService::CreateDirect();
    session_context_.proxy_resolution_service =
        proxy_resolution_service_.get();

    session_context_.client_socket_factory = &mock_socket_factory_;
    session_context_.host_resolver = &mock_resolver_;
    session_context_.http_server_properties = &http_server_properties_;
    session_context_.cert_verifier = &cert_verifier_;
    session_context_.transport_security_state = &transport_security_state_;
    session_context_.cert_transparency_verifier = &ct_verifier_;
    session_context_.ct_policy_enforcer = &ct_policy_enforcer_;
  }

  HttpRequestInfo* GetRequestInfo(const std::string& url) {
    HttpRequestInfo* request_info = new HttpRequestInfo;
    request_info->url = GURL(url);
    request_info->method = "GET";
    request_info->traffic_annotation =
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);
    request_info_vector_.push_back(base::WrapUnique(request_info));
    return request_info;
  }

  scoped_refptr<SSLConfigService> ssl_config_service_;
  std::unique_ptr<HttpAuthHandlerMock::Factory> auth_handler_factory_;
  std::unique_ptr<ProxyResolutionService> proxy_resolution_service_;

  MockClientSocketFactory mock_socket_factory_;
  MockHostResolver mock_resolver_;
  HttpServerPropertiesImpl http_server_properties_;
  MockCertVerifier cert_verifier_;
  TransportSecurityState transport_security_state_;
  MultiLogCTVerifier ct_verifier_;
  DefaultCTPolicyEnforcer ct_policy_enforcer_;
  HttpNetworkSession::Context session_context_;
  std::vector<std::unique_ptr<HttpRequestInfo>> request_info_vector_;
};

TEST_F(HttpNetworkTransactionSSLTest, ChannelID) {
  ChannelIDService channel_id_service(new DefaultChannelIDStore(NULL));
  session_context_.channel_id_service = &channel_id_service;

  HttpNetworkSession::Params params;
  params.enable_channel_id = true;
  HttpNetworkSession session(params, session_context_);

  HttpNetworkTransaction trans(DEFAULT_PRIORITY, &session);
  TestCompletionCallback callback;
  EXPECT_EQ(ERR_IO_PENDING,
            trans.Start(GetRequestInfo("https://example.com"),
                        callback.callback(), NetLogWithSource()));

  EXPECT_TRUE(trans.server_ssl_config_.channel_id_enabled);
}

#if !defined(OS_IOS)
TEST_F(HttpNetworkTransactionSSLTest, TokenBinding) {
  ChannelIDService channel_id_service(new DefaultChannelIDStore(NULL));
  session_context_.channel_id_service = &channel_id_service;

  SSLSocketDataProvider ssl_data(ASYNC, OK);
  ssl_data.ssl_info.token_binding_negotiated = true;
  ssl_data.ssl_info.token_binding_key_param = TB_PARAM_ECDSAP256;
  mock_socket_factory_.AddSSLSocketDataProvider(&ssl_data);
  MockRead mock_reads[] = {MockRead("HTTP/1.1 200 OK\r\n\r\n"),
                           MockRead(SYNCHRONOUS, OK)};
  StaticSocketDataProvider data(mock_reads, base::span<MockWrite>());
  mock_socket_factory_.AddSocketDataProvider(&data);

  HttpNetworkSession session(HttpNetworkSession::Params(), session_context_);
  HttpNetworkTransaction trans1(DEFAULT_PRIORITY, &session);

  TestCompletionCallback callback;
  int rv = callback.GetResult(
      trans1.Start(GetRequestInfo("https://www.example.com/"),
                   callback.callback(), NetLogWithSource()));
  EXPECT_THAT(rv, IsOk());

  HttpRequestHeaders headers1;
  ASSERT_TRUE(trans1.GetFullRequestHeaders(&headers1));
  std::string token_binding_header1;
  EXPECT_TRUE(headers1.GetHeader(HttpRequestHeaders::kTokenBinding,
                                 &token_binding_header1));

  // Send a second request and verify that the token binding header is the same
  // as in the first request.
  mock_socket_factory_.AddSSLSocketDataProvider(&ssl_data);
  StaticSocketDataProvider data2(mock_reads, base::span<MockWrite>());
  mock_socket_factory_.AddSocketDataProvider(&data2);
  HttpNetworkTransaction trans2(DEFAULT_PRIORITY, &session);

  rv = callback.GetResult(
      trans2.Start(GetRequestInfo("https://www.example.com/"),
                   callback.callback(), NetLogWithSource()));
  EXPECT_THAT(rv, IsOk());

  HttpRequestHeaders headers2;
  ASSERT_TRUE(trans2.GetFullRequestHeaders(&headers2));
  std::string token_binding_header2;
  EXPECT_TRUE(headers2.GetHeader(HttpRequestHeaders::kTokenBinding,
                                 &token_binding_header2));

  EXPECT_EQ(token_binding_header1, token_binding_header2);
}

TEST_F(HttpNetworkTransactionSSLTest, NoTokenBindingOverHttp) {
  ChannelIDService channel_id_service(new DefaultChannelIDStore(NULL));
  session_context_.channel_id_service = &channel_id_service;

  SSLSocketDataProvider ssl_data(ASYNC, OK);
  ssl_data.ssl_info.token_binding_negotiated = true;
  ssl_data.ssl_info.token_binding_key_param = TB_PARAM_ECDSAP256;
  mock_socket_factory_.AddSSLSocketDataProvider(&ssl_data);
  MockRead mock_reads[] = {MockRead("HTTP/1.1 200 OK\r\n\r\n"),
                           MockRead(SYNCHRONOUS, OK)};
  StaticSocketDataProvider data(mock_reads, base::span<MockWrite>());
  mock_socket_factory_.AddSocketDataProvider(&data);

  HttpNetworkSession session(HttpNetworkSession::Params(), session_context_);
  HttpNetworkTransaction trans(DEFAULT_PRIORITY, &session);

  TestCompletionCallback callback;
  int rv =
      callback.GetResult(trans.Start(GetRequestInfo("http://www.example.com/"),
                                     callback.callback(), NetLogWithSource()));
  EXPECT_THAT(rv, IsOk());

  HttpRequestHeaders headers;
  ASSERT_TRUE(trans.GetFullRequestHeaders(&headers));
  std::string token_binding_header;
  EXPECT_FALSE(headers.GetHeader(HttpRequestHeaders::kTokenBinding,
                                 &token_binding_header));
}

// Regression test for https://crbug.com/667683.
TEST_F(HttpNetworkTransactionSSLTest, TokenBindingAsync) {
  // Create a separate thread for ChannelIDService
  // so that asynchronous Channel ID creation can be delayed.
  base::Thread channel_id_thread("ThreadForChannelIDService");
  channel_id_thread.Start();
  scoped_refptr<base::DeferredSequencedTaskRunner> channel_id_runner =
      new base::DeferredSequencedTaskRunner(channel_id_thread.task_runner());
  ChannelIDService channel_id_service(new DefaultChannelIDStore(nullptr));
  channel_id_service.set_task_runner_for_testing(channel_id_runner);
  session_context_.channel_id_service = &channel_id_service;

  SSLSocketDataProvider ssl_data(ASYNC, OK);
  ssl_data.ssl_info.token_binding_negotiated = true;
  ssl_data.ssl_info.token_binding_key_param = TB_PARAM_ECDSAP256;
  ssl_data.next_proto = kProtoHTTP2;
  mock_socket_factory_.AddSSLSocketDataProvider(&ssl_data);

  MockRead reads[] = {MockRead(ASYNC, OK, 0)};
  StaticSocketDataProvider data(reads, base::span<MockWrite>());
  mock_socket_factory_.AddSocketDataProvider(&data);

  HttpRequestInfo request_info;
  request_info.url = GURL("https://www.example.com/");
  request_info.method = "GET";
  request_info.token_binding_referrer = "encrypted.example.com";
  request_info.traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS);

  HttpNetworkSession session(HttpNetworkSession::Params(), session_context_);
  HttpNetworkTransaction trans(DEFAULT_PRIORITY, &session);

  TestCompletionCallback callback;
  int rv = trans.Start(&request_info, callback.callback(), NetLogWithSource());
  EXPECT_THAT(rv, IsError(ERR_IO_PENDING));

  base::RunLoop().RunUntilIdle();

  // When ChannelIdService calls back to HttpNetworkSession,
  // SpdyHttpStream should not crash.
  channel_id_runner->Start();

  rv = callback.WaitForResult();
  EXPECT_THAT(rv, IsError(ERR_CONNECTION_CLOSED));
}
#endif  // !defined(OS_IOS)

}  // namespace net
