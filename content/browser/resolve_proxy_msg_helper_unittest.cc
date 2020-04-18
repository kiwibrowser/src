// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/resolve_proxy_msg_helper.h"

#include <tuple>

#include "base/memory/ptr_util.h"
#include "content/common/view_messages.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "ipc/ipc_test_sink.h"
#include "net/base/net_errors.h"
#include "net/proxy_resolution/mock_proxy_resolver.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

// This ProxyConfigService always returns "http://pac" as the PAC url to use.
class MockProxyConfigService : public net::ProxyConfigService {
 public:
  void AddObserver(Observer* observer) override {}
  void RemoveObserver(Observer* observer) override {}
  ConfigAvailability GetLatestProxyConfig(
      net::ProxyConfigWithAnnotation* results) override {
    *results = net::ProxyConfigWithAnnotation(
        net::ProxyConfig::CreateFromCustomPacURL(GURL("http://pac")),
        TRAFFIC_ANNOTATION_FOR_TESTS);
    return CONFIG_VALID;
  }
};

class TestResolveProxyMsgHelper : public ResolveProxyMsgHelper {
 public:
  TestResolveProxyMsgHelper(net::ProxyResolutionService* proxy_resolution_service,
                            IPC::Listener* listener)
      : ResolveProxyMsgHelper(proxy_resolution_service), listener_(listener) {}
  bool Send(IPC::Message* message) override {
    listener_->OnMessageReceived(*message);
    delete message;
    return true;
  }

 protected:
  ~TestResolveProxyMsgHelper() override {}

  IPC::Listener* listener_;
};

class ResolveProxyMsgHelperTest : public testing::Test, public IPC::Listener {
 public:
  struct PendingResult {
    PendingResult(bool result,
                  const std::string& proxy_list)
        : result(result), proxy_list(proxy_list) {
    }

    bool result;
    std::string proxy_list;
  };

  ResolveProxyMsgHelperTest()
      : resolver_factory_(new net::MockAsyncProxyResolverFactory(false)),
        service_(new net::ProxyResolutionService(
            base::WrapUnique(new MockProxyConfigService),
            base::WrapUnique(resolver_factory_),
            nullptr)),
        helper_(new TestResolveProxyMsgHelper(service_.get(), this)) {
    test_sink_.AddFilter(this);
  }

 protected:
  const PendingResult* pending_result() const { return pending_result_.get(); }

  void clear_pending_result() {
    pending_result_.reset();
  }

  IPC::Message* GenerateReply() {
    bool temp_bool;
    std::string temp_string;
    ViewHostMsg_ResolveProxy message(GURL(), &temp_bool, &temp_string);
    return IPC::SyncMessage::GenerateReply(&message);
  }

  net::MockAsyncProxyResolverFactory* resolver_factory_;
  net::MockAsyncProxyResolver resolver_;
  std::unique_ptr<net::ProxyResolutionService> service_;
  scoped_refptr<ResolveProxyMsgHelper> helper_;
  std::unique_ptr<PendingResult> pending_result_;

 private:
  bool OnMessageReceived(const IPC::Message& msg) override {
    ViewHostMsg_ResolveProxy::ReplyParam reply_data;
    EXPECT_TRUE(ViewHostMsg_ResolveProxy::ReadReplyParam(&msg, &reply_data));
    DCHECK(!pending_result_.get());
    pending_result_.reset(
        new PendingResult(std::get<0>(reply_data), std::get<1>(reply_data)));
    test_sink_.ClearMessages();
    return true;
  }

  TestBrowserThreadBundle thread_bundle_;
  IPC::TestSink test_sink_;
};

// Issue three sequential requests -- each should succeed.
TEST_F(ResolveProxyMsgHelperTest, Sequential) {
  GURL url1("http://www.google1.com/");
  GURL url2("http://www.google2.com/");
  GURL url3("http://www.google3.com/");

  // Messages are deleted by the sink.
  IPC::Message* msg1 = GenerateReply();
  IPC::Message* msg2 = GenerateReply();
  IPC::Message* msg3 = GenerateReply();

  // Execute each request sequentially (so there are never 2 requests
  // outstanding at the same time).

  helper_->OnResolveProxy(url1, msg1);

  // Finish ProxyResolutionService's initialization.
  ASSERT_EQ(1u, resolver_factory_->pending_requests().size());
  resolver_factory_->pending_requests()[0]->CompleteNowWithForwarder(
      net::OK, &resolver_);

  ASSERT_EQ(1u, resolver_.pending_jobs().size());
  EXPECT_EQ(url1, resolver_.pending_jobs()[0]->url());
  resolver_.pending_jobs()[0]->results()->UseNamedProxy("result1:80");
  resolver_.pending_jobs()[0]->CompleteNow(net::OK);

  // Check result.
  EXPECT_EQ(true, pending_result()->result);
  EXPECT_EQ("PROXY result1:80", pending_result()->proxy_list);
  clear_pending_result();

  helper_->OnResolveProxy(url2, msg2);

  ASSERT_EQ(1u, resolver_.pending_jobs().size());
  EXPECT_EQ(url2, resolver_.pending_jobs()[0]->url());
  resolver_.pending_jobs()[0]->results()->UseNamedProxy("result2:80");
  resolver_.pending_jobs()[0]->CompleteNow(net::OK);

  // Check result.
  EXPECT_EQ(true, pending_result()->result);
  EXPECT_EQ("PROXY result2:80", pending_result()->proxy_list);
  clear_pending_result();

  helper_->OnResolveProxy(url3, msg3);

  ASSERT_EQ(1u, resolver_.pending_jobs().size());
  EXPECT_EQ(url3, resolver_.pending_jobs()[0]->url());
  resolver_.pending_jobs()[0]->results()->UseNamedProxy("result3:80");
  resolver_.pending_jobs()[0]->CompleteNow(net::OK);

  // Check result.
  EXPECT_EQ(true, pending_result()->result);
  EXPECT_EQ("PROXY result3:80", pending_result()->proxy_list);
  clear_pending_result();
}

// Issue a request while one is already in progress -- should be queued.
TEST_F(ResolveProxyMsgHelperTest, QueueRequests) {
  GURL url1("http://www.google1.com/");
  GURL url2("http://www.google2.com/");
  GURL url3("http://www.google3.com/");

  IPC::Message* msg1 = GenerateReply();
  IPC::Message* msg2 = GenerateReply();
  IPC::Message* msg3 = GenerateReply();

  // Start three requests. Since the proxy resolver is async, all the
  // requests will be pending.

  helper_->OnResolveProxy(url1, msg1);

  // Finish ProxyResolutionService's initialization.
  ASSERT_EQ(1u, resolver_factory_->pending_requests().size());
  resolver_factory_->pending_requests()[0]->CompleteNowWithForwarder(
      net::OK, &resolver_);

  helper_->OnResolveProxy(url2, msg2);
  helper_->OnResolveProxy(url3, msg3);

  // ResolveProxyHelper only keeps 1 request outstanding in
  // ProxyResolutionService at a time.
  ASSERT_EQ(1u, resolver_.pending_jobs().size());
  EXPECT_EQ(url1, resolver_.pending_jobs()[0]->url());

  resolver_.pending_jobs()[0]->results()->UseNamedProxy("result1:80");
  resolver_.pending_jobs()[0]->CompleteNow(net::OK);

  // Check result.
  EXPECT_EQ(true, pending_result()->result);
  EXPECT_EQ("PROXY result1:80", pending_result()->proxy_list);
  clear_pending_result();

  ASSERT_EQ(1u, resolver_.pending_jobs().size());
  EXPECT_EQ(url2, resolver_.pending_jobs()[0]->url());

  resolver_.pending_jobs()[0]->results()->UseNamedProxy("result2:80");
  resolver_.pending_jobs()[0]->CompleteNow(net::OK);

  // Check result.
  EXPECT_EQ(true, pending_result()->result);
  EXPECT_EQ("PROXY result2:80", pending_result()->proxy_list);
  clear_pending_result();

  ASSERT_EQ(1u, resolver_.pending_jobs().size());
  EXPECT_EQ(url3, resolver_.pending_jobs()[0]->url());

  resolver_.pending_jobs()[0]->results()->UseNamedProxy("result3:80");
  resolver_.pending_jobs()[0]->CompleteNow(net::OK);

  // Check result.
  EXPECT_EQ(true, pending_result()->result);
  EXPECT_EQ("PROXY result3:80", pending_result()->proxy_list);
  clear_pending_result();
}

// Delete the helper while a request is in progress, and others are pending.
TEST_F(ResolveProxyMsgHelperTest, CancelPendingRequests) {
  GURL url1("http://www.google1.com/");
  GURL url2("http://www.google2.com/");
  GURL url3("http://www.google3.com/");

  // They will be deleted by the request's cancellation.
  IPC::Message* msg1 = GenerateReply();
  IPC::Message* msg2 = GenerateReply();
  IPC::Message* msg3 = GenerateReply();

  // Start three requests. Since the proxy resolver is async, all the
  // requests will be pending.

  helper_->OnResolveProxy(url1, msg1);

  // Finish ProxyResolutionService's initialization.
  ASSERT_EQ(1u, resolver_factory_->pending_requests().size());
  resolver_factory_->pending_requests()[0]->CompleteNowWithForwarder(
      net::OK, &resolver_);

  helper_->OnResolveProxy(url2, msg2);
  helper_->OnResolveProxy(url3, msg3);

  // ResolveProxyHelper only keeps 1 request outstanding in
  // ProxyResolutionService at a time.
  ASSERT_EQ(1u, resolver_.pending_jobs().size());
  EXPECT_EQ(url1, resolver_.pending_jobs()[0]->url());

  // Delete the underlying ResolveProxyMsgHelper -- this should cancel all
  // the requests which are outstanding.
  helper_ = nullptr;

  // The pending requests sent to the proxy resolver should have been cancelled.

  EXPECT_EQ(0u, resolver_.pending_jobs().size());

  EXPECT_TRUE(pending_result() == nullptr);

  // It should also be the case that msg1, msg2, msg3 were deleted by the
  // cancellation. (Else will show up as a leak in Valgrind).
}

}  // namespace content
