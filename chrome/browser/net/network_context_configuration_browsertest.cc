// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/guid.h"
#include "base/location.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/default_network_context_params.h"
#include "chrome/browser/net/profile_network_context_service.h"
#include "chrome/browser/net/profile_network_context_service_factory.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/simple_url_loader_test_helper.h"
#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "net/base/filename_util.h"
#include "net/base/host_port_pair.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/dns/mock_host_resolver.h"
#include "net/http/http_response_headers.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_server_config.h"
#include "net/test/embedded_test_server/controllable_http_response.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/embedded_test_server_connection_listener.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/resource_response_info.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/test/test_url_loader_client.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kCacheRandomPath[] = "/cacherandom";

// Path using a ControllableHttpResponse that's part of the test fixture.
const char kControllablePath[] = "/controllable";

enum class NetworkServiceState {
  kDisabled,
  kEnabled,
  // Similar to |kEnabled|, but will simulate a crash and run tests again the
  // restarted Network Service process.
  kRestarted,
};

enum class NetworkContextType {
  kSystem,
  kSafeBrowsing,
  kProfile,
  kIncognitoProfile,
};

struct TestCase {
  NetworkServiceState network_service_state;
  NetworkContextType network_context_type;
};

// Tests the system, profile, and incognito profile NetworkContexts.
class NetworkContextConfigurationBrowserTest
    : public InProcessBrowserTest,
      public testing::WithParamInterface<TestCase> {
 public:
  // Backing storage types that can used for various things (HTTP cache,
  // cookies, etc).
  enum class StorageType {
    kNone,
    kMemory,
    kDisk,
  };

  NetworkContextConfigurationBrowserTest() {
    // Have to get a port before setting up the command line, but can only set
    // up the connection listener after there's a main thread, so can't start
    // the test server here.
    EXPECT_TRUE(embedded_test_server()->InitializeAndListen());
  }

  // Returns a cacheable response (10 hours) that is some random text.
  static std::unique_ptr<net::test_server::HttpResponse> HandleCacheRandom(
      const net::test_server::HttpRequest& request) {
    if (request.relative_url != kCacheRandomPath)
      return nullptr;

    std::unique_ptr<net::test_server::BasicHttpResponse> response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_content(base::GenerateGUID());
    response->set_content_type("text/plain");
    response->AddCustomHeader("Cache-Control", "max-age=60000");
    return std::move(response);
  }

  ~NetworkContextConfigurationBrowserTest() override {}

  void SetUpInProcessBrowserTestFixture() override {
    if (GetParam().network_service_state != NetworkServiceState::kDisabled)
      feature_list_.InitAndEnableFeature(network::features::kNetworkService);
  }

  void SetUpOnMainThread() override {
    // Used in a bunch of proxy tests. Should not resolve.
    host_resolver()->AddSimulatedFailure("does.not.resolve.test");

    controllable_http_response_ =
        std::make_unique<net::test_server::ControllableHttpResponse>(
            embedded_test_server(), kControllablePath);
    embedded_test_server()->RegisterRequestHandler(base::BindRepeating(
        &NetworkContextConfigurationBrowserTest::HandleCacheRandom));
    embedded_test_server()->StartAcceptingConnections();

    if (GetParam().network_context_type ==
        NetworkContextType::kIncognitoProfile) {
      incognito_ = CreateIncognitoBrowser();
    }
    SimulateNetworkServiceCrashIfNecessary();
  }

  void TearDownOnMainThread() override {
    // Have to destroy this before the main message loop is torn down. Need to
    // leave the embedded test server up for tests that use
    // |live_during_shutdown_simple_loader_|. It's safe to destroy the
    // ControllableHttpResponse before the test server.
    controllable_http_response_.reset();
  }

  // Returns, as a string, a PAC script that will use the EmbeddedTestServer as
  // a proxy.
  std::string GetPacScript() const {
    return base::StringPrintf(
        "function FindProxyForURL(url, host){ return 'PROXY %s;' }",
        net::HostPortPair::FromURL(embedded_test_server()->base_url())
            .ToString()
            .c_str());
  }

  network::mojom::URLLoaderFactory* loader_factory() const {
    switch (GetParam().network_context_type) {
      case NetworkContextType::kSystem:
        return g_browser_process->system_network_context_manager()
            ->GetURLLoaderFactory();
      case NetworkContextType::kSafeBrowsing:
        return g_browser_process->safe_browsing_service()
            ->GetURLLoaderFactory()
            .get();
      case NetworkContextType::kProfile:
        return content::BrowserContext::GetDefaultStoragePartition(
                   browser()->profile())
            ->GetURLLoaderFactoryForBrowserProcess()
            .get();
      case NetworkContextType::kIncognitoProfile:
        DCHECK(incognito_);
        return content::BrowserContext::GetDefaultStoragePartition(
                   incognito_->profile())
            ->GetURLLoaderFactoryForBrowserProcess()
            .get();
    }
    NOTREACHED();
    return nullptr;
  }

  network::mojom::NetworkContext* network_context() const {
    switch (GetParam().network_context_type) {
      case NetworkContextType::kSystem:
        return g_browser_process->system_network_context_manager()
            ->GetContext();
      case NetworkContextType::kSafeBrowsing:
        return g_browser_process->safe_browsing_service()->GetNetworkContext();
      case NetworkContextType::kProfile:
        return content::BrowserContext::GetDefaultStoragePartition(
                   browser()->profile())
            ->GetNetworkContext();
      case NetworkContextType::kIncognitoProfile:
        DCHECK(incognito_);
        return content::BrowserContext::GetDefaultStoragePartition(
                   incognito_->profile())
            ->GetNetworkContext();
    }
    NOTREACHED();
    return nullptr;
  }

  StorageType GetHttpCacheType() const {
    switch (GetParam().network_context_type) {
      case NetworkContextType::kSystem:
      case NetworkContextType::kSafeBrowsing:
        return StorageType::kNone;
      case NetworkContextType::kProfile:
        return StorageType::kDisk;
      case NetworkContextType::kIncognitoProfile:
        return StorageType::kMemory;
    }
    NOTREACHED();
    return StorageType::kNone;
  }

  // Sets the proxy preference on a PrefService based on the NetworkContextType,
  // and waits for it to be applied.
  void SetProxyPref(const net::HostPortPair& host_port_pair) {
    // Get the correct PrefService.
    PrefService* pref_service = nullptr;
    switch (GetParam().network_context_type) {
      case NetworkContextType::kSystem:
      case NetworkContextType::kSafeBrowsing:
        pref_service = g_browser_process->local_state();
        break;
      case NetworkContextType::kProfile:
        pref_service = browser()->profile()->GetPrefs();
        break;
      case NetworkContextType::kIncognitoProfile:
        // Incognito uses the non-incognito prefs.
        pref_service =
            browser()->profile()->GetOffTheRecordProfile()->GetPrefs();
        break;
    }

    pref_service->Set(proxy_config::prefs::kProxy,
                      *ProxyConfigDictionary::CreateFixedServers(
                          host_port_pair.ToString(), std::string()));

    // Wait for the new ProxyConfig to be passed over the pipe. Needed because
    // Mojo doesn't guarantee ordering of events on different Mojo pipes, and
    // requests are sent on a separate pipe from ProxyConfigs.
    switch (GetParam().network_context_type) {
      case NetworkContextType::kSystem:
      case NetworkContextType::kSafeBrowsing:
        g_browser_process->system_network_context_manager()
            ->FlushProxyConfigMonitorForTesting();
        break;
      case NetworkContextType::kProfile:
        ProfileNetworkContextServiceFactory::GetForContext(browser()->profile())
            ->FlushProxyConfigMonitorForTesting();
        break;
      case NetworkContextType::kIncognitoProfile:
        ProfileNetworkContextServiceFactory::GetForContext(
            browser()->profile()->GetOffTheRecordProfile())
            ->FlushProxyConfigMonitorForTesting();
        break;
    }
  }

  // Sends a request and expects it to be handled by embedded_test_server()
  // acting as a proxy;
  void TestProxyConfigured(bool expect_success) {
    std::unique_ptr<network::ResourceRequest> request =
        std::make_unique<network::ResourceRequest>();
    // This URL should be directed to the test server because of the proxy.
    request->url = GURL("http://does.not.resolve.test:1872/echo");

    content::SimpleURLLoaderTestHelper simple_loader_helper;
    std::unique_ptr<network::SimpleURLLoader> simple_loader =
        network::SimpleURLLoader::Create(std::move(request),
                                         TRAFFIC_ANNOTATION_FOR_TESTS);

    simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        loader_factory(), simple_loader_helper.GetCallback());
    simple_loader_helper.WaitForCallback();

    if (expect_success) {
      EXPECT_EQ(net::OK, simple_loader->NetError());
      ASSERT_TRUE(simple_loader_helper.response_body());
      EXPECT_EQ(*simple_loader_helper.response_body(), "Echo");
    } else {
      // TestHostResolver returns net::ERR_NOT_IMPLEMENTED for non-local host
      // URLs.
      EXPECT_EQ(net::ERR_NOT_IMPLEMENTED, simple_loader->NetError());
      ASSERT_FALSE(simple_loader_helper.response_body());
    }
  }

  // Makes a request that hangs, and will live until browser shutdown.
  void MakeLongLivedRequestThatHangsUntilShutdown() {
    std::unique_ptr<network::ResourceRequest> request =
        std::make_unique<network::ResourceRequest>();
    request->url = embedded_test_server()->GetURL(kControllablePath);
    live_during_shutdown_simple_loader_ = network::SimpleURLLoader::Create(
        std::move(request), TRAFFIC_ANNOTATION_FOR_TESTS);
    live_during_shutdown_simple_loader_helper_ =
        std::make_unique<content::SimpleURLLoaderTestHelper>();

    live_during_shutdown_simple_loader_
        ->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
            loader_factory(),
            live_during_shutdown_simple_loader_helper_->GetCallback());

    // Don't actually care about controlling the response, just need to wait
    // until it sees the request, to make sure that a URLRequest has been
    // created to potentially leak. Since the |controllable_http_response_| is
    // not used to send a response to the request, the request just hangs until
    // the NetworkContext is destroyed (Or the test server is shut down, but the
    // NetworkContext should be destroyed before that happens, in this test).
    controllable_http_response_->WaitForRequest();
  }

  bool FetchHeaderEcho(const std::string& header_name,
                       std::string* header_value_out) {
    std::unique_ptr<network::ResourceRequest> request =
        std::make_unique<network::ResourceRequest>();
    request->url = embedded_test_server()->GetURL(
        base::StrCat({"/echoheader?", header_name}));
    content::SimpleURLLoaderTestHelper simple_loader_helper;
    std::unique_ptr<network::SimpleURLLoader> simple_loader =
        network::SimpleURLLoader::Create(std::move(request),
                                         TRAFFIC_ANNOTATION_FOR_TESTS);
    simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        loader_factory(), simple_loader_helper.GetCallback());
    simple_loader_helper.WaitForCallback();
    if (simple_loader_helper.response_body()) {
      *header_value_out = *simple_loader_helper.response_body();
      return true;
    }
    return false;
  }

  void FlushNetworkInterface() {
    switch (GetParam().network_context_type) {
      case NetworkContextType::kSystem:
        g_browser_process->system_network_context_manager()
            ->FlushNetworkInterfaceForTesting();
        break;
      case NetworkContextType::kSafeBrowsing:
        g_browser_process->safe_browsing_service()
            ->FlushNetworkInterfaceForTesting();
        break;
      case NetworkContextType::kProfile:
        content::BrowserContext::GetDefaultStoragePartition(
            browser()->profile())
            ->FlushNetworkInterfaceForTesting();
        break;
      case NetworkContextType::kIncognitoProfile:
        DCHECK(incognito_);
        content::BrowserContext::GetDefaultStoragePartition(
            incognito_->profile())
            ->FlushNetworkInterfaceForTesting();
        break;
    }
  }

 private:
  void SimulateNetworkServiceCrashIfNecessary() {
    if (GetParam().network_service_state != NetworkServiceState::kRestarted)
      return;

    // Make sure |network_context()| is working as expected. Use '/echoheader'
    // instead of '/echo' to avoid a disk_cache bug.
    // See https://crbug.com/792255.
    int net_error = content::LoadBasicRequest(
        network_context(), embedded_test_server()->GetURL("/echoheader"));
    // The error code could be |net::ERR_PROXY_CONNECTION_FAILED| if the test is
    // using 'bad_server.pac'.
    EXPECT_TRUE(net_error == net::OK ||
                net_error == net::ERR_PROXY_CONNECTION_FAILED);

    // Crash the NetworkService process. Existing interfaces should receive
    // error notifications at some point.
    content::SimulateNetworkServiceCrash();
    // Flush the interface to make sure the error notification was received.
    FlushNetworkInterface();
  }

  Browser* incognito_ = nullptr;
  base::test::ScopedFeatureList feature_list_;

  std::unique_ptr<net::test_server::ControllableHttpResponse>
      controllable_http_response_;

  // Used in tests that need a live request during browser shutdown.
  std::unique_ptr<network::SimpleURLLoader> live_during_shutdown_simple_loader_;
  std::unique_ptr<content::SimpleURLLoaderTestHelper>
      live_during_shutdown_simple_loader_helper_;

  DISALLOW_COPY_AND_ASSIGN(NetworkContextConfigurationBrowserTest);
};

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, BasicRequest) {
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = embedded_test_server()->GetURL("/echo");
  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);

  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();

  ASSERT_TRUE(simple_loader->ResponseInfo());
  ASSERT_TRUE(simple_loader->ResponseInfo()->headers);
  EXPECT_EQ(200, simple_loader->ResponseInfo()->headers->response_code());
  ASSERT_TRUE(simple_loader_helper.response_body());
  EXPECT_EQ("Echo", *simple_loader_helper.response_body());
}

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, DataURL) {
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = GURL("data:text/plain,foo");
  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);

  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();

  ASSERT_TRUE(simple_loader->ResponseInfo());
  // Data URLs don't have headers.
  EXPECT_FALSE(simple_loader->ResponseInfo()->headers);
  EXPECT_EQ("text/plain", simple_loader->ResponseInfo()->mime_type);
  ASSERT_TRUE(simple_loader_helper.response_body());
  EXPECT_EQ("foo", *simple_loader_helper.response_body());
}

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, FileURL) {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::ScopedTempDir temp_dir_;
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  base::FilePath file_path;
  ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir_.GetPath(), &file_path));
  const char kFileContents[] = "This file intentionally left empty.";
  ASSERT_EQ(static_cast<int>(strlen(kFileContents)),
            base::WriteFile(file_path, kFileContents, strlen(kFileContents)));

  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = net::FilePathToFileURL(file_path);
  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);

  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();

  ASSERT_TRUE(simple_loader->ResponseInfo());
  // File URLs don't have headers.
  EXPECT_FALSE(simple_loader->ResponseInfo()->headers);
  ASSERT_TRUE(simple_loader_helper.response_body());
  EXPECT_EQ(kFileContents, *simple_loader_helper.response_body());
}

// Make sure a cache is used when expected.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, Cache) {
  // Make a request whose response should be cached.
  GURL request_url = embedded_test_server()->GetURL("/cachetime");
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = request_url;
  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);

  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();

  ASSERT_TRUE(simple_loader_helper.response_body());
  EXPECT_GT(simple_loader_helper.response_body()->size(), 0u);

  // Stop the server.
  ASSERT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());

  // Make the request again, and make sure it's cached or not, according to
  // expectations. Reuse the content::ResourceRequest, but nothing else.
  std::unique_ptr<network::ResourceRequest> request2 =
      std::make_unique<network::ResourceRequest>();
  request2->url = request_url;
  content::SimpleURLLoaderTestHelper simple_loader_helper2;
  std::unique_ptr<network::SimpleURLLoader> simple_loader2 =
      network::SimpleURLLoader::Create(std::move(request2),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);
  simple_loader2->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper2.GetCallback());
  simple_loader_helper2.WaitForCallback();
  if (GetHttpCacheType() == StorageType::kNone) {
    // If there's no cache, and not server running, the request should have
    // failed.
    EXPECT_FALSE(simple_loader_helper2.response_body());
    EXPECT_EQ(net::ERR_CONNECTION_REFUSED, simple_loader2->NetError());
  } else {
    // Otherwise, the request should have succeeded, and returned the same
    // result as before.
    ASSERT_TRUE(simple_loader_helper2.response_body());
    EXPECT_EQ(*simple_loader_helper.response_body(),
              *simple_loader_helper2.response_body());
  }
}

// Make sure an on-disk cache is used when expected. PRE_DiskCache populates the
// cache. DiskCache then makes sure the cache entry is still there (Or not) as
// expected.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, PRE_DiskCache) {
  // Save test URL to disk, so it can be used in the next test (Test server uses
  // a random port, so need to know the port to try and retrieve it from the
  // cache in the next test). The profile directory is preserved between the
  // PRE_DiskCache and DiskCache run, so can just keep a file there.
  GURL test_url = embedded_test_server()->GetURL(kCacheRandomPath);
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath save_url_file_path = browser()->profile()->GetPath().Append(
      FILE_PATH_LITERAL("url_for_test.txt"));

  // Make a request whose response should be cached.
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = test_url;

  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);
  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();

  EXPECT_EQ(net::OK, simple_loader->NetError());
  ASSERT_TRUE(simple_loader_helper.response_body());
  EXPECT_FALSE(simple_loader_helper.response_body()->empty());

  // Write the URL and expected response to a file.
  std::string file_data =
      test_url.spec() + "\n" + *simple_loader_helper.response_body();
  ASSERT_EQ(
      static_cast<int>(file_data.length()),
      base::WriteFile(save_url_file_path, file_data.data(), file_data.size()));

  EXPECT_TRUE(embedded_test_server()->ShutdownAndWaitUntilComplete());
}

// Check if the URL loaded in PRE_DiskCache is still in the cache, across a
// browser restart.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, DiskCache) {
  // Load URL from the above test body to disk.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath save_url_file_path = browser()->profile()->GetPath().Append(
      FILE_PATH_LITERAL("url_for_test.txt"));
  std::string file_data;
  ASSERT_TRUE(ReadFileToString(save_url_file_path, &file_data));

  size_t newline_pos = file_data.find('\n');
  ASSERT_NE(newline_pos, std::string::npos);

  GURL test_url = GURL(file_data.substr(0, newline_pos));
  ASSERT_TRUE(test_url.is_valid()) << test_url.possibly_invalid_spec();

  std::string original_response = file_data.substr(newline_pos + 1);

  // Request the same test URL as may have been cached by PRE_DiskCache.
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = test_url;
  content::SimpleURLLoaderTestHelper simple_loader_helper;
  request->load_flags = net::LOAD_ONLY_FROM_CACHE;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);
  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();

  // The request should only succeed if there is an on-disk cache.
  if (GetHttpCacheType() != StorageType::kDisk) {
    EXPECT_FALSE(simple_loader_helper.response_body());
  } else if (GetParam().network_service_state !=
             NetworkServiceState::kRestarted) {
    ASSERT_TRUE(simple_loader_helper.response_body());
    EXPECT_EQ(original_response, *simple_loader_helper.response_body());
  } else {
    // The network service restarted, and may or may not have recovered the
    // cache entry. If the request succeded, though, it must return the correct
    // result.
    // TODO(mmenke): Is there any way to ensure the item in the cache can be
    // recovered / the cache can be loaded in this test?
    if (simple_loader_helper.response_body())
      EXPECT_EQ(original_response, *simple_loader_helper.response_body());
  }
}

// Test certs cannot currently be installed on OSX with the network service
// enabled.
// TODO(mmenke): Once that is fixed, enable this test on OSX.
// See https://crbug.com/757088
#if !defined(OS_MACOSX)

// Visits a URL with an HSTS header, and makes sure it is respected.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, PRE_Hsts) {
  net::test_server::EmbeddedTestServer ssl_server(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS);
  ssl_server.SetSSLConfig(
      net::test_server::EmbeddedTestServer::CERT_COMMON_NAME_IS_DOMAIN);
  ssl_server.AddDefaultHandlers(
      base::FilePath(FILE_PATH_LITERAL("chrome/test/data")));
  ASSERT_TRUE(ssl_server.Start());

  // Make a request whose response has an STS header.
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = ssl_server.GetURL(
      "/set-header?Strict-Transport-Security: max-age%3D600000");

  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);
  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();

  EXPECT_EQ(net::OK, simple_loader->NetError());
  ASSERT_TRUE(simple_loader->ResponseInfo()->headers);
  EXPECT_TRUE(simple_loader->ResponseInfo()->headers->HasHeaderValue(
      "Strict-Transport-Security", "max-age=600000"));

  // Make a cache-only request to make sure the HSTS entry is respected. Using a
  // cache-only request both prevents the result from being cached, and makes
  // the check identical to the one in the next test, which is run after the
  // test server has been shut down.
  GURL exected_ssl_url = ssl_server.base_url();
  GURL::Replacements replacements;
  replacements.SetSchemeStr("http");
  GURL start_url = exected_ssl_url.ReplaceComponents(replacements);

  request = std::make_unique<network::ResourceRequest>();
  request->url = start_url;
  request->load_flags = net::LOAD_ONLY_FROM_CACHE;

  content::SimpleURLLoaderTestHelper simple_loader_helper2;
  simple_loader = network::SimpleURLLoader::Create(
      std::move(request), TRAFFIC_ANNOTATION_FOR_TESTS);
  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper2.GetCallback());
  simple_loader_helper2.WaitForCallback();
  EXPECT_FALSE(simple_loader_helper2.response_body());
  EXPECT_EQ(exected_ssl_url, simple_loader->GetFinalURL());

  // Write the URL with HSTS information to a file, so it can be loaded in the
  // next test. Have to use a file for this, since the server's port is random.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath save_url_file_path = browser()->profile()->GetPath().Append(
      FILE_PATH_LITERAL("url_for_test.txt"));
  std::string file_data = start_url.spec();
  ASSERT_EQ(
      static_cast<int>(file_data.length()),
      base::WriteFile(save_url_file_path, file_data.data(), file_data.size()));
}

// Checks if the HSTS information from the last test is still available after a
// restart.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, Hsts) {
  // The network service must be cleanly shut down to guarantee HSTS information
  // is flushed to disk, but that currently generally doesn't happen. See
  // https://crbug.com/820996.
  if (GetParam().network_service_state != NetworkServiceState::kDisabled &&
      GetHttpCacheType() == StorageType::kDisk) {
    return;
  }
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath save_url_file_path = browser()->profile()->GetPath().Append(
      FILE_PATH_LITERAL("url_for_test.txt"));
  std::string file_data;
  ASSERT_TRUE(ReadFileToString(save_url_file_path, &file_data));
  GURL start_url = GURL(file_data);

  // Unfortunately, loading HSTS information is loaded asynchronously from
  // disk, so there's no way to guarantee it has loaded by the time a
  // request is made. As a result, may have to try multiple times to verify that
  // cached HSTS information has been loaded, when it's stored on disk.
  while (true) {
    std::unique_ptr<network::ResourceRequest> request =
        std::make_unique<network::ResourceRequest>();
    request->url = start_url;
    request->load_flags = net::LOAD_ONLY_FROM_CACHE;

    content::SimpleURLLoaderTestHelper simple_loader_helper;
    std::unique_ptr<network::SimpleURLLoader> simple_loader =
        network::SimpleURLLoader::Create(std::move(request),
                                         TRAFFIC_ANNOTATION_FOR_TESTS);
    simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        loader_factory(), simple_loader_helper.GetCallback());
    simple_loader_helper.WaitForCallback();
    EXPECT_FALSE(simple_loader_helper.response_body());

    // HSTS information is currently stored on disk if-and-only-if there's an
    // on-disk HTTP cache.
    if (GetHttpCacheType() == StorageType::kDisk) {
      GURL::Replacements replacements;
      replacements.SetSchemeStr("https");
      GURL expected_https_url = start_url.ReplaceComponents(replacements);
      // The file may not have been loaded yet, so if the cached HSTS
      // information was not respected, try again.
      if (expected_https_url != simple_loader->GetFinalURL())
        continue;
      EXPECT_EQ(expected_https_url, simple_loader->GetFinalURL());
      break;
    } else {
      EXPECT_EQ(start_url, simple_loader->GetFinalURL());
      break;
    }
  }
}

#endif  // !defined(OS_MACOSX)

// Check that the SSLConfig is hooked up. PRE_SSLConfig checks that changing
// local_state() after start modifies the SSLConfig, SSLConfig makes sure the
// (now modified) initial value of local_state() is respected.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, PRE_SSLConfig) {
  // Start a TLS 1.0 server.
  net::EmbeddedTestServer ssl_server(net::EmbeddedTestServer::TYPE_HTTPS);
  net::SSLServerConfig ssl_config;
  ssl_config.version_min = net::SSL_PROTOCOL_VERSION_TLS1;
  ssl_config.version_max = net::SSL_PROTOCOL_VERSION_TLS1;
  ssl_server.SetSSLConfig(net::EmbeddedTestServer::CERT_OK, ssl_config);
  ssl_server.AddDefaultHandlers(
      base::FilePath(FILE_PATH_LITERAL("chrome/test/data")));
  ASSERT_TRUE(ssl_server.Start());

  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = ssl_server.GetURL("/echo");
  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);
  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();
#if defined(OS_MACOSX)
  // TODO(https://crbug.com/757088):  Test certs don't work on OSX, with the
  // network service.
  if (GetParam().network_service_state != NetworkServiceState::kDisabled) {
    EXPECT_FALSE(simple_loader_helper.response_body());
  } else {
    ASSERT_TRUE(simple_loader_helper.response_body());
    EXPECT_EQ(*simple_loader_helper.response_body(), "Echo");
  }
#else
  ASSERT_TRUE(simple_loader_helper.response_body());
  EXPECT_EQ(*simple_loader_helper.response_body(), "Echo");
#endif

  // Disallow TLS 1.0 via prefs.
  g_browser_process->local_state()->SetString(prefs::kSSLVersionMin,
                                              switches::kSSLVersionTLSv11);
  // Flush the changes to the network process, to avoid a race between updating
  // the config and the next request.
  g_browser_process->system_network_context_manager()
      ->FlushSSLConfigManagerForTesting();

  // With the new prefs, requests to the server should be blocked.
  request = std::make_unique<network::ResourceRequest>();
  request->url = ssl_server.GetURL("/echo");
  content::SimpleURLLoaderTestHelper simple_loader_helper2;
  simple_loader = network::SimpleURLLoader::Create(
      std::move(request), TRAFFIC_ANNOTATION_FOR_TESTS);
  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper2.GetCallback());
  simple_loader_helper2.WaitForCallback();
  EXPECT_FALSE(simple_loader_helper2.response_body());
  EXPECT_EQ(net::ERR_SSL_VERSION_OR_CIPHER_MISMATCH, simple_loader->NetError());
}

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, SSLConfig) {
  // Start a TLS 1.0 server.
  net::EmbeddedTestServer ssl_server(net::EmbeddedTestServer::TYPE_HTTPS);
  net::SSLServerConfig ssl_config;
  ssl_config.version_min = net::SSL_PROTOCOL_VERSION_TLS1;
  ssl_config.version_max = net::SSL_PROTOCOL_VERSION_TLS1;
  ssl_server.SetSSLConfig(net::EmbeddedTestServer::CERT_OK, ssl_config);
  ASSERT_TRUE(ssl_server.Start());

  // Making a request should fail, since PRE_SSLConfig saved a pref to disallow
  // TLS 1.0.
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  request->url = ssl_server.GetURL("/echo");
  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);
  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();
  EXPECT_FALSE(simple_loader_helper.response_body());
  EXPECT_EQ(net::ERR_SSL_VERSION_OR_CIPHER_MISMATCH, simple_loader->NetError());
}

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest, ProxyConfig) {
  SetProxyPref(embedded_test_server()->host_port_pair());
  TestProxyConfigured(/*expect_success=*/true);
}

// This test should not end in an AssertNoURLLRequests CHECK.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest,
                       ShutdownWithLiveRequest) {
  MakeLongLivedRequestThatHangsUntilShutdown();
}

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationBrowserTest,
                       UserAgentAndLanguagePrefs) {
  // The system and SafeBrowsing network contexts aren't associated with any
  // profile, so changing the language settings for the profile's main network
  // context won't affect what they send.
  bool system =
      (GetParam().network_context_type == NetworkContextType::kSystem ||
       GetParam().network_context_type == NetworkContextType::kSafeBrowsing);
  // echoheader returns "None" when the header isn't there in the first place.
  const char kNoAcceptLanguage[] = "None";

  std::string accept_language, user_agent;
  // Check default.
  ASSERT_TRUE(FetchHeaderEcho("accept-language", &accept_language));
  EXPECT_EQ(system ? kNoAcceptLanguage : "en-US,en;q=0.9", accept_language);
  ASSERT_TRUE(FetchHeaderEcho("user-agent", &user_agent));
  EXPECT_EQ(::GetUserAgent(), user_agent);

  // Now change the profile a different language, and see if the headers
  // get updated.
  browser()->profile()->GetPrefs()->SetString(prefs::kAcceptLanguages, "uk");
  FlushNetworkInterface();
  std::string accept_language2, user_agent2;
  ASSERT_TRUE(FetchHeaderEcho("accept-language", &accept_language2));
  EXPECT_EQ(system ? kNoAcceptLanguage : "uk", accept_language2);
  ASSERT_TRUE(FetchHeaderEcho("user-agent", &user_agent2));
  EXPECT_EQ(::GetUserAgent(), user_agent2);

  // Try a more complicated one, with multiple languages.
  browser()->profile()->GetPrefs()->SetString(prefs::kAcceptLanguages,
                                              "uk, en_US");
  FlushNetworkInterface();
  std::string accept_language3, user_agent3;
  ASSERT_TRUE(FetchHeaderEcho("accept-language", &accept_language3));
  EXPECT_EQ(system ? kNoAcceptLanguage : "uk,en_US;q=0.9", accept_language3);
  ASSERT_TRUE(FetchHeaderEcho("user-agent", &user_agent3));
  EXPECT_EQ(::GetUserAgent(), user_agent3);
}

class NetworkContextConfigurationFixedPortBrowserTest
    : public NetworkContextConfigurationBrowserTest {
 public:
  NetworkContextConfigurationFixedPortBrowserTest() {}
  ~NetworkContextConfigurationFixedPortBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kTestingFixedHttpPort,
        base::StringPrintf("%u", embedded_test_server()->port()));
    LOG(WARNING) << base::StringPrintf("%u", embedded_test_server()->port());
  }
};

// Test that the command line switch makes it to the network service and is
// respected.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationFixedPortBrowserTest,
                       TestingFixedPort) {
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  // This URL does not use the port the embedded test server is using. The
  // command line switch should make it result in the request being directed to
  // the test server anyways.
  request->url = GURL("http://127.0.0.1/echo");
  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);

  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();

  EXPECT_EQ(net::OK, simple_loader->NetError());
  ASSERT_TRUE(simple_loader_helper.response_body());
  EXPECT_EQ(*simple_loader_helper.response_body(), "Echo");
}

class NetworkContextConfigurationProxyOnStartBrowserTest
    : public NetworkContextConfigurationBrowserTest {
 public:
  NetworkContextConfigurationProxyOnStartBrowserTest() {}
  ~NetworkContextConfigurationProxyOnStartBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kProxyServer,
        embedded_test_server()->host_port_pair().ToString());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkContextConfigurationProxyOnStartBrowserTest);
};

// Test that when there's a proxy configuration at startup, the initial requests
// use that configuration.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationProxyOnStartBrowserTest,
                       TestInitialProxyConfig) {
  TestProxyConfigured(/*expect_success=*/true);
}

// Make sure the system URLRequestContext can handle fetching PAC scripts from
// http URLs.
class NetworkContextConfigurationHttpPacBrowserTest
    : public NetworkContextConfigurationBrowserTest {
 public:
  NetworkContextConfigurationHttpPacBrowserTest()
      : pac_test_server_(net::test_server::EmbeddedTestServer::TYPE_HTTP) {}

  ~NetworkContextConfigurationHttpPacBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    pac_test_server_.RegisterRequestHandler(base::Bind(
        &NetworkContextConfigurationHttpPacBrowserTest::HandlePacRequest,
        GetPacScript()));
    EXPECT_TRUE(pac_test_server_.Start());

    command_line->AppendSwitchASCII(switches::kProxyPacUrl,
                                    pac_test_server_.base_url().spec().c_str());
  }

  static std::unique_ptr<net::test_server::HttpResponse> HandlePacRequest(
      const std::string& pac_script,
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    response->set_content(pac_script);
    return response;
  }

 private:
  net::test_server::EmbeddedTestServer pac_test_server_;

  DISALLOW_COPY_AND_ASSIGN(NetworkContextConfigurationHttpPacBrowserTest);
};

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationHttpPacBrowserTest, HttpPac) {
  TestProxyConfigured(/*expect_success=*/true);
}

// Make sure the system URLRequestContext can handle fetching PAC scripts from
// file URLs.
class NetworkContextConfigurationFilePacBrowserTest
    : public NetworkContextConfigurationBrowserTest {
 public:
  NetworkContextConfigurationFilePacBrowserTest() {}

  ~NetworkContextConfigurationFilePacBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    const char kPacFileName[] = "foo.pac";

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath pac_file_path =
        temp_dir_.GetPath().AppendASCII(kPacFileName);

    std::string pac_script = GetPacScript();
    ASSERT_EQ(
        static_cast<int>(pac_script.size()),
        base::WriteFile(pac_file_path, pac_script.c_str(), pac_script.size()));

    command_line->AppendSwitchASCII(
        switches::kProxyPacUrl, net::FilePathToFileURL(pac_file_path).spec());
  }

 private:
  base::ScopedTempDir temp_dir_;

  DISALLOW_COPY_AND_ASSIGN(NetworkContextConfigurationFilePacBrowserTest);
};

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationFilePacBrowserTest, FilePac) {
  bool network_service_disabled =
      !base::FeatureList::IsEnabled(network::features::kNetworkService);
#if defined(OS_MACOSX)
  // https://crbug.com/843324: the NetworkServiceTestHelper does not work on Mac
  // so the TestHostResolver is not used to resolve the host name when the
  // network service is enabled. The system host resolver is used instead
  // and goed to the network, which we don't want in tests. (and it does not
  // return the expected net::ERR_NOT_IMPLEMENTED).
  if (!network_service_disabled)
    return;
#endif
  // PAC file URLs are not supported with the network service
  TestProxyConfigured(/*expect_success=*/network_service_disabled);
}

// Make sure the system URLRequestContext can handle fetching PAC scripts from
// data URLs.
class NetworkContextConfigurationDataPacBrowserTest
    : public NetworkContextConfigurationBrowserTest {
 public:
  NetworkContextConfigurationDataPacBrowserTest() {}
  ~NetworkContextConfigurationDataPacBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    std::string contents;
    // Read in kPACScript contents.
    command_line->AppendSwitchASCII(switches::kProxyPacUrl,
                                    "data:," + GetPacScript());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkContextConfigurationDataPacBrowserTest);
};

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationDataPacBrowserTest, DataPac) {
  TestProxyConfigured(/*expect_success=*/true);
}

// Make sure the system URLRequestContext can handle fetching PAC scripts from
// ftp URLs. Unlike the other PAC tests, this test uses a PAC script that
// results in an error, since the spawned test server is designed so that it can
// run remotely (So can't just write a script to a local file and have the
// server serve it).
class NetworkContextConfigurationFtpPacBrowserTest
    : public NetworkContextConfigurationBrowserTest {
 public:
  NetworkContextConfigurationFtpPacBrowserTest()
      : ftp_server_(net::SpawnedTestServer::TYPE_FTP,
                    base::FilePath(FILE_PATH_LITERAL("chrome/test/data"))) {
    EXPECT_TRUE(ftp_server_.Start());
  }
  ~NetworkContextConfigurationFtpPacBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kProxyPacUrl,
        ftp_server_.GetURL("bad_server.pac").spec().c_str());
  }

 private:
  net::SpawnedTestServer ftp_server_;

  DISALLOW_COPY_AND_ASSIGN(NetworkContextConfigurationFtpPacBrowserTest);
};

IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationFtpPacBrowserTest, FtpPac) {
  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  // This URL should be directed to the test server because of the proxy.
  request->url = GURL("http://does.not.resolve.test:1872/echo");

  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);

  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();

  EXPECT_EQ(net::ERR_PROXY_CONNECTION_FAILED, simple_loader->NetError());
}

// Used to test that PAC HTTPS URL stripping works. A different test server is
// used as the "proxy" based on whether the PAC script sees the full path or
// not. The servers aren't correctly set up to mimic HTTP proxies that tunnel
// to an HTTPS test server, so the test fixture just watches for any incoming
// connection.
class NetworkContextConfigurationHttpsStrippingPacBrowserTest
    : public NetworkContextConfigurationBrowserTest {
 public:
  NetworkContextConfigurationHttpsStrippingPacBrowserTest() {}

  ~NetworkContextConfigurationHttpsStrippingPacBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Test server HostPortPair, as a string.
    std::string test_server_host_port_pair =
        net::HostPortPair::FromURL(embedded_test_server()->base_url())
            .ToString();
    // Set up a PAC file that directs to different servers based on the URL it
    // sees.
    std::string pac_script = base::StringPrintf(
        "function FindProxyForURL(url, host) {"
        // With the test URL stripped of the path, try to use the embedded test
        // server to establish a an SSL tunnel over an HTTP proxy. The request
        // will fail with ERR_TUNNEL_CONNECTION_FAILED.
        "  if (url == 'https://does.not.resolve.test:1872/')"
        "    return 'PROXY %s';"
        // With the full test URL, try to use a domain that does not resolve as
        // a proxy. Errors connecting to the proxy result in
        // ERR_PROXY_CONNECTION_FAILED.
        "  if (url == 'https://does.not.resolve.test:1872/foo')"
        "    return 'PROXY does.not.resolve.test';"
        // Otherwise, use direct. If a connection to "does.not.resolve.test"
        // tries to use a direction connection, it will fail with
        // ERR_NAME_NOT_RESOLVED. This path will also
        // be used by the initial request in NetworkServiceState::kRestarted
        // tests to make sure the network service process is fully started
        // before it's crashed and restarted. Using direct in this case avoids
        // that request failing with an unexpeced error when being directed to a
        // bogus proxy.
        "  return 'DIRECT';"
        "}",
        test_server_host_port_pair.c_str());

    command_line->AppendSwitchASCII(switches::kProxyPacUrl,
                                    "data:," + pac_script);
  }
};

// Start Chrome and check that PAC HTTPS path stripping is enabled.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationHttpsStrippingPacBrowserTest,
                       PRE_PacHttpsUrlStripping) {
  ASSERT_FALSE(CreateDefaultNetworkContextParams()
                   ->dangerously_allow_pac_access_to_secure_urls);

  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  // This URL should be directed to the proxy that fails with
  // ERR_TUNNEL_CONNECTION_FAILED.
  request->url = GURL("https://does.not.resolve.test:1872/foo");

  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);

  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();
  EXPECT_FALSE(simple_loader_helper.response_body());
  EXPECT_EQ(net::ERR_TUNNEL_CONNECTION_FAILED, simple_loader->NetError());

  // Disable stripping paths from HTTPS PAC URLs for the next test.
  g_browser_process->local_state()->SetBoolean(
      prefs::kPacHttpsUrlStrippingEnabled, false);
  // Check that the changed setting is reflected in the network context params.
  // The changes aren't applied to existing URLRequestContexts, however, so have
  // to restart to see the setting change respected.
  EXPECT_TRUE(CreateDefaultNetworkContextParams()
                  ->dangerously_allow_pac_access_to_secure_urls);
}

// Restart Chrome and check the case where PAC HTTPS path stripping is disabled.
// Have to restart Chrome because the setting is only checked on NetworkContext
// creation.
IN_PROC_BROWSER_TEST_P(NetworkContextConfigurationHttpsStrippingPacBrowserTest,
                       PacHttpsUrlStripping) {
  ASSERT_TRUE(CreateDefaultNetworkContextParams()
                  ->dangerously_allow_pac_access_to_secure_urls);

  std::unique_ptr<network::ResourceRequest> request =
      std::make_unique<network::ResourceRequest>();
  // This URL should be directed to the proxy that fails with
  // ERR_PROXY_CONNECTION_FAILED.
  request->url = GURL("https://does.not.resolve.test:1872/foo");

  content::SimpleURLLoaderTestHelper simple_loader_helper;
  std::unique_ptr<network::SimpleURLLoader> simple_loader =
      network::SimpleURLLoader::Create(std::move(request),
                                       TRAFFIC_ANNOTATION_FOR_TESTS);

  simple_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      loader_factory(), simple_loader_helper.GetCallback());
  simple_loader_helper.WaitForCallback();
  EXPECT_FALSE(simple_loader_helper.response_body());
  EXPECT_EQ(net::ERR_PROXY_CONNECTION_FAILED, simple_loader->NetError());
}

// |NetworkServiceTestHelper| doesn't work on browser_tests on OSX.
// See https://crbug.com/757088
#if defined(OS_MACOSX)
// Instantiates tests with a prefix indicating which NetworkContext is being
// tested, and a suffix of "/0" if the network service is disabled and "/1" if
// it's enabled.
#define INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(TestFixture)               \
  INSTANTIATE_TEST_CASE_P(                                                 \
      SystemNetworkContext, TestFixture,                                   \
      ::testing::Values(TestCase({NetworkServiceState::kDisabled,          \
                                  NetworkContextType::kSystem}),           \
                        TestCase({NetworkServiceState::kEnabled,           \
                                  NetworkContextType::kSystem})));         \
                                                                           \
  INSTANTIATE_TEST_CASE_P(                                                 \
      SafeBrowsingNetworkContext, TestFixture,                             \
      ::testing::Values(TestCase({NetworkServiceState::kDisabled,          \
                                  NetworkContextType::kSafeBrowsing}),     \
                        TestCase({NetworkServiceState::kEnabled,           \
                                  NetworkContextType::kSystem})));         \
                                                                           \
  INSTANTIATE_TEST_CASE_P(                                                 \
      ProfileMainNetworkContext, TestFixture,                              \
      ::testing::Values(TestCase({NetworkServiceState::kDisabled,          \
                                  NetworkContextType::kProfile}),          \
                        TestCase({NetworkServiceState::kEnabled,           \
                                  NetworkContextType::kProfile})));        \
                                                                           \
  INSTANTIATE_TEST_CASE_P(                                                 \
      IncognitoProfileMainNetworkContext, TestFixture,                     \
      ::testing::Values(TestCase({NetworkServiceState::kDisabled,          \
                                  NetworkContextType::kIncognitoProfile}), \
                        TestCase({NetworkServiceState::kEnabled,           \
                                  NetworkContextType::kIncognitoProfile})))

#else  // !defined(OS_MACOSX)
// Instiates tests with a prefix indicating which NetworkContext is being
// tested, and a suffix of "/0" if the network service is disabled, "/1" if it's
// enabled, and "/2" if it's enabled and restarted.
//
// TODO(mmenke): Enabled tests for the SafeBrowsing NetworkContext, once it
// works with the network service enabled.
#define INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(TestFixture)               \
  INSTANTIATE_TEST_CASE_P(                                                 \
      SystemNetworkContext, TestFixture,                                   \
      ::testing::Values(TestCase({NetworkServiceState::kDisabled,          \
                                  NetworkContextType::kSystem}),           \
                        TestCase({NetworkServiceState::kEnabled,           \
                                  NetworkContextType::kSystem}),           \
                        TestCase({NetworkServiceState::kRestarted,         \
                                  NetworkContextType::kSystem})));         \
                                                                           \
  INSTANTIATE_TEST_CASE_P(                                                 \
      SafeBrowsingNetworkContext, TestFixture,                             \
      ::testing::Values(TestCase({NetworkServiceState::kDisabled,          \
                                  NetworkContextType::kSafeBrowsing}),     \
                        TestCase({NetworkServiceState::kEnabled,           \
                                  NetworkContextType::kSafeBrowsing}),     \
                        TestCase({NetworkServiceState::kRestarted,         \
                                  NetworkContextType::kSafeBrowsing})));   \
                                                                           \
  INSTANTIATE_TEST_CASE_P(                                                 \
      ProfileMainNetworkContext, TestFixture,                              \
      ::testing::Values(TestCase({NetworkServiceState::kDisabled,          \
                                  NetworkContextType::kProfile}),          \
                        TestCase({NetworkServiceState::kEnabled,           \
                                  NetworkContextType::kProfile}),          \
                        TestCase({NetworkServiceState::kRestarted,         \
                                  NetworkContextType::kProfile})));        \
                                                                           \
  INSTANTIATE_TEST_CASE_P(                                                 \
      IncognitoProfileMainNetworkContext, TestFixture,                     \
      ::testing::Values(TestCase({NetworkServiceState::kDisabled,          \
                                  NetworkContextType::kIncognitoProfile}), \
                        TestCase({NetworkServiceState::kEnabled,           \
                                  NetworkContextType::kIncognitoProfile}), \
                        TestCase({NetworkServiceState::kRestarted,         \
                                  NetworkContextType::kIncognitoProfile})))
#endif  // !defined(OS_MACOSX)

INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(NetworkContextConfigurationBrowserTest);
INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(
    NetworkContextConfigurationFixedPortBrowserTest);
INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(
    NetworkContextConfigurationProxyOnStartBrowserTest);
INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(
    NetworkContextConfigurationHttpPacBrowserTest);
INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(
    NetworkContextConfigurationFilePacBrowserTest);
INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(
    NetworkContextConfigurationDataPacBrowserTest);
INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(
    NetworkContextConfigurationFtpPacBrowserTest);
INSTANTIATE_TEST_CASES_FOR_TEST_FIXTURE(
    NetworkContextConfigurationHttpsStrippingPacBrowserTest);

}  // namespace
