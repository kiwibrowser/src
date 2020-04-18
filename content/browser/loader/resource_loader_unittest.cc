// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/resource_loader.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/statistics_recorder.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/loader/resource_loader_delegate.h"
#include "content/browser/loader/test_resource_handler.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/login_delegate.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/content_paths.h"
#include "content/public/common/resource_type.h"
#include "content/public/test/mock_resource_context.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_renderer_host.h"
#include "content/test/test_content_browser_client.h"
#include "content/test/test_web_contents.h"
#include "net/base/chunked_upload_data_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/cert/x509_certificate.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/network_quality_estimator_test_util.h"
#include "net/ssl/client_cert_identity_test_util.h"
#include "net/ssl/client_cert_store.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_private_key.h"
#include "net/test/cert_test_util.h"
#include "net/test/embedded_test_server/controllable_http_response.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/test_data_directory.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_filter.h"
#include "net/url_request/url_request_interceptor.h"
#include "net/url_request/url_request_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_test_job.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/public/cpp/resource_response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

constexpr char kBodyReadFromNetBeforePausedHistogram[] =
    "Network.URLLoader.BodyReadFromNetBeforePaused";

// Stub client certificate store that returns a preset list of certificates for
// each request and records the arguments of the most recent request for later
// inspection.
class ClientCertStoreStub : public net::ClientCertStore {
 public:
  // Creates a new ClientCertStoreStub that returns |response| on query. It
  // saves the number of requests and most recently certificate authorities list
  // in |requested_authorities| and |request_count|, respectively. The caller is
  // responsible for ensuring those pointers outlive the ClientCertStoreStub.
  //
  // TODO(ppi): Make the stub independent from the internal representation of
  // SSLCertRequestInfo. For now it seems that we can neither save the
  // scoped_refptr<> (since it is never passed to us) nor copy the entire
  // CertificateRequestInfo (since there is no copy constructor).
  ClientCertStoreStub(const net::CertificateList& response,
                      int* request_count,
                      std::vector<std::string>* requested_authorities)
      : response_(std::move(response)),
        requested_authorities_(requested_authorities),
        request_count_(request_count) {
    requested_authorities_->clear();
    *request_count_ = 0;
  }

  ~ClientCertStoreStub() override {}

  // net::ClientCertStore:
  void GetClientCerts(const net::SSLCertRequestInfo& cert_request_info,
                      const ClientCertListCallback& callback) override {
    *requested_authorities_ = cert_request_info.cert_authorities;
    ++(*request_count_);

    callback.Run(net::FakeClientCertIdentityListFromCertificateList(response_));
  }

 private:
  const net::CertificateList response_;
  std::vector<std::string>* requested_authorities_;
  int* request_count_;
};

// Client certificate store which destroys its resource loader before the
// asynchronous GetClientCerts callback is called.
class LoaderDestroyingCertStore : public net::ClientCertStore {
 public:
  // Creates a client certificate store which, when looked up, posts a task to
  // reset |loader| and then call the callback. The caller is responsible for
  // ensuring the pointers remain valid until the process is complete.
  LoaderDestroyingCertStore(std::unique_ptr<ResourceLoader>* loader,
                            const base::Closure& on_loader_deleted_callback)
      : loader_(loader),
        on_loader_deleted_callback_(on_loader_deleted_callback) {}

  // net::ClientCertStore:
  void GetClientCerts(
      const net::SSLCertRequestInfo& cert_request_info,
      const ClientCertListCallback& cert_selected_callback) override {
    // Don't destroy |loader_| while it's on the stack.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&LoaderDestroyingCertStore::DoCallback,
                       base::Unretained(loader_), cert_selected_callback,
                       on_loader_deleted_callback_));
  }

 private:
  // This needs to be static because |loader| owns the
  // LoaderDestroyingCertStore (ClientCertStores are actually handles, and not
  // global cert stores).
  static void DoCallback(std::unique_ptr<ResourceLoader>* loader,
                         const ClientCertListCallback& cert_selected_callback,
                         const base::Closure& on_loader_deleted_callback) {
    loader->reset();
    cert_selected_callback.Run(net::ClientCertIdentityList());
    on_loader_deleted_callback.Run();
  }

  std::unique_ptr<ResourceLoader>* loader_;
  base::Closure on_loader_deleted_callback_;

  DISALLOW_COPY_AND_ASSIGN(LoaderDestroyingCertStore);
};

// A mock URLRequestJob which simulates an SSL client auth request.
class MockClientCertURLRequestJob : public net::URLRequestTestJob {
 public:
  MockClientCertURLRequestJob(net::URLRequest* request,
                              net::NetworkDelegate* network_delegate)
      : net::URLRequestTestJob(request, network_delegate),
        weak_factory_(this) {}

  static std::vector<std::string> test_authorities() {
    return std::vector<std::string>(1, "dummy");
  }

  // net::URLRequestTestJob:
  void Start() override {
    scoped_refptr<net::SSLCertRequestInfo> cert_request_info(
        new net::SSLCertRequestInfo);
    cert_request_info->cert_authorities = test_authorities();
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&MockClientCertURLRequestJob::NotifyCertificateRequested,
                       weak_factory_.GetWeakPtr(),
                       base::RetainedRef(cert_request_info)));
  }

  void ContinueWithCertificate(
      scoped_refptr<net::X509Certificate> cert,
      scoped_refptr<net::SSLPrivateKey> private_key) override {
    net::URLRequestTestJob::Start();
  }

 private:
  ~MockClientCertURLRequestJob() override {}

  base::WeakPtrFactory<MockClientCertURLRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MockClientCertURLRequestJob);
};

class MockClientCertJobProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  // URLRequestJobFactory::ProtocolHandler implementation:
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    return new MockClientCertURLRequestJob(request, network_delegate);
  }
};

// Set up dummy values to use in test HTTPS requests.

const net::CertStatus kTestCertError = net::CERT_STATUS_DATE_INVALID;
const int kTestSecurityBits = 256;
// SSL3 TLS_DHE_RSA_WITH_AES_256_CBC_SHA
const int kTestConnectionStatus = 0x300039;

// A mock URLRequestJob which simulates an HTTPS request.
class MockHTTPSURLRequestJob : public net::URLRequestTestJob {
 public:
  MockHTTPSURLRequestJob(net::URLRequest* request,
                         net::NetworkDelegate* network_delegate,
                         const std::string& response_headers,
                         const std::string& response_data,
                         bool auto_advance)
      : net::URLRequestTestJob(request,
                               network_delegate,
                               response_headers,
                               response_data,
                               auto_advance) {}

  // net::URLRequestTestJob:
  void GetResponseInfo(net::HttpResponseInfo* info) override {
    // Get the original response info, but override the SSL info.
    net::URLRequestJob::GetResponseInfo(info);
    info->ssl_info.cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
    info->ssl_info.cert_status = kTestCertError;
    info->ssl_info.security_bits = kTestSecurityBits;
    info->ssl_info.connection_status = kTestConnectionStatus;
  }

 private:
  ~MockHTTPSURLRequestJob() override {}

  DISALLOW_COPY_AND_ASSIGN(MockHTTPSURLRequestJob);
};

const char kRedirectHeaders[] =
    "HTTP/1.1 302 Found\n"
    "Location: https://example.test\n"
    "\n";

class MockHTTPSJobURLRequestInterceptor : public net::URLRequestInterceptor {
 public:
  explicit MockHTTPSJobURLRequestInterceptor(bool redirect)
      : redirect_(redirect) {}
  ~MockHTTPSJobURLRequestInterceptor() override {}

  // net::URLRequestInterceptor:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    std::string headers =
        redirect_ ? std::string(kRedirectHeaders, arraysize(kRedirectHeaders))
                  : net::URLRequestTestJob::test_headers();
    return new MockHTTPSURLRequestJob(request, network_delegate, headers,
                                      "dummy response", true);
  }

 private:
  bool redirect_;
};

// Test browser client that captures calls to SelectClientCertificates and
// records the arguments of the most recent call for later inspection.
class SelectCertificateBrowserClient : public TestContentBrowserClient {
 public:
  SelectCertificateBrowserClient() : call_count_(0) {}

  // Waits until the first call to SelectClientCertificate.
  void WaitForSelectCertificate() {
    select_certificate_run_loop_.Run();
    // Process any pending messages - just so tests can check if
    // SelectClientCertificate was called more than once.
    base::RunLoop().RunUntilIdle();
  }

  void SelectClientCertificate(
      WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<ClientCertificateDelegate> delegate) override {
    EXPECT_FALSE(delegate_.get());

    ++call_count_;
    passed_identities_ = std::move(client_certs);
    delegate_ = std::move(delegate);
    select_certificate_run_loop_.Quit();
  }

  std::unique_ptr<net::ClientCertStore> CreateClientCertStore(
      ResourceContext* resource_context) override {
    return std::move(dummy_cert_store_);
  }

  void SetClientCertStore(std::unique_ptr<net::ClientCertStore> store) {
    dummy_cert_store_ = std::move(store);
  }

  int call_count() { return call_count_; }
  const net::ClientCertIdentityList& passed_identities() {
    return passed_identities_;
  }

  void ContinueWithCertificate(scoped_refptr<net::X509Certificate> cert,
                               scoped_refptr<net::SSLPrivateKey> private_key) {
    delegate_->ContinueWithCertificate(std::move(cert), std::move(private_key));
    delegate_.reset();
  }

  void CancelCertificateSelection() { delegate_.reset(); }

 private:
  net::ClientCertIdentityList passed_identities_;
  int call_count_;
  std::unique_ptr<ClientCertificateDelegate> delegate_;
  std::unique_ptr<net::ClientCertStore> dummy_cert_store_;

  base::RunLoop select_certificate_run_loop_;

  DISALLOW_COPY_AND_ASSIGN(SelectCertificateBrowserClient);
};

// Wraps a ChunkedUploadDataStream to behave as non-chunked to enable upload
// progress reporting.
class NonChunkedUploadDataStream : public net::UploadDataStream {
 public:
  explicit NonChunkedUploadDataStream(uint64_t size)
      : net::UploadDataStream(false, 0), stream_(0), size_(size) {}

  void AppendData(const char* data) {
    stream_.AppendData(data, strlen(data), false);
  }

 private:
  int InitInternal(const net::NetLogWithSource& net_log) override {
    SetSize(size_);
    stream_.Init(base::BindOnce(&NonChunkedUploadDataStream::OnInitCompleted,
                                base::Unretained(this)),
                 net_log);
    return net::OK;
  }

  int ReadInternal(net::IOBuffer* buf, int buf_len) override {
    return stream_.Read(
        buf, buf_len,
        base::BindOnce(&NonChunkedUploadDataStream::OnReadCompleted,
                       base::Unretained(this)));
  }

  void ResetInternal() override { stream_.Reset(); }

  net::ChunkedUploadDataStream stream_;
  uint64_t size_;

  DISALLOW_COPY_AND_ASSIGN(NonChunkedUploadDataStream);
};

// Returns whether monitoring was successfully set up. If yes,
// StopMonitorBodyReadFromNetBeforePausedHistogram() needs to be called later to
// stop monitoring.
//
// |*output_sample| needs to stay valid until monitoring is stopped.
WARN_UNUSED_RESULT bool StartMonitorBodyReadFromNetBeforePausedHistogram(
    base::HistogramBase::Sample* output_sample) {
  return base::StatisticsRecorder::SetCallback(
      kBodyReadFromNetBeforePausedHistogram,
      base::BindRepeating(
          [](base::HistogramBase::Sample* output,
             base::HistogramBase::Sample sample) { *output = sample; },
          output_sample));
}

void StopMonitorBodyReadFromNetBeforePausedHistogram() {
  base::StatisticsRecorder::ClearCallback(
      kBodyReadFromNetBeforePausedHistogram);
}

}  // namespace

class ResourceLoaderTest : public testing::Test,
                           public ResourceLoaderDelegate {
 protected:
  ResourceLoaderTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        test_url_request_context_(true),
        resource_context_(&test_url_request_context_),
        raw_ptr_resource_handler_(nullptr),
        raw_ptr_to_request_(nullptr) {
    test_url_request_context_.set_job_factory(&job_factory_);
    test_url_request_context_.set_network_quality_estimator(
        &network_quality_estimator_);
    test_url_request_context_.Init();
  }

  // URL with a response body of test_data() where reads complete synchronously.
  GURL test_sync_url() const { return net::URLRequestTestJob::test_url_1(); }

  // URL with a response body of test_data() where reads complete
  // asynchronously.
  GURL test_async_url() const {
    return net::URLRequestTestJob::test_url_auto_advance_async_reads_1();
  }

  // URL that redirects to test_sync_url(). The ResourceLoader is set up to
  // use this URL by default.
  GURL test_redirect_url() const {
    return net::URLRequestTestJob::test_url_redirect_to_url_1();
  }

  std::string test_data() const {
    return net::URLRequestTestJob::test_data_1();
  }

  net::TestNetworkQualityEstimator* network_quality_estimator() {
    return &network_quality_estimator_;
  }

  virtual std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
  CreateProtocolHandler() {
    return net::URLRequestTestJob::CreateProtocolHandler();
  }

  virtual std::unique_ptr<ResourceHandler> WrapResourceHandler(
      std::unique_ptr<TestResourceHandler> leaf_handler,
      net::URLRequest* request) {
    return std::move(leaf_handler);
  }

  // Replaces loader_ with a new one for |request|.
  void SetUpResourceLoader(std::unique_ptr<net::URLRequest> request,
                           ResourceType resource_type,
                           bool belongs_to_main_frame) {
    raw_ptr_to_request_ = request.get();

    // A request marked as a main frame request must also belong to a main
    // frame.
    ASSERT_TRUE((resource_type != RESOURCE_TYPE_MAIN_FRAME) ||
                belongs_to_main_frame);

    RenderFrameHost* rfh = web_contents_->GetMainFrame();
    ResourceRequestInfo::AllocateForTesting(
        request.get(), resource_type, &resource_context_,
        rfh->GetProcess()->GetID(), rfh->GetRenderViewHost()->GetRoutingID(),
        rfh->GetRoutingID(), belongs_to_main_frame, true /* allow_download */,
        false /* is_async */, PREVIEWS_OFF /* previews_state */,
        nullptr /* navigation_ui_data */);
    std::unique_ptr<TestResourceHandler> resource_handler(
        new TestResourceHandler(nullptr, nullptr));
    raw_ptr_resource_handler_ = resource_handler.get();
    loader_.reset(new ResourceLoader(
        std::move(request),
        WrapResourceHandler(std::move(resource_handler), raw_ptr_to_request_),
        this, &resource_context_));
  }

  void SetUpResourceLoaderForUrl(const GURL& test_url) {
    std::unique_ptr<net::URLRequest> request(
        resource_context_.GetRequestContext()->CreateRequest(
            test_url, net::DEFAULT_PRIORITY, nullptr /* delegate */,
            TRAFFIC_ANNOTATION_FOR_TESTS));
    SetUpResourceLoader(std::move(request), RESOURCE_TYPE_MAIN_FRAME, true);
  }

  void SetUp() override {
    job_factory_.SetProtocolHandler("test", CreateProtocolHandler());
    net::URLRequestFailedJob::AddUrlHandler();

    browser_context_.reset(new TestBrowserContext());
    scoped_refptr<SiteInstance> site_instance =
        SiteInstance::Create(browser_context_.get());
    web_contents_ =
        TestWebContents::Create(browser_context_.get(), site_instance.get());
    SetUpResourceLoaderForUrl(test_redirect_url());
  }

  void TearDown() override {
    // Destroy the WebContents and pump the event loop before destroying
    // |rvh_test_enabler_| and |thread_bundle_|. This lets asynchronous cleanup
    // tasks complete.
    web_contents_.reset();

    // Clean up handlers.
    net::URLRequestFilter::GetInstance()->ClearHandlers();

    base::RunLoop().RunUntilIdle();
  }

  // ResourceLoaderDelegate:
  scoped_refptr<LoginDelegate> CreateLoginDelegate(
      ResourceLoader* loader,
      net::AuthChallengeInfo* auth_info) override {
    return nullptr;
  }
  bool HandleExternalProtocol(ResourceLoader* loader,
                              const GURL& url) override {
    EXPECT_EQ(loader, loader_.get());
    ++handle_external_protocol_;

    // Check that calls to HandleExternalProtocol always happen after the calls
    // to the ResourceHandler's OnWillStart and OnRequestRedirected.
    EXPECT_EQ(handle_external_protocol_,
              raw_ptr_resource_handler_->on_will_start_called() +
                  raw_ptr_resource_handler_->on_request_redirected_called());

    bool return_value = handle_external_protocol_results_.front();
    if (handle_external_protocol_results_.size() > 1)
      handle_external_protocol_results_.pop_front();
    return return_value;
  }
  void DidStartRequest(ResourceLoader* loader) override {
    EXPECT_EQ(loader, loader_.get());
    EXPECT_EQ(0, did_finish_loading_);
    EXPECT_EQ(0, did_start_request_);
    ++did_start_request_;
  }
  void DidReceiveRedirect(ResourceLoader* loader,
                          const GURL& new_url,
                          network::ResourceResponse* response) override {
    EXPECT_EQ(loader, loader_.get());
    EXPECT_EQ(0, did_finish_loading_);
    EXPECT_EQ(0, did_receive_response_);
    EXPECT_EQ(1, did_start_request_);
    ++did_received_redirect_;
  }
  void DidReceiveResponse(ResourceLoader* loader,
                          network::ResourceResponse* response) override {
    EXPECT_EQ(loader, loader_.get());
    EXPECT_EQ(0, did_finish_loading_);
    EXPECT_EQ(0, did_receive_response_);
    EXPECT_EQ(1, did_start_request_);
    ++did_receive_response_;
  }
  void DidFinishLoading(ResourceLoader* loader) override {
    EXPECT_EQ(loader, loader_.get());
    EXPECT_EQ(0, did_finish_loading_);

    // Shouldn't be in a recursive ResourceHandler call - this is normally where
    // the ResourceLoader (And thus the ResourceHandler chain) is destroyed.
    EXPECT_EQ(0, raw_ptr_resource_handler_->call_depth());

    ++did_finish_loading_;
  }

  TestBrowserThreadBundle thread_bundle_;
  RenderViewHostTestEnabler rvh_test_enabler_;

  // Record which ResourceDispatcherHostDelegate methods have been invoked.
  int did_start_request_ = 0;
  int did_received_redirect_ = 0;
  int did_receive_response_ = 0;
  int did_finish_loading_ = 0;
  int handle_external_protocol_ = 0;

  // Allows controlling the return values of sequential calls to
  // HandleExternalProtocol. Values are removed by the measure they are used
  // but the last one which is used for all following calls.
  base::circular_deque<bool> handle_external_protocol_results_{false};

  net::URLRequestJobFactoryImpl job_factory_;
  net::TestNetworkQualityEstimator network_quality_estimator_;
  net::TestURLRequestContext test_url_request_context_;
  MockResourceContext resource_context_;
  std::unique_ptr<TestBrowserContext> browser_context_;
  std::unique_ptr<TestWebContents> web_contents_;

  // The ResourceLoader owns the URLRequest and the ResourceHandler.
  TestResourceHandler* raw_ptr_resource_handler_;
  net::URLRequest* raw_ptr_to_request_;
  std::unique_ptr<ResourceLoader> loader_;
};

class ClientCertResourceLoaderTest : public ResourceLoaderTest {
 protected:
  std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>
  CreateProtocolHandler() override {
    return base::WrapUnique(new MockClientCertJobProtocolHandler);
  }

  void SetUp() override {
    ResourceLoaderTest::SetUp();
    // These tests don't expect any redirects.
    SetUpResourceLoaderForUrl(test_sync_url());
  }
};

// A ResourceLoaderTest that intercepts https://example.test and
// https://example-redirect.test URLs and sets SSL info on the
// responses. The latter serves a Location: header in the response.
class HTTPSSecurityInfoResourceLoaderTest : public ResourceLoaderTest {
 public:
  HTTPSSecurityInfoResourceLoaderTest()
      : ResourceLoaderTest(),
        test_https_url_("https://example.test"),
        test_https_redirect_url_("https://example-redirect.test") {}

  ~HTTPSSecurityInfoResourceLoaderTest() override {}

  const GURL& test_https_url() const { return test_https_url_; }
  const GURL& test_https_redirect_url() const {
    return test_https_redirect_url_;
  }

 protected:
  void SetUp() override {
    ResourceLoaderTest::SetUp();
    net::URLRequestFilter::GetInstance()->ClearHandlers();
    net::URLRequestFilter::GetInstance()->AddHostnameInterceptor(
        "https", "example.test",
        std::unique_ptr<net::URLRequestInterceptor>(
            new MockHTTPSJobURLRequestInterceptor(false /* redirect */)));
    net::URLRequestFilter::GetInstance()->AddHostnameInterceptor(
        "https", "example-redirect.test",
        std::unique_ptr<net::URLRequestInterceptor>(
            new MockHTTPSJobURLRequestInterceptor(true /* redirect */)));
  }

 private:
  const GURL test_https_url_;
  const GURL test_https_redirect_url_;
};

TEST_F(HTTPSSecurityInfoResourceLoaderTest, CertStatusOnResponse) {
  SetUpResourceLoaderForUrl(test_https_url());
  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(kTestCertError,
            raw_ptr_resource_handler_->resource_response()->head.cert_status);
}

// Tests that client certificates are requested with ClientCertStore lookup.
TEST_F(ClientCertResourceLoaderTest, WithStoreLookup) {
  // Set up the test client cert store.
  int store_request_count;
  std::vector<std::string> store_requested_authorities;
  scoped_refptr<net::X509Certificate> test_cert =
      net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
  ASSERT_TRUE(test_cert);
  net::CertificateList dummy_certs(1, test_cert);
  std::unique_ptr<ClientCertStoreStub> test_store(new ClientCertStoreStub(
      dummy_certs, &store_request_count, &store_requested_authorities));

  // Plug in test content browser client.
  SelectCertificateBrowserClient test_client;
  test_client.SetClientCertStore(std::move(test_store));
  ContentBrowserClient* old_client = SetBrowserClientForTesting(&test_client);

  // Start the request and wait for it to pause.
  loader_->StartRequest();
  test_client.WaitForSelectCertificate();

  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  // Check if the test store was queried against correct |cert_authorities|.
  EXPECT_EQ(1, store_request_count);
  EXPECT_EQ(MockClientCertURLRequestJob::test_authorities(),
            store_requested_authorities);

  // Check if the retrieved certificates were passed to the content browser
  // client.
  EXPECT_EQ(1, test_client.call_count());
  EXPECT_EQ(1U, test_client.passed_identities().size());
  EXPECT_EQ(test_cert.get(), test_client.passed_identities()[0]->certificate());

  // Continue the request.
  test_client.ContinueWithCertificate(nullptr, nullptr);
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(net::OK, raw_ptr_resource_handler_->final_status().error());

  // Restore the original content browser client.
  SetBrowserClientForTesting(old_client);
}

// Tests that client certificates are requested on a platform with NULL
// ClientCertStore.
TEST_F(ClientCertResourceLoaderTest, WithNullStore) {
  // Plug in test content browser client.
  SelectCertificateBrowserClient test_client;
  ContentBrowserClient* old_client = SetBrowserClientForTesting(&test_client);

  // Start the request and wait for it to pause.
  loader_->StartRequest();
  test_client.WaitForSelectCertificate();

  // Check if the SelectClientCertificate was called on the content browser
  // client.
  EXPECT_EQ(1, test_client.call_count());
  EXPECT_EQ(net::ClientCertIdentityList(), test_client.passed_identities());

  // Continue the request.
  test_client.ContinueWithCertificate(nullptr, nullptr);
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(net::OK, raw_ptr_resource_handler_->final_status().error());

  // Restore the original content browser client.
  SetBrowserClientForTesting(old_client);
}

// Tests that the ContentBrowserClient may cancel a certificate request.
TEST_F(ClientCertResourceLoaderTest, CancelSelection) {
  // Plug in test content browser client.
  SelectCertificateBrowserClient test_client;
  ContentBrowserClient* old_client = SetBrowserClientForTesting(&test_client);

  // Start the request and wait for it to pause.
  loader_->StartRequest();
  test_client.WaitForSelectCertificate();

  // Check if the SelectClientCertificate was called on the content browser
  // client.
  EXPECT_EQ(1, test_client.call_count());
  EXPECT_EQ(net::ClientCertIdentityList(), test_client.passed_identities());

  // Cancel the request.
  test_client.CancelCertificateSelection();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(net::ERR_SSL_CLIENT_AUTH_CERT_NEEDED,
            raw_ptr_resource_handler_->final_status().error());

  // Restore the original content browser client.
  SetBrowserClientForTesting(old_client);
}

// Verifies that requests without WebContents attached abort.
TEST_F(ClientCertResourceLoaderTest, NoWebContents) {
  // Destroy the WebContents before starting the request.
  web_contents_.reset();

  // Plug in test content browser client.
  SelectCertificateBrowserClient test_client;
  ContentBrowserClient* old_client = SetBrowserClientForTesting(&test_client);

  // Start the request and wait for it to complete.
  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();

  // Check that SelectClientCertificate wasn't called and the request aborted.
  EXPECT_EQ(0, test_client.call_count());
  EXPECT_EQ(net::ERR_SSL_CLIENT_AUTH_CERT_NEEDED,
            raw_ptr_resource_handler_->final_status().error());

  // Restore the original content browser client.
  SetBrowserClientForTesting(old_client);
}

// Verifies that ClientCertStore's callback doesn't crash if called after the
// loader is destroyed.
TEST_F(ClientCertResourceLoaderTest, StoreAsyncCancel) {
  base::RunLoop loader_destroyed_run_loop;
  LoaderDestroyingCertStore* test_store =
      new LoaderDestroyingCertStore(&loader_,
                                    loader_destroyed_run_loop.QuitClosure());

  // Plug in test content browser client.
  SelectCertificateBrowserClient test_client;
  test_client.SetClientCertStore(base::WrapUnique(test_store));
  ContentBrowserClient* old_client = SetBrowserClientForTesting(&test_client);

  loader_->StartRequest();
  loader_destroyed_run_loop.Run();
  EXPECT_FALSE(loader_);

  // Pump the event loop to ensure nothing asynchronous crashes either.
  base::RunLoop().RunUntilIdle();

  // Restore the original content browser client.
  SetBrowserClientForTesting(old_client);
}

// Tests that a RESOURCE_TYPE_PREFETCH request sets the LOAD_PREFETCH flag.
TEST_F(ResourceLoaderTest, PrefetchFlag) {
  std::unique_ptr<net::URLRequest> request(
      resource_context_.GetRequestContext()->CreateRequest(
          test_async_url(), net::DEFAULT_PRIORITY, nullptr /* delegate */,
          TRAFFIC_ANNOTATION_FOR_TESTS));
  SetUpResourceLoader(std::move(request), RESOURCE_TYPE_PREFETCH, true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());
}

// Test the case the ResourceHandler defers nothing.
TEST_F(ResourceLoaderTest, SyncResourceHandler) {
  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(1, did_start_request_);
  EXPECT_EQ(1, did_received_redirect_);
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(2, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::OK, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());
}

// Same as above, except reads complete asynchronously, and there's no redirect.
TEST_F(ResourceLoaderTest, SyncResourceHandlerAsyncReads) {
  SetUpResourceLoaderForUrl(test_async_url());

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(1, did_start_request_);
  EXPECT_EQ(0, did_received_redirect_);
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, handle_external_protocol_);
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::OK, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());
}

// Test the case where ResourceHandler defers nothing and the request is handled
// as an external protocol on start.
TEST_F(ResourceLoaderTest, SyncExternalProtocolHandlingOnStart) {
  handle_external_protocol_results_ = {true};

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(0, did_start_request_);
  EXPECT_EQ(0, did_received_redirect_);
  EXPECT_EQ(0, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_TRUE(raw_ptr_resource_handler_->body().empty());
}

// Test the case where ResourceHandler defers nothing and the request is handled
// as an external protocol on redirect.
TEST_F(ResourceLoaderTest, SyncExternalProtocolHandlingOnRedirect) {
  handle_external_protocol_results_ = {false, true};

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(1, did_start_request_);
  EXPECT_EQ(1, did_received_redirect_);
  EXPECT_EQ(0, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(2, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_TRUE(raw_ptr_resource_handler_->body().empty());
}

// Test the case the ResourceHandler defers everything.
TEST_F(ResourceLoaderTest, AsyncResourceHandler) {
  raw_ptr_resource_handler_->set_defer_on_will_start(true);
  raw_ptr_resource_handler_->set_defer_on_request_redirected(true);
  raw_ptr_resource_handler_->set_defer_on_response_started(true);
  raw_ptr_resource_handler_->set_defer_on_will_read(true);
  raw_ptr_resource_handler_->set_defer_on_read_completed(true);
  raw_ptr_resource_handler_->set_defer_on_read_eof(true);
  raw_ptr_resource_handler_->set_defer_on_response_completed(true);

  // Start and run until OnWillStart.
  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, did_start_request_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(0, handle_external_protocol_);

  // Resume and run until OnRequestRedirected.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(1, handle_external_protocol_);

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_received_redirect_);
  EXPECT_EQ(0, did_receive_response_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(1, handle_external_protocol_);

  // Resume and run until OnResponseStarted.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(2, handle_external_protocol_);

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(0, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  // Resume and run until OnWillRead.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_read_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  // Resume and run until OnReadCompleted.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  // Defer on the next OnWillRead call, for the EOF.
  raw_ptr_resource_handler_->set_defer_on_will_read(true);

  // Resume and run until the next OnWillRead call.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  // Resume and run until the final 0-byte read, signaling EOF.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_eof_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());

  // Resume and run until OnResponseCompleted is called, which again defers the
  // request.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(0, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::OK, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());

  // Resume and run until all pending tasks. Note that OnResponseCompleted was
  // invoked in the previous section, so can't use RunUntilCompleted().
  raw_ptr_resource_handler_->Resume();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::OK, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());
  EXPECT_EQ(2, handle_external_protocol_);
}

// Same as above, except reads complete asynchronously and there's no redirect.
TEST_F(ResourceLoaderTest, AsyncResourceHandlerAsyncReads) {
  SetUpResourceLoaderForUrl(test_async_url());

  raw_ptr_resource_handler_->set_defer_on_will_start(true);
  raw_ptr_resource_handler_->set_defer_on_response_started(true);
  raw_ptr_resource_handler_->set_defer_on_will_read(true);
  raw_ptr_resource_handler_->set_defer_on_read_completed(true);
  raw_ptr_resource_handler_->set_defer_on_read_eof(true);
  raw_ptr_resource_handler_->set_defer_on_response_completed(true);

  // Start and run until OnWillStart.
  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, did_start_request_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(0, handle_external_protocol_);

  // Resume and run until OnResponseStarted.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(1, handle_external_protocol_);

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(0, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  // Resume and run until OnWillRead.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_read_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  // Resume and run until OnReadCompleted.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  // Defer on the next OnWillRead call, for the EOF.
  raw_ptr_resource_handler_->set_defer_on_will_read(true);

  // Resume and run until the next OnWillRead.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  // Resume and run until the final 0-byte read, signalling EOF.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_eof_called());

  // Spinning the message loop should not advance the state further.
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());

  // Resume and run until OnResponseCompleted is called, which again defers the
  // request.
  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(0, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::OK, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());

  // Resume and run until all pending tasks. Note that OnResponseCompleted was
  // invoked in the previous section, so can't use RunUntilCompleted().
  raw_ptr_resource_handler_->Resume();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::OK, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());
  EXPECT_EQ(1, handle_external_protocol_);
}

TEST_F(ResourceLoaderTest, SyncCancelOnWillStart) {
  raw_ptr_resource_handler_->set_on_will_start_result(false);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, did_start_request_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(0, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, SyncCancelOnRequestRedirected) {
  raw_ptr_resource_handler_->set_on_request_redirected_result(false);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_received_redirect_);
  EXPECT_EQ(0, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, SyncCancelOnResponseStarted) {
  raw_ptr_resource_handler_->set_on_response_started_result(false);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, SyncCancelOnWillRead) {
  raw_ptr_resource_handler_->set_on_will_read_result(false);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, SyncCancelOnReadCompleted) {
  raw_ptr_resource_handler_->set_on_read_completed_result(false);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_LT(0u, raw_ptr_resource_handler_->body().size());
}

TEST_F(ResourceLoaderTest, SyncCancelOnReceivedEof) {
  raw_ptr_resource_handler_->set_on_read_eof_result(false);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, SyncCancelOnAsyncReadCompleted) {
  SetUpResourceLoaderForUrl(test_async_url());
  raw_ptr_resource_handler_->set_on_read_completed_result(false);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_LT(0u, raw_ptr_resource_handler_->body().size());
}

TEST_F(ResourceLoaderTest, SyncCancelOnAsyncReceivedEof) {
  SetUpResourceLoaderForUrl(test_async_url());
  raw_ptr_resource_handler_->set_on_read_eof_result(false);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, AsyncCancelOnWillStart) {
  raw_ptr_resource_handler_->set_defer_on_will_start(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  raw_ptr_resource_handler_->CancelWithError(net::ERR_FAILED);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, did_start_request_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(0, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, AsyncCancelOnRequestRedirected) {
  raw_ptr_resource_handler_->set_defer_on_request_redirected(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  raw_ptr_resource_handler_->CancelWithError(net::ERR_FAILED);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_received_redirect_);
  EXPECT_EQ(0, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, AsyncCancelOnResponseStarted) {
  raw_ptr_resource_handler_->set_defer_on_response_started(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  raw_ptr_resource_handler_->CancelWithError(net::ERR_FAILED);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, AsyncCancelOnWillRead) {
  raw_ptr_resource_handler_->set_defer_on_will_read(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  raw_ptr_resource_handler_->CancelWithError(net::ERR_FAILED);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_read_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
}

TEST_F(ResourceLoaderTest, AsyncCancelOnReadCompleted) {
  raw_ptr_resource_handler_->set_defer_on_read_completed(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  raw_ptr_resource_handler_->CancelWithError(net::ERR_FAILED);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_LT(0u, raw_ptr_resource_handler_->body().size());
}

TEST_F(ResourceLoaderTest, AsyncCancelOnReceivedEof) {
  raw_ptr_resource_handler_->set_defer_on_read_eof(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  raw_ptr_resource_handler_->CancelWithError(net::ERR_FAILED);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, AsyncCancelOnAsyncReadCompleted) {
  SetUpResourceLoaderForUrl(test_async_url());
  raw_ptr_resource_handler_->set_defer_on_read_completed(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  raw_ptr_resource_handler_->CancelWithError(net::ERR_FAILED);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_LT(0u, raw_ptr_resource_handler_->body().size());
}

TEST_F(ResourceLoaderTest, AsyncCancelOnAsyncReceivedEof) {
  SetUpResourceLoaderForUrl(test_async_url());
  raw_ptr_resource_handler_->set_defer_on_read_eof(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  raw_ptr_resource_handler_->CancelWithError(net::ERR_FAILED);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_read_eof_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ(test_data(), raw_ptr_resource_handler_->body());
}

// Tests the request being deferred and then being handled as an external
// protocol, both on start.
TEST_F(ResourceLoaderTest, AsyncExternalProtocolHandlingOnStart) {
  handle_external_protocol_results_ = {true};
  raw_ptr_resource_handler_->set_defer_on_will_start(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(0, did_finish_loading_);
  EXPECT_EQ(0, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(0, did_start_request_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

// Tests the request being deferred and then being handled as an external
// protocol, both on redirect.
TEST_F(ResourceLoaderTest, AsyncExternalProtocolHandlingOnRedirect) {
  handle_external_protocol_results_ = {false, true};
  raw_ptr_resource_handler_->set_defer_on_request_redirected(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  EXPECT_EQ(1, did_start_request_);
  EXPECT_EQ(1, did_received_redirect_);
  EXPECT_EQ(0, did_finish_loading_);
  EXPECT_EQ(1, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());

  raw_ptr_resource_handler_->Resume();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(1, did_start_request_);
  EXPECT_EQ(1, did_received_redirect_);
  EXPECT_EQ(0, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(2, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());

  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, RequestFailsOnStart) {
  SetUpResourceLoaderForUrl(
      net::URLRequestFailedJob::GetMockHttpUrlWithFailurePhase(
          net::URLRequestFailedJob::START, net::ERR_FAILED));

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, did_received_redirect_);
  EXPECT_EQ(0, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, RequestFailsOnReadSync) {
  SetUpResourceLoaderForUrl(
      net::URLRequestFailedJob::GetMockHttpUrlWithFailurePhase(
          net::URLRequestFailedJob::READ_SYNC, net::ERR_FAILED));

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, did_received_redirect_);
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, RequestFailsOnReadAsync) {
  SetUpResourceLoaderForUrl(
      net::URLRequestFailedJob::GetMockHttpUrlWithFailurePhase(
          net::URLRequestFailedJob::READ_ASYNC, net::ERR_FAILED));

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0, did_received_redirect_);
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::ERR_FAILED, raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, OutOfBandCancelDuringStart) {
  SetUpResourceLoaderForUrl(
      net::URLRequestFailedJob::GetMockHttpUrlWithFailurePhase(
          net::URLRequestFailedJob::START, net::ERR_IO_PENDING));

  loader_->StartRequest();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(1, handle_external_protocol_);

  // Can't cancel through the ResourceHandler, since that depends on
  // ResourceDispatachHost, which these tests don't use.
  loader_->CancelRequest(false);
  raw_ptr_resource_handler_->WaitUntilResponseComplete();

  EXPECT_EQ(0, did_received_redirect_);
  EXPECT_EQ(0, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, OutOfBandCancelDuringRead) {
  SetUpResourceLoaderForUrl(
      net::URLRequestFailedJob::GetMockHttpUrlWithFailurePhase(
          net::URLRequestFailedJob::READ_SYNC, net::ERR_IO_PENDING));

  loader_->StartRequest();
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(1, handle_external_protocol_);

  // Can't cancel through the ResourceHandler, since that depends on
  // ResourceDispatachHost, which these tests don't use.
  loader_->CancelRequest(false);
  raw_ptr_resource_handler_->WaitUntilResponseComplete();
  EXPECT_EQ(0, did_received_redirect_);
  EXPECT_EQ(1, did_receive_response_);
  EXPECT_EQ(1, did_finish_loading_);
  EXPECT_EQ(1, handle_external_protocol_);
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_will_start_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_request_redirected_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_completed_called());
  EXPECT_EQ(net::ERR_ABORTED,
            raw_ptr_resource_handler_->final_status().error());
  EXPECT_EQ("", raw_ptr_resource_handler_->body());
}

TEST_F(ResourceLoaderTest, ResumeCanceledRequest) {
  raw_ptr_resource_handler_->set_defer_on_will_start(true);

  loader_->StartRequest();
  raw_ptr_resource_handler_->WaitUntilDeferred();
  loader_->CancelRequest(true);
  raw_ptr_resource_handler_->Resume();
}

class EffectiveConnectionTypeResourceLoaderTest : public ResourceLoaderTest {
 public:
  void VerifyEffectiveConnectionType(
      ResourceType resource_type,
      bool belongs_to_main_frame,
      net::EffectiveConnectionType set_type,
      net::EffectiveConnectionType expected_type) {
    network_quality_estimator()->set_effective_connection_type(set_type);

    // Start the request and wait for it to finish.
    std::unique_ptr<net::URLRequest> request(
        resource_context_.GetRequestContext()->CreateRequest(
            test_redirect_url(), net::DEFAULT_PRIORITY, nullptr /* delegate */,
            TRAFFIC_ANNOTATION_FOR_TESTS));
    SetUpResourceLoader(std::move(request), resource_type,
                        belongs_to_main_frame);

    // Send the request and wait until it completes.
    loader_->StartRequest();
    raw_ptr_resource_handler_->WaitUntilResponseComplete();
    ASSERT_EQ(net::URLRequestStatus::SUCCESS,
              raw_ptr_to_request_->status().status());

    EXPECT_EQ(expected_type, raw_ptr_resource_handler_->resource_response()
                                 ->head.effective_connection_type);
  }
};

// Tests that the effective connection type is set on main frame requests.
TEST_F(EffectiveConnectionTypeResourceLoaderTest, Slow2G) {
  VerifyEffectiveConnectionType(RESOURCE_TYPE_MAIN_FRAME, true,
                                net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G,
                                net::EFFECTIVE_CONNECTION_TYPE_SLOW_2G);
}

// Tests that the effective connection type is set on main frame requests.
TEST_F(EffectiveConnectionTypeResourceLoaderTest, 3G) {
  VerifyEffectiveConnectionType(RESOURCE_TYPE_MAIN_FRAME, true,
                                net::EFFECTIVE_CONNECTION_TYPE_3G,
                                net::EFFECTIVE_CONNECTION_TYPE_3G);
}

// Tests that the effective connection type is not set on requests that belong
// to main frame.
TEST_F(EffectiveConnectionTypeResourceLoaderTest, BelongsToMainFrame) {
  VerifyEffectiveConnectionType(RESOURCE_TYPE_OBJECT, true,
                                net::EFFECTIVE_CONNECTION_TYPE_3G,
                                net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN);
}

// Tests that the effective connection type is not set on non-main frame
// requests.
TEST_F(EffectiveConnectionTypeResourceLoaderTest, DoesNotBelongToMainFrame) {
  VerifyEffectiveConnectionType(RESOURCE_TYPE_OBJECT, false,
                                net::EFFECTIVE_CONNECTION_TYPE_3G,
                                net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN);
}

TEST_F(ResourceLoaderTest, PauseReadingBodyFromNetBeforeRespnoseHeaders) {
  static constexpr char kPath[] = "/hello.html";
  static constexpr char kBodyContents[] = "This is the data as you requested.";

  base::HistogramBase::Sample output_sample = -1;
  EXPECT_TRUE(StartMonitorBodyReadFromNetBeforePausedHistogram(&output_sample));

  net::EmbeddedTestServer server;
  net::test_server::ControllableHttpResponse response_controller(&server,
                                                                 kPath);
  ASSERT_TRUE(server.Start());

  SetUpResourceLoaderForUrl(server.GetURL(kPath));
  loader_->StartRequest();

  // Pausing reading response body from network stops future reads from the
  // underlying URLRequest. So no data should be sent using the response body
  // data pipe.
  loader_->PauseReadingBodyFromNet();

  response_controller.WaitForRequest();
  response_controller.Send(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n\r\n" +
      std::string(kBodyContents));
  response_controller.Done();

  // We will still receive the response header, although there won't be any data
  // available until ResumeReadBodyFromNet() is called.
  raw_ptr_resource_handler_->WaitUntilResponseStarted();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());
  EXPECT_EQ(0, raw_ptr_resource_handler_->on_read_completed_called());

  // Wait for a little amount of time so that if the loader mistakenly reads
  // response body from the underlying URLRequest, it is easier to find out.
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(),
      base::TimeDelta::FromMilliseconds(100));
  run_loop.Run();

  EXPECT_TRUE(raw_ptr_resource_handler_->body().empty());

  loader_->ResumeReadingBodyFromNet();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();

  EXPECT_EQ(kBodyContents, raw_ptr_resource_handler_->body());

  loader_.reset();
  EXPECT_EQ(0, output_sample);
  StopMonitorBodyReadFromNetBeforePausedHistogram();
}

TEST_F(ResourceLoaderTest, PauseReadingBodyFromNetWhenReadIsPending) {
  static constexpr char kPath[] = "/hello.html";
  static constexpr char kBodyContentsFirstHalf[] = "This is the first half.";
  static constexpr char kBodyContentsSecondHalf[] = "This is the second half.";

  base::HistogramBase::Sample output_sample = -1;
  EXPECT_TRUE(StartMonitorBodyReadFromNetBeforePausedHistogram(&output_sample));

  net::EmbeddedTestServer server;
  net::test_server::ControllableHttpResponse response_controller(&server,
                                                                 kPath);
  ASSERT_TRUE(server.Start());

  SetUpResourceLoaderForUrl(server.GetURL(kPath));
  loader_->StartRequest();

  response_controller.WaitForRequest();
  response_controller.Send(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n\r\n" +
      std::string(kBodyContentsFirstHalf));

  raw_ptr_resource_handler_->WaitUntilResponseStarted();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());

  loader_->PauseReadingBodyFromNet();

  response_controller.Send(kBodyContentsSecondHalf);
  response_controller.Done();

  // It is uncertain how much data has been read before reading is actually
  // paused, because if there is a pending read when PauseReadingBodyFromNet()
  // arrives, the pending read won't be cancelled. Therefore, this test only
  // checks that after ResumeReadingBodyFromNet() we should be able to get the
  // whole response body.
  loader_->ResumeReadingBodyFromNet();
  raw_ptr_resource_handler_->WaitUntilResponseComplete();

  EXPECT_EQ(std::string(kBodyContentsFirstHalf) +
                std::string(kBodyContentsSecondHalf),
            raw_ptr_resource_handler_->body());

  loader_.reset();
  EXPECT_LE(0, output_sample);
  StopMonitorBodyReadFromNetBeforePausedHistogram();
}

TEST_F(ResourceLoaderTest, MultiplePauseResumeReadingBodyFromNet) {
  static constexpr char kPath[] = "/hello.html";
  static constexpr char kBodyContentsFirstHalf[] = "This is the first half.";
  static constexpr char kBodyContentsSecondHalf[] = "This is the second half.";

  base::HistogramBase::Sample output_sample = -1;
  EXPECT_TRUE(StartMonitorBodyReadFromNetBeforePausedHistogram(&output_sample));

  net::EmbeddedTestServer server;
  net::test_server::ControllableHttpResponse response_controller(&server,
                                                                 kPath);
  ASSERT_TRUE(server.Start());

  SetUpResourceLoaderForUrl(server.GetURL(kPath));
  loader_->StartRequest();

  // It is okay to call ResumeReadingBodyFromNet() even if there is no prior
  // PauseReadingBodyFromNet().
  loader_->ResumeReadingBodyFromNet();

  response_controller.WaitForRequest();
  response_controller.Send(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n\r\n" +
      std::string(kBodyContentsFirstHalf));

  loader_->PauseReadingBodyFromNet();

  raw_ptr_resource_handler_->WaitUntilResponseStarted();
  EXPECT_EQ(1, raw_ptr_resource_handler_->on_response_started_called());

  loader_->PauseReadingBodyFromNet();
  loader_->PauseReadingBodyFromNet();

  response_controller.Send(kBodyContentsSecondHalf);
  response_controller.Done();

  // One ResumeReadingBodyFromNet() call will resume reading even if there are
  // multiple PauseReadingBodyFromNet() calls before it.
  loader_->ResumeReadingBodyFromNet();

  raw_ptr_resource_handler_->WaitUntilResponseComplete();

  EXPECT_EQ(std::string(kBodyContentsFirstHalf) +
                std::string(kBodyContentsSecondHalf),
            raw_ptr_resource_handler_->body());

  loader_.reset();
  EXPECT_LE(0, output_sample);
  StopMonitorBodyReadFromNetBeforePausedHistogram();
}

}  // namespace content
