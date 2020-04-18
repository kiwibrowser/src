// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_new_script_loader.h"

#include <map>
#include <utility>
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_disk_cache.h"
#include "content/browser/url_loader_factory_getter.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "net/base/test_completion_callback.h"
#include "net/http/http_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/test/test_url_loader_client.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"

namespace content {

namespace {

const char kNormalScriptURL[] = "https://example.com/normal.js";

// MockHTTPServer is a utility to provide mocked responses for
// ServiceWorkerNewScriptLoader.
class MockHTTPServer {
 public:
  struct Response {
    Response(const std::string& headers, const std::string& body)
        : headers(headers), body(body) {}

    const std::string headers;
    const std::string body;
    bool has_certificate_error = false;
  };

  void Set(const GURL& url, const Response& response) {
    responses_.erase(url);
    responses_.emplace(url, response);
  }

  const Response& Get(const GURL& url) {
    auto found = responses_.find(url);
    EXPECT_TRUE(found != responses_.end());
    return found->second;
  }

 private:
  std::map<GURL, Response> responses_;
};

// A URLLoaderFactory that returns a mocked response provided by MockHTTPServer.
//
// TODO(nhiroki): We copied this from
// service_worker_navigation_loader_unittest.cc instead of making it a common
// test helper because we might want to customize the mock factory to add more
// tests later. Merge this and that if we're convinced it's better.
class MockNetworkURLLoaderFactory final
    : public network::mojom::URLLoaderFactory {
 public:
  explicit MockNetworkURLLoaderFactory(MockHTTPServer* mock_server)
      : mock_server_(mock_server) {}

  // network::mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& url_request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    const MockHTTPServer::Response& response =
        mock_server_->Get(url_request.url);

    // Pass the response header to the client.
    net::HttpResponseInfo info;
    info.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
        net::HttpUtil::AssembleRawHeaders(response.headers.c_str(),
                                          response.headers.size()));
    network::ResourceResponseHead response_head;
    response_head.headers = info.headers;
    response_head.headers->GetMimeType(&response_head.mime_type);
    if (response.has_certificate_error) {
      response_head.cert_status = net::CERT_STATUS_DATE_INVALID;
    }

    if (response_head.headers->response_code() == 307) {
      client->OnReceiveRedirect(net::RedirectInfo(), response_head);
      return;
    }
    client->OnReceiveResponse(response_head, nullptr /* downloaded_file */);

    // Pass the response body to the client.
    uint32_t bytes_written = response.body.size();
    mojo::DataPipe data_pipe;
    MojoResult result = data_pipe.producer_handle->WriteData(
        response.body.data(), &bytes_written, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE);
    ASSERT_EQ(MOJO_RESULT_OK, result);
    client->OnStartLoadingResponseBody(std::move(data_pipe.consumer_handle));

    network::URLLoaderCompletionStatus status;
    status.error_code = net::OK;
    client->OnComplete(status);
  }

  void Clone(network::mojom::URLLoaderFactoryRequest factory) override {
    NOTREACHED();
  }

 private:
  // This is owned by ServiceWorkerNewScriptLoaderTest.
  MockHTTPServer* mock_server_;

  DISALLOW_COPY_AND_ASSIGN(MockNetworkURLLoaderFactory);
};

}  // namespace

// ServiceWorkerNewScriptLoaderTest is for testing the handling of requests for
// installing service worker scripts via ServiceWorkerNewScriptLoader.
class ServiceWorkerNewScriptLoaderTest : public testing::Test {
 public:
  ServiceWorkerNewScriptLoaderTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP),
        mock_server_(std::make_unique<MockHTTPServer>()) {}
  ~ServiceWorkerNewScriptLoaderTest() override = default;

  ServiceWorkerContextCore* context() { return helper_->context(); }

  void SetUp() override {
    helper_ = std::make_unique<EmbeddedWorkerTestHelper>(
        base::FilePath(), base::MakeRefCounted<URLLoaderFactoryGetter>());

    InitializeStorage();

    mock_server_->Set(GURL(kNormalScriptURL),
                      MockHTTPServer::Response(
                          std::string("HTTP/1.1 200 OK\n"
                                      "Content-Type: text/javascript\n\n"),
                          std::string("this body came from the network")));

    // Initialize URLLoaderFactory.
    network::mojom::URLLoaderFactoryPtr test_loader_factory;
    mock_url_loader_factory_ =
        std::make_unique<MockNetworkURLLoaderFactory>(mock_server_.get());
    helper_->url_loader_factory_getter()->SetNetworkFactoryForTesting(
        mock_url_loader_factory_.get());
  }

  void InitializeStorage() {
    base::RunLoop run_loop;
    context()->storage()->LazyInitializeForTest(run_loop.QuitClosure());
    run_loop.Run();
  }

  // Sets up ServiceWorkerRegistration and ServiceWorkerVersion. This should be
  // called before DoRequest().
  void SetUpRegistration(const GURL& script_url, const GURL& scope) {
    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = scope;
    registration_ = base::MakeRefCounted<ServiceWorkerRegistration>(
        options, context()->storage()->NewRegistrationId(),
        context()->AsWeakPtr());
    version_ = base::MakeRefCounted<ServiceWorkerVersion>(
        registration_.get(), script_url, context()->storage()->NewVersionId(),
        helper_->context()->AsWeakPtr());
    version_->SetStatus(ServiceWorkerVersion::NEW);
  }

  // Sets up ServiceWorkerRegistration and ServiceWorkerVersion with the default
  // scope.
  void SetUpRegistration(const GURL& script_url) {
    SetUpRegistration(script_url, script_url.GetWithoutFilename());
  }

  // Creates an active worker in |registration_|. After this is called,
  // |version_| will be a new, uninstalled version. The next time DoRequest() is
  // called, |version_| will attempt to update, comparing against the active
  // worker.
  void SetUpActivatedWorker(const GURL& script_url) {
    DoRequest(script_url);
    client_->RunUntilComplete();
    EXPECT_EQ(net::OK, client_->completion_status().error_code);
    loader_.reset();
    client_.reset();

    version_->set_fetch_handler_existence(
        ServiceWorkerVersion::FetchHandlerExistence::DOES_NOT_EXIST);
    version_->SetStatus(ServiceWorkerVersion::ACTIVATED);
    registration_->SetActiveVersion(version_);

    version_ = base::MakeRefCounted<ServiceWorkerVersion>(
        registration_.get(), script_url, context()->storage()->NewVersionId(),
        context()->AsWeakPtr());
    version_->SetStatus(ServiceWorkerVersion::NEW);
    version_->set_pause_after_download(true);
  }

  void DoRequest(const GURL& request_url) {
    DCHECK(registration_);
    DCHECK(version_);

    // Dummy values.
    int routing_id = 0;
    int request_id = 10;
    uint32_t options = 0;

    network::ResourceRequest request;
    request.url = version_->script_url();
    request.method = "GET";
    // TODO(nhiroki): Test importScripts() cases.
    request.resource_type = RESOURCE_TYPE_SERVICE_WORKER;

    DCHECK(!loader_);
    DCHECK(!client_);
    client_ = std::make_unique<network::TestURLLoaderClient>();
    loader_ = std::make_unique<ServiceWorkerNewScriptLoader>(
        routing_id, request_id, options, request, client_->CreateInterfacePtr(),
        version_, helper_->url_loader_factory_getter()->GetNetworkFactory(),
        net::MutableNetworkTrafficAnnotationTag(TRAFFIC_ANNOTATION_FOR_TESTS));
  }

  // Returns false if the entry for |url| doesn't exist in the storage.
  bool VerifyStoredResponse(const GURL& url) {
    int64_t cache_resource_id = LookupResourceId(url);
    if (cache_resource_id == kInvalidServiceWorkerResourceId)
      return false;

    // Verify the response status.
    size_t response_data_size = 0;
    {
      std::unique_ptr<ServiceWorkerResponseReader> reader =
          context()->storage()->CreateResponseReader(cache_resource_id);
      auto info_buffer = base::MakeRefCounted<HttpResponseInfoIOBuffer>();
      net::TestCompletionCallback cb;
      reader->ReadInfo(info_buffer.get(), cb.callback());
      int rv = cb.WaitForResult();
      if (rv < 0)
        return false;
      EXPECT_LT(0, rv);
      EXPECT_EQ("OK", info_buffer->http_info->headers->GetStatusText());
      response_data_size = info_buffer->response_data_size;
    }

    // Verify the response body.
    {
      const std::string& expected_body = mock_server_->Get(url).body;
      std::unique_ptr<ServiceWorkerResponseReader> reader =
          context()->storage()->CreateResponseReader(cache_resource_id);
      auto buffer =
          base::MakeRefCounted<net::IOBufferWithSize>(response_data_size);
      net::TestCompletionCallback cb;
      reader->ReadData(buffer.get(), buffer->size(), cb.callback());
      int rv = cb.WaitForResult();
      if (rv < 0)
        return false;
      EXPECT_EQ(static_cast<int>(expected_body.size()), rv);

      std::string received_body(buffer->data(), rv);
      EXPECT_EQ(expected_body, received_body);
    }
    return true;
  }

  int64_t LookupResourceId(const GURL& url) {
    return version_->script_cache_map()->LookupResourceId(url);
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<MockNetworkURLLoaderFactory> mock_url_loader_factory_;
  std::unique_ptr<EmbeddedWorkerTestHelper> helper_;

  scoped_refptr<ServiceWorkerRegistration> registration_;
  scoped_refptr<ServiceWorkerVersion> version_;
  std::unique_ptr<ServiceWorkerNewScriptLoader> loader_;
  std::unique_ptr<MockHTTPServer> mock_server_;

  std::unique_ptr<network::TestURLLoaderClient> client_;
};

TEST_F(ServiceWorkerNewScriptLoaderTest, Success) {
  const GURL kScriptURL(kNormalScriptURL);
  SetUpRegistration(kScriptURL);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();
  EXPECT_EQ(net::OK, client_->completion_status().error_code);

  // The client should have received the response.
  EXPECT_TRUE(client_->has_received_response());
  EXPECT_TRUE(client_->response_body().is_valid());
  std::string response;
  EXPECT_TRUE(
      mojo::BlockingCopyToString(client_->response_body_release(), &response));
  EXPECT_EQ(mock_server_->Get(kScriptURL).body, response);

  // The response should also be stored in the storage.
  EXPECT_TRUE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Success_EmptyBody) {
  const GURL kScriptURL("https://example.com/empty.js");
  mock_server_->Set(
      kScriptURL,
      MockHTTPServer::Response(std::string("HTTP/1.1 200 OK\n"
                                           "Content-Type: text/javascript\n\n"),
                               std::string()));
  SetUpRegistration(kScriptURL);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();
  EXPECT_EQ(net::OK, client_->completion_status().error_code);

  // The client should have received the response.
  EXPECT_TRUE(client_->has_received_response());
  EXPECT_TRUE(client_->response_body().is_valid());
  std::string response;
  EXPECT_TRUE(
      mojo::BlockingCopyToString(client_->response_body_release(), &response));
  EXPECT_TRUE(response.empty());

  // The response should also be stored in the storage.
  EXPECT_TRUE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Success_LargeBody) {
  // Create a response that has a larger body than the script loader's buffer
  // to test chunked data write. We chose this multiplier to avoid hitting the
  // limit of mojo's data pipe buffer (it's about kReadBufferSize * 2 as of
  // now).
  const uint32_t kBodySize =
      ServiceWorkerNewScriptLoader::kReadBufferSize * 1.6;
  const GURL kScriptURL("https://example.com/large-body.js");
  mock_server_->Set(
      kScriptURL,
      MockHTTPServer::Response(std::string("HTTP/1.1 200 OK\n"
                                           "Content-Type: text/javascript\n\n"),
                               std::string(kBodySize, 'a')));
  SetUpRegistration(kScriptURL);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();
  EXPECT_EQ(net::OK, client_->completion_status().error_code);

  // The client should have received the response.
  EXPECT_TRUE(client_->has_received_response());
  EXPECT_TRUE(client_->response_body().is_valid());
  std::string response;
  EXPECT_TRUE(
      mojo::BlockingCopyToString(client_->response_body_release(), &response));
  EXPECT_EQ(mock_server_->Get(kScriptURL).body, response);

  // The response should also be stored in the storage.
  EXPECT_TRUE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Error_404) {
  const GURL kScriptURL("https://example.com/nonexistent.js");
  mock_server_->Set(kScriptURL, MockHTTPServer::Response(
                                    std::string("HTTP/1.1 404 Not Found\n\n"),
                                    std::string()));
  SetUpRegistration(kScriptURL);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();

  // The request should be failed because of the 404 response.
  EXPECT_EQ(net::ERR_INVALID_RESPONSE, client_->completion_status().error_code);
  EXPECT_FALSE(client_->has_received_response());

  // The response shouldn't be stored in the storage.
  EXPECT_FALSE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Error_Redirect) {
  const GURL kScriptURL("https://example.com/redirect.js");
  mock_server_->Set(
      kScriptURL,
      MockHTTPServer::Response(
          std::string("HTTP/1.1 307 Temporary Redirect\n\n"), std::string()));
  SetUpRegistration(kScriptURL);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();

  // The request should be failed because of the redirected response.
  EXPECT_EQ(net::ERR_UNSAFE_REDIRECT, client_->completion_status().error_code);
  EXPECT_FALSE(client_->has_received_response());

  // The response shouldn't be stored in the storage.
  EXPECT_FALSE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Error_CertificateError) {
  // Serve a response with a certificate error.
  const GURL kScriptURL("https://example.com/certificate-error.js");
  MockHTTPServer::Response response(std::string("HTTP/1.1 200 OK\n\n"),
                                    std::string("body"));
  response.has_certificate_error = true;
  mock_server_->Set(kScriptURL, response);
  SetUpRegistration(kScriptURL);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();

  // The request should be failed because of the response with the certificate
  // error.
  EXPECT_EQ(net::ERR_CERT_DATE_INVALID,
            client_->completion_status().error_code);
  EXPECT_FALSE(client_->has_received_response());

  // The response shouldn't be stored in the storage.
  EXPECT_FALSE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Error_NoMimeType) {
  const GURL kScriptURL("https://example.com/no-mime-type.js");
  mock_server_->Set(kScriptURL, MockHTTPServer::Response(
                                    std::string("HTTP/1.1 200 OK\n\n"),
                                    std::string("body with no MIME type")));
  SetUpRegistration(kScriptURL);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();

  // The request should be failed because of the response with no MIME type.
  EXPECT_EQ(net::ERR_INSECURE_RESPONSE,
            client_->completion_status().error_code);
  EXPECT_FALSE(client_->has_received_response());

  // The response shouldn't be stored in the storage.
  EXPECT_FALSE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Error_BadMimeType) {
  const GURL kScriptURL("https://example.com/bad-mime-type.js");
  mock_server_->Set(kScriptURL, MockHTTPServer::Response(
                                    std::string("HTTP/1.1 200 OK\n"
                                                "Content-Type: text/css\n\n"),
                                    std::string("body with bad MIME type")));
  SetUpRegistration(kScriptURL);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();

  // The request should be failed because of the response with the bad MIME
  // type.
  EXPECT_EQ(net::ERR_INSECURE_RESPONSE,
            client_->completion_status().error_code);
  EXPECT_FALSE(client_->has_received_response());

  // The response shouldn't be stored in the storage.
  EXPECT_FALSE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Success_PathRestriction) {
  // |kScope| is not under the default scope ("/out-of-scope/"), but the
  // Service-Worker-Allowed header allows it.
  const GURL kScriptURL("https://example.com/out-of-scope/normal.js");
  const GURL kScope("https://example.com/in-scope/");
  mock_server_->Set(kScriptURL,
                    MockHTTPServer::Response(
                        std::string("HTTP/1.1 200 OK\n"
                                    "Content-Type: text/javascript\n"
                                    "Service-Worker-Allowed: /in-scope/\n\n"),
                        std::string()));
  SetUpRegistration(kScriptURL, kScope);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();
  EXPECT_EQ(net::OK, client_->completion_status().error_code);

  // The client should have received the response.
  EXPECT_TRUE(client_->has_received_response());
  EXPECT_TRUE(client_->response_body().is_valid());
  std::string response;
  EXPECT_TRUE(
      mojo::BlockingCopyToString(client_->response_body_release(), &response));
  EXPECT_EQ(mock_server_->Get(kScriptURL).body, response);

  // The response should also be stored in the storage.
  EXPECT_TRUE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Error_PathRestriction) {
  // |kScope| is not under the default scope ("/out-of-scope/") and the
  // Service-Worker-Allowed header is not specified.
  const GURL kScriptURL("https://example.com/out-of-scope/normal.js");
  const GURL kScope("https://example.com/in-scope/");
  mock_server_->Set(
      kScriptURL,
      MockHTTPServer::Response(std::string("HTTP/1.1 200 OK\n"
                                           "Content-Type: text/javascript\n\n"),
                               std::string()));
  SetUpRegistration(kScriptURL, kScope);
  DoRequest(kScriptURL);
  client_->RunUntilComplete();

  // The request should be failed because the scope is not allowed.
  EXPECT_EQ(net::ERR_INSECURE_RESPONSE,
            client_->completion_status().error_code);
  EXPECT_FALSE(client_->has_received_response());

  // The response shouldn't be stored in the storage.
  EXPECT_FALSE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Error_RedundantWorker) {
  const GURL kScriptURL(kNormalScriptURL);
  SetUpRegistration(kScriptURL);
  DoRequest(kScriptURL);

  // Make the service worker redundant.
  version_->Doom();
  ASSERT_TRUE(version_->is_redundant());

  client_->RunUntilComplete();

  // The request should be aborted.
  EXPECT_EQ(net::ERR_FAILED, client_->completion_status().error_code);
  EXPECT_FALSE(client_->has_received_response());

  // The response shouldn't be stored in the storage.
  EXPECT_FALSE(VerifyStoredResponse(kScriptURL));
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Update) {
  // Set up a registration with an incumbent.
  const GURL kScriptURL(kNormalScriptURL);
  SetUpRegistration(kScriptURL);
  SetUpActivatedWorker(kScriptURL);

  // Change the script on the server.
  mock_server_->Set(
      kScriptURL,
      MockHTTPServer::Response(std::string("HTTP/1.1 200 OK\n"
                                           "Content-Type: text/javascript\n\n"),
                               std::string("this is the updated body")));

  // Attempt to update.
  DoRequest(kScriptURL);
  client_->RunUntilComplete();
  EXPECT_EQ(net::OK, client_->completion_status().error_code);
  // |version_| should have installed.
  EXPECT_EQ(1UL, version_->script_cache_map()->size());
}

TEST_F(ServiceWorkerNewScriptLoaderTest, Update_IdenticalScript) {
  // Set up a registration with an incumbent.
  const GURL kScriptURL(kNormalScriptURL);
  SetUpRegistration(kScriptURL);
  SetUpActivatedWorker(kScriptURL);

  // Attempt to update.
  DoRequest(kScriptURL);
  client_->RunUntilComplete();
  EXPECT_EQ(net::OK, client_->completion_status().error_code);
  // The byte-to-byte check should detect the identical script, so the
  // |version_| should not have installed.
  EXPECT_EQ(0UL, version_->script_cache_map()->size());
}

}  // namespace content
