// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "net/base/mock_network_change_notifier.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/network_context.h"
#include "services/network/network_service.h"
#include "services/network/public/mojom/network_change_manager.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/test/test_url_loader_client.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "services/service_manager/public/mojom/service_factory.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

const char kNetworkServiceName[] = "network";

mojom::NetworkContextParamsPtr CreateContextParams() {
  mojom::NetworkContextParamsPtr params = mojom::NetworkContextParams::New();
  // Use a fixed proxy config, to avoid dependencies on local network
  // configuration.
  params->initial_proxy_config = net::ProxyConfigWithAnnotation::CreateDirect();
  return params;
}

class NetworkServiceTest : public testing::Test {
 public:
  NetworkServiceTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        service_(NetworkService::CreateForTesting()) {}
  ~NetworkServiceTest() override {}

  NetworkService* service() const { return service_.get(); }

  void DestroyService() { service_.reset(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<NetworkService> service_;
};

// Test shutdown in the case a NetworkContext is destroyed before the
// NetworkService.
TEST_F(NetworkServiceTest, CreateAndDestroyContext) {
  mojom::NetworkContextPtr network_context;
  service()->CreateNetworkContext(mojo::MakeRequest(&network_context),
                                  CreateContextParams());
  network_context.reset();
  // Make sure the NetworkContext is destroyed.
  base::RunLoop().RunUntilIdle();
}

// Test shutdown in the case there is still a live NetworkContext when the
// NetworkService is destroyed. The service should destroy the NetworkContext
// itself.
TEST_F(NetworkServiceTest, DestroyingServiceDestroysContext) {
  mojom::NetworkContextPtr network_context;
  service()->CreateNetworkContext(mojo::MakeRequest(&network_context),
                                  CreateContextParams());
  base::RunLoop run_loop;
  network_context.set_connection_error_handler(run_loop.QuitClosure());
  DestroyService();

  // Destroying the service should destroy the context, causing a connection
  // error.
  run_loop.Run();
}

TEST_F(NetworkServiceTest, CreateContextWithoutChannelID) {
  mojom::NetworkContextParamsPtr params = CreateContextParams();
  params->cookie_path = base::FilePath();
  mojom::NetworkContextPtr network_context;
  service()->CreateNetworkContext(mojo::MakeRequest(&network_context),
                                  std::move(params));
  network_context.reset();
  // Make sure the NetworkContext is destroyed.
  base::RunLoop().RunUntilIdle();
}

namespace {

class ServiceTestClient : public service_manager::test::ServiceTestClient,
                          public service_manager::mojom::ServiceFactory {
 public:
  explicit ServiceTestClient(service_manager::test::ServiceTest* test)
      : service_manager::test::ServiceTestClient(test) {
    registry_.AddInterface<service_manager::mojom::ServiceFactory>(base::Bind(
        &ServiceTestClient::BindServiceFactoryRequest, base::Unretained(this)));
  }
  ~ServiceTestClient() override {}

 protected:
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  void CreateService(
      service_manager::mojom::ServiceRequest request,
      const std::string& name,
      service_manager::mojom::PIDReceiverPtr pid_receiver) override {
    if (name == kNetworkServiceName) {
      service_context_.reset(new service_manager::ServiceContext(
          NetworkService::CreateForTesting(), std::move(request)));
    }
  }

  void BindServiceFactoryRequest(
      service_manager::mojom::ServiceFactoryRequest request) {
    service_factory_bindings_.AddBinding(this, std::move(request));
  }

  std::unique_ptr<service_manager::ServiceContext> service_context_;

 private:
  service_manager::BinderRegistry registry_;
  mojo::BindingSet<service_manager::mojom::ServiceFactory>
      service_factory_bindings_;
};

}  // namespace

class NetworkServiceTestWithService
    : public service_manager::test::ServiceTest {
 public:
  NetworkServiceTestWithService()
      : ServiceTest("network_unittests",
                    base::test::ScopedTaskEnvironment::MainThreadType::IO) {}
  ~NetworkServiceTestWithService() override {}

  void LoadURL(const GURL& url) {
    ResourceRequest request;
    request.url = url;
    request.method = "GET";
    request.request_initiator = url::Origin();
    StartLoadingURL(request, 0);
    client_->RunUntilComplete();
  }

  void StartLoadingURL(const ResourceRequest& request, uint32_t process_id) {
    client_.reset(new TestURLLoaderClient());
    mojom::URLLoaderFactoryPtr loader_factory;
    mojom::URLLoaderFactoryParamsPtr params =
        mojom::URLLoaderFactoryParams::New();
    params->process_id = process_id;
    params->is_corb_enabled = false;
    network_context_->CreateURLLoaderFactory(mojo::MakeRequest(&loader_factory),
                                             std::move(params));

    loader_factory->CreateLoaderAndStart(
        mojo::MakeRequest(&loader_), 1, 1, mojom::kURLLoadOptionNone, request,
        client_->CreateInterfacePtr(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  }

  net::EmbeddedTestServer* test_server() { return &test_server_; }
  TestURLLoaderClient* client() { return client_.get(); }
  mojom::URLLoader* loader() { return loader_.get(); }
  mojom::NetworkService* service() { return network_service_.get(); }
  mojom::NetworkContext* context() { return network_context_.get(); }

 private:
  std::unique_ptr<service_manager::Service> CreateService() override {
    return std::make_unique<ServiceTestClient>(this);
  }

  void SetUp() override {
    base::FilePath services_test_data(FILE_PATH_LITERAL("services/test/data"));
    test_server_.AddDefaultHandlers(services_test_data);
    ASSERT_TRUE(test_server_.Start());
    service_manager::test::ServiceTest::SetUp();
    connector()->BindInterface(kNetworkServiceName, &network_service_);
    mojom::NetworkContextParamsPtr context_params =
        mojom::NetworkContextParams::New();
    network_service_->CreateNetworkContext(mojo::MakeRequest(&network_context_),
                                           std::move(context_params));
  }

  net::EmbeddedTestServer test_server_;
  std::unique_ptr<TestURLLoaderClient> client_;
  mojom::NetworkServicePtr network_service_;
  mojom::NetworkContextPtr network_context_;
  mojom::URLLoaderPtr loader_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceTestWithService);
};

// Verifies that loading a URL through the network service's mojo interface
// works.
TEST_F(NetworkServiceTestWithService, Basic) {
  LoadURL(test_server()->GetURL("/echo"));
  EXPECT_EQ(net::OK, client()->completion_status().error_code);
}

// Verifies that raw headers are only reported if requested.
TEST_F(NetworkServiceTestWithService, RawRequestHeadersAbsent) {
  ResourceRequest request;
  request.url = test_server()->GetURL("/server-redirect?/echo");
  request.method = "GET";
  request.request_initiator = url::Origin();
  StartLoadingURL(request, 0);
  client()->RunUntilRedirectReceived();
  EXPECT_TRUE(client()->has_received_redirect());
  EXPECT_TRUE(!client()->response_head().raw_request_response_info);
  loader()->FollowRedirect(base::nullopt);
  client()->RunUntilComplete();
  EXPECT_TRUE(!client()->response_head().raw_request_response_info);
}

TEST_F(NetworkServiceTestWithService, RawRequestHeadersPresent) {
  ResourceRequest request;
  request.url = test_server()->GetURL("/server-redirect?/echo");
  request.method = "GET";
  request.report_raw_headers = true;
  request.request_initiator = url::Origin();
  StartLoadingURL(request, 0);
  client()->RunUntilRedirectReceived();
  EXPECT_TRUE(client()->has_received_redirect());
  {
    scoped_refptr<HttpRawRequestResponseInfo> request_response_info =
        client()->response_head().raw_request_response_info;
    ASSERT_TRUE(request_response_info);
    EXPECT_EQ(301, request_response_info->http_status_code);
    EXPECT_EQ("Moved Permanently", request_response_info->http_status_text);
    EXPECT_TRUE(base::StartsWith(request_response_info->request_headers_text,
                                 "GET /server-redirect?/echo HTTP/1.1\r\n",
                                 base::CompareCase::SENSITIVE));
    EXPECT_GE(request_response_info->request_headers.size(), 1lu);
    EXPECT_GE(request_response_info->response_headers.size(), 1lu);
    EXPECT_TRUE(base::StartsWith(request_response_info->response_headers_text,
                                 "HTTP/1.1 301 Moved Permanently\r",
                                 base::CompareCase::SENSITIVE));
  }
  loader()->FollowRedirect(base::nullopt);
  client()->RunUntilComplete();
  {
    scoped_refptr<HttpRawRequestResponseInfo> request_response_info =
        client()->response_head().raw_request_response_info;
    EXPECT_EQ(200, request_response_info->http_status_code);
    EXPECT_EQ("OK", request_response_info->http_status_text);
    EXPECT_TRUE(base::StartsWith(request_response_info->request_headers_text,
                                 "GET /echo HTTP/1.1\r\n",
                                 base::CompareCase::SENSITIVE));
    EXPECT_GE(request_response_info->request_headers.size(), 1lu);
    EXPECT_GE(request_response_info->response_headers.size(), 1lu);
    EXPECT_TRUE(base::StartsWith(request_response_info->response_headers_text,
                                 "HTTP/1.1 200 OK\r",
                                 base::CompareCase::SENSITIVE));
  }
}

TEST_F(NetworkServiceTestWithService, RawRequestAccessControl) {
  const uint32_t process_id = 42;
  ResourceRequest request;
  request.url = test_server()->GetURL("/nocache.html");
  request.method = "GET";
  request.report_raw_headers = true;
  request.request_initiator = url::Origin();

  StartLoadingURL(request, process_id);
  client()->RunUntilComplete();
  EXPECT_FALSE(client()->response_head().raw_request_response_info);
  service()->SetRawHeadersAccess(process_id, true);
  StartLoadingURL(request, process_id);
  client()->RunUntilComplete();
  {
    scoped_refptr<HttpRawRequestResponseInfo> request_response_info =
        client()->response_head().raw_request_response_info;
    ASSERT_TRUE(request_response_info);
    EXPECT_EQ(200, request_response_info->http_status_code);
    EXPECT_EQ("OK", request_response_info->http_status_text);
  }

  service()->SetRawHeadersAccess(process_id, false);
  StartLoadingURL(request, process_id);
  client()->RunUntilComplete();
  EXPECT_FALSE(client()->response_head().raw_request_response_info.get());
}

TEST_F(NetworkServiceTestWithService, SetNetworkConditions) {
  mojom::NetworkConditionsPtr network_conditions =
      mojom::NetworkConditions::New();
  network_conditions->offline = true;
  context()->SetNetworkConditions("42", std::move(network_conditions));

  ResourceRequest request;
  request.url = test_server()->GetURL("/nocache.html");
  request.method = "GET";

  StartLoadingURL(request, 0);
  client()->RunUntilComplete();
  EXPECT_EQ(net::OK, client()->completion_status().error_code);

  request.headers.AddHeaderFromString(
      "X-DevTools-Emulate-Network-Conditions-Client-Id: 42");
  StartLoadingURL(request, 0);
  client()->RunUntilComplete();
  EXPECT_EQ(net::ERR_INTERNET_DISCONNECTED,
            client()->completion_status().error_code);

  network_conditions = mojom::NetworkConditions::New();
  network_conditions->offline = false;
  context()->SetNetworkConditions("42", std::move(network_conditions));
  StartLoadingURL(request, 0);
  client()->RunUntilComplete();
  EXPECT_EQ(net::OK, client()->completion_status().error_code);

  network_conditions = mojom::NetworkConditions::New();
  network_conditions->offline = true;
  context()->SetNetworkConditions("42", std::move(network_conditions));

  request.headers.AddHeaderFromString(
      "X-DevTools-Emulate-Network-Conditions-Client-Id: 42");
  StartLoadingURL(request, 0);
  client()->RunUntilComplete();
  EXPECT_EQ(net::ERR_INTERNET_DISCONNECTED,
            client()->completion_status().error_code);
  context()->SetNetworkConditions("42", nullptr);
  StartLoadingURL(request, 0);
  client()->RunUntilComplete();
  EXPECT_EQ(net::OK, client()->completion_status().error_code);
}

class TestNetworkChangeManagerClient
    : public mojom::NetworkChangeManagerClient {
 public:
  explicit TestNetworkChangeManagerClient(
      mojom::NetworkService* network_service)
      : connection_type_(mojom::ConnectionType::CONNECTION_UNKNOWN),
        binding_(this) {
    mojom::NetworkChangeManagerPtr manager_ptr;
    mojom::NetworkChangeManagerRequest request(mojo::MakeRequest(&manager_ptr));
    network_service->GetNetworkChangeManager(std::move(request));

    mojom::NetworkChangeManagerClientPtr client_ptr;
    mojom::NetworkChangeManagerClientRequest client_request(
        mojo::MakeRequest(&client_ptr));
    binding_.Bind(std::move(client_request));
    manager_ptr->RequestNotifications(std::move(client_ptr));
  }

  ~TestNetworkChangeManagerClient() override {}

  // NetworkChangeManagerClient implementation:
  void OnInitialConnectionType(mojom::ConnectionType type) override {
    if (type == connection_type_)
      run_loop_.Quit();
  }

  void OnNetworkChanged(mojom::ConnectionType type) override {
    if (type == connection_type_)
      run_loop_.Quit();
  }

  // Waits for the desired |connection_type| notification.
  void WaitForNotification(mojom::ConnectionType type) {
    connection_type_ = type;
    run_loop_.Run();
  }

 private:
  base::RunLoop run_loop_;
  mojom::ConnectionType connection_type_;
  mojo::Binding<mojom::NetworkChangeManagerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkChangeManagerClient);
};

class NetworkChangeTest : public testing::Test {
 public:
  NetworkChangeTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {
    service_ = NetworkService::CreateForTesting();
  }

  ~NetworkChangeTest() override {}

  NetworkService* service() const { return service_.get(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
#if defined(OS_ANDROID)
  // On Android, NetworkChangeNotifier setup is more involved and needs to
  // to be split between UI thread and network thread. Use a mock
  // NetworkChangeNotifier in tests, so the test setup is simpler.
  net::test::MockNetworkChangeNotifier network_change_notifier_;
#endif
  std::unique_ptr<NetworkService> service_;
};

// mojom:NetworkChangeManager isn't supported on these platforms.
// See the same ifdef in CreateNetworkChangeNotifierIfNeeded.
#if defined(OS_CHROMEOS) || defined(OS_FUCHSIA) || defined(OS_IOS)
#define MAYBE_NetworkChangeManagerRequest DISABLED_NetworkChangeManagerRequest
#else
#define MAYBE_NetworkChangeManagerRequest NetworkChangeManagerRequest
#endif
TEST_F(NetworkChangeTest, MAYBE_NetworkChangeManagerRequest) {
  TestNetworkChangeManagerClient manager_client(service());
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_3G);
  manager_client.WaitForNotification(mojom::ConnectionType::CONNECTION_3G);
}

class NetworkServiceNetworkChangeTest
    : public service_manager::test::ServiceTest {
 public:
  NetworkServiceNetworkChangeTest()
      : ServiceTest("network_unittests",
                    base::test::ScopedTaskEnvironment::MainThreadType::IO) {}
  ~NetworkServiceNetworkChangeTest() override {}

  mojom::NetworkService* service() { return network_service_.get(); }

 private:
  // A ServiceTestClient that broadcasts a network change notification in the
  // network service's process.
  class ServiceTestClientWithNetworkChange : public ServiceTestClient {
   public:
    explicit ServiceTestClientWithNetworkChange(
        service_manager::test::ServiceTest* test)
        : ServiceTestClient(test) {}
    ~ServiceTestClientWithNetworkChange() override {}

   protected:
    void CreateService(
        service_manager::mojom::ServiceRequest request,
        const std::string& name,
        service_manager::mojom::PIDReceiverPtr pid_receiver) override {
      if (name == kNetworkServiceName) {
        service_context_.reset(new service_manager::ServiceContext(
            NetworkService::CreateForTesting(), std::move(request)));
        // Send a broadcast after NetworkService is actually created.
        // Otherwise, this NotifyObservers is a no-op.
        net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
            net::NetworkChangeNotifier::CONNECTION_3G);
      }
    }
  };
  std::unique_ptr<service_manager::Service> CreateService() override {
    return std::make_unique<ServiceTestClientWithNetworkChange>(this);
  }

  void SetUp() override {
    service_manager::test::ServiceTest::SetUp();
    connector()->BindInterface(kNetworkServiceName, &network_service_);
  }

  mojom::NetworkServicePtr network_service_;
#if defined(OS_ANDROID)
  // On Android, NetworkChangeNotifier setup is more involved and needs
  // to be split between UI thread and network thread. Use a mock
  // NetworkChangeNotifier in tests, so the test setup is simpler.
  net::test::MockNetworkChangeNotifier network_change_notifier_;
#endif

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceNetworkChangeTest);
};

TEST_F(NetworkServiceNetworkChangeTest, MAYBE_NetworkChangeManagerRequest) {
  TestNetworkChangeManagerClient manager_client(service());
  manager_client.WaitForNotification(mojom::ConnectionType::CONNECTION_3G);
}

}  // namespace

}  // namespace network
