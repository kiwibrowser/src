// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/services/proxy_resolution_service_provider.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/thread_test_helper.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chromeos/dbus/services/service_provider_test_helper.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "net/base/net_errors.h"
#include "net/proxy_resolution/mock_proxy_resolver.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/proxy_resolution/proxy_resolver.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_test_util.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

// Runs pending, non-delayed tasks on |task_runner|. Note that delayed tasks or
// additional tasks posted by pending tests will not be run.
void RunPendingTasks(scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  scoped_refptr<base::ThreadTestHelper> helper =
      new base::ThreadTestHelper(task_runner);
  ASSERT_TRUE(helper->Run());
}

// Trivial net::ProxyResolver implementation that returns canned data either
// synchronously or asynchronously.
class TestProxyResolver : public net::ProxyResolver {
 public:
  explicit TestProxyResolver(
      scoped_refptr<base::SingleThreadTaskRunner> network_task_runner)
      : network_task_runner_(network_task_runner) {
    proxy_info_.UseDirect();
  }
  ~TestProxyResolver() override = default;

  const net::ProxyInfo& proxy_info() const { return proxy_info_; }
  net::ProxyInfo* mutable_proxy_info() { return &proxy_info_; }

  void set_async(bool async) { async_ = async; }
  void set_result(net::Error result) { result_ = result; }

  // net::ProxyResolver:
  int GetProxyForURL(const GURL& url,
                     net::ProxyInfo* results,
                     const net::CompletionCallback& callback,
                     std::unique_ptr<Request>* request,
                     const net::NetLogWithSource& net_log) override {
    CHECK(network_task_runner_->BelongsToCurrentThread());
    results->Use(proxy_info_);
    if (!async_)
      return result_;

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(callback, result_));
    return net::ERR_IO_PENDING;
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;

  // Proxy info for GetProxyForURL() to return.
  net::ProxyInfo proxy_info_;

  // If true, GetProxyForURL() replies asynchronously rather than synchronously.
  bool async_ = false;

  // Final result for GetProxyForURL() to return.
  net::Error result_ = net::OK;

  DISALLOW_COPY_AND_ASSIGN(TestProxyResolver);
};

// Trivial net::ProxyResolverFactory implementation that synchronously creates
// net::ForwardingProxyResolvers that forward to a single passed-in resolver.
class TestProxyResolverFactory : public net::ProxyResolverFactory {
 public:
  // Ownership of |resolver| remains with the caller. |resolver| must outlive
  // the forwarding resolvers returned by CreateProxyResolver().
  explicit TestProxyResolverFactory(net::ProxyResolver* resolver)
      : net::ProxyResolverFactory(false /* expects_pac_bytes */),
        resolver_(resolver) {}
  ~TestProxyResolverFactory() override = default;

  // net::ProxyResolverFactory:
  int CreateProxyResolver(const scoped_refptr<net::PacFileData>& pac_script,
                          std::unique_ptr<net::ProxyResolver>* resolver,
                          const net::CompletionCallback& callback,
                          std::unique_ptr<Request>* request) override {
    *resolver = std::make_unique<net::ForwardingProxyResolver>(resolver_);
    return net::OK;
  }

 private:
  net::ProxyResolver* resolver_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(TestProxyResolverFactory);
};

// Test ProxyResolutionServiceProvider::Delegate implementation.
class TestDelegate : public ProxyResolutionServiceProvider::Delegate {
 public:
  explicit TestDelegate(
      const scoped_refptr<base::SingleThreadTaskRunner>& network_task_runner,
      net::ProxyResolver* proxy_resolver)
      : proxy_resolver_(proxy_resolver),
        context_getter_(
            new net::TestURLRequestContextGetter(network_task_runner)) {
    network_task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(
            &TestDelegate::CreateProxyResolutionServiceOnNetworkThread,
            base::Unretained(this)));
    RunPendingTasks(network_task_runner);
  }

  ~TestDelegate() override {
    context_getter_->GetNetworkTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(&TestDelegate::DeleteProxyServiceOnNetworkThread,
                       base::Unretained(this)));
    RunPendingTasks(context_getter_->GetNetworkTaskRunner());
  }

  // ProxyResolutionServiceProvider::Delegate:
  scoped_refptr<net::URLRequestContextGetter> GetRequestContext() override {
    return context_getter_;
  }

 private:
  // Helper method for the constructor that initializes
  // |proxy_resolution_service_| and injects it into |context_getter_|'s context.
  void CreateProxyResolutionServiceOnNetworkThread() {
    CHECK(context_getter_->GetNetworkTaskRunner()->BelongsToCurrentThread());

    // Setting a mandatory PAC URL makes |proxy_resolution_service_| query
    // |proxy_resolver_| and also lets us generate
    // net::ERR_MANDATORY_PROXY_CONFIGURATION_FAILED errors.
    net::ProxyConfig config;
    config.set_pac_url(GURL("http://www.example.com"));
    config.set_pac_mandatory(true);
    proxy_resolution_service_ = std::make_unique<net::ProxyResolutionService>(
        std::make_unique<net::ProxyConfigServiceFixed>(
            net::ProxyConfigWithAnnotation(config,
                                           TRAFFIC_ANNOTATION_FOR_TESTS)),
        std::make_unique<TestProxyResolverFactory>(proxy_resolver_),
        nullptr /* net_log */);
    context_getter_->GetURLRequestContext()->set_proxy_resolution_service(
        proxy_resolution_service_.get());
  }

  // Helper method for the destructor that resets |proxy_resolution_service_|.
  void DeleteProxyServiceOnNetworkThread() {
    CHECK(context_getter_->GetNetworkTaskRunner()->BelongsToCurrentThread());
    proxy_resolution_service_.reset();
  }

  net::ProxyResolver* proxy_resolver_;  // Not owned.

  // Created, used, and destroyed on the network thread (since
  // net::ProxyResolutionService is thread-affine (uses ThreadChecker)).
  std::unique_ptr<net::ProxyResolutionService> proxy_resolution_service_;

  scoped_refptr<net::TestURLRequestContextGetter> context_getter_;

  DISALLOW_COPY_AND_ASSIGN(TestDelegate);
};

}  // namespace

class ProxyResolutionServiceProviderTest : public testing::Test {
 public:
  ProxyResolutionServiceProviderTest() : network_thread_("NetworkThread") {
    CHECK(network_thread_.Start());

    proxy_resolver_ =
        std::make_unique<TestProxyResolver>(network_thread_.task_runner());
    service_provider_ = std::make_unique<ProxyResolutionServiceProvider>(
        std::make_unique<TestDelegate>(network_thread_.task_runner(),
                                       proxy_resolver_.get()));
    test_helper_.SetUp(
        kNetworkProxyServiceName, dbus::ObjectPath(kNetworkProxyServicePath),
        kNetworkProxyServiceInterface, kNetworkProxyServiceResolveProxyMethod,
        service_provider_.get());
  }

  ~ProxyResolutionServiceProviderTest() override {
    test_helper_.TearDown();

    // URLRequestContextGetter posts a task to delete itself to its task runner,
    // so give it a chance to do that.
    service_provider_.reset();
    RunPendingTasks(network_thread_.task_runner());

    network_thread_.Stop();
  }

 protected:
  // Makes a D-Bus call to |service_provider_|'s ResolveProxy method and returns
  // the response.
  std::unique_ptr<dbus::Response> CallMethod(const std::string& source_url) {
    dbus::MethodCall method_call(kNetworkProxyServiceInterface,
                                 kNetworkProxyServiceResolveProxyMethod);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(source_url);
    return test_helper_.CallMethod(&method_call);
  }

  // Thread used to perform network operations.
  base::Thread network_thread_;

  std::unique_ptr<TestProxyResolver> proxy_resolver_;
  std::unique_ptr<ProxyResolutionServiceProvider> service_provider_;
  ServiceProviderTestHelper test_helper_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolutionServiceProviderTest);
};

TEST_F(ProxyResolutionServiceProviderTest, Sync) {
  const char kSourceURL[] = "http://www.gmail.com/";
  std::unique_ptr<dbus::Response> response = CallMethod(kSourceURL);

  // The response should contain the proxy info and an empty error.
  ASSERT_TRUE(response);
  dbus::MessageReader reader(response.get());
  std::string proxy_info, error;
  EXPECT_TRUE(reader.PopString(&proxy_info));
  EXPECT_TRUE(reader.PopString(&error));
  EXPECT_EQ(proxy_resolver_->proxy_info().ToPacString(), proxy_info);
  EXPECT_EQ("", error);
}

TEST_F(ProxyResolutionServiceProviderTest, Async) {
  const char kSourceURL[] = "http://www.gmail.com/";
  proxy_resolver_->set_async(true);
  proxy_resolver_->mutable_proxy_info()->UseNamedProxy("http://localhost:8080");
  std::unique_ptr<dbus::Response> response = CallMethod(kSourceURL);

  // The response should contain the proxy info and an empty error.
  ASSERT_TRUE(response);
  dbus::MessageReader reader(response.get());
  std::string proxy_info, error;
  EXPECT_TRUE(reader.PopString(&proxy_info));
  EXPECT_TRUE(reader.PopString(&error));
  EXPECT_EQ(proxy_resolver_->proxy_info().ToPacString(), proxy_info);
  EXPECT_EQ("", error);
}

TEST_F(ProxyResolutionServiceProviderTest, Error) {
  const char kSourceURL[] = "http://www.gmail.com/";
  proxy_resolver_->set_result(net::ERR_FAILED);
  std::unique_ptr<dbus::Response> response = CallMethod(kSourceURL);

  // The response should contain empty proxy info and a "mandatory proxy config
  // failed" error (which the error from the resolver will be mapped to).
  ASSERT_TRUE(response);
  dbus::MessageReader reader(response.get());
  std::string proxy_info, error;
  EXPECT_TRUE(reader.PopString(&proxy_info));
  EXPECT_TRUE(reader.PopString(&error));
  EXPECT_EQ("DIRECT", proxy_info);
  EXPECT_EQ(net::ErrorToString(net::ERR_MANDATORY_PROXY_CONFIGURATION_FAILED),
            error);
}

}  // namespace chromeos
