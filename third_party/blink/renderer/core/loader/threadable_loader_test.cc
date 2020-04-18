// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/threadable_loader.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/memory/scoped_refptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_url_load_timing.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/public/platform/web_worker_fetch_context.h"
#include "third_party/blink/renderer/core/loader/document_threadable_loader.h"
#include "third_party/blink/renderer/core/loader/threadable_loader_client.h"
#include "third_party/blink/renderer/core/loader/threadable_loading_context.h"
#include "third_party/blink/renderer/core/loader/worker_fetch_context.h"
#include "third_party/blink/renderer/core/loader/worker_threadable_loader.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/core/workers/worker_reporting_proxy.h"
#include "third_party/blink/renderer/core/workers/worker_thread_test_helper.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_timing_info.h"
#include "third_party/blink/renderer/platform/loader/testing/web_url_loader_factory_with_mock.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"
#include "third_party/blink/renderer/platform/waitable_event.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

using testing::_;
using testing::InSequence;
using testing::InvokeWithoutArgs;
using testing::StrEq;
using testing::Truly;
using Checkpoint = testing::StrictMock<testing::MockFunction<void(int)>>;

constexpr char kFileName[] = "fox-null-terminated.html";

class MockThreadableLoaderClient : public ThreadableLoaderClient {
 public:
  static std::unique_ptr<MockThreadableLoaderClient> Create() {
    return base::WrapUnique(
        new testing::StrictMock<MockThreadableLoaderClient>);
  }
  MOCK_METHOD2(DidSendData, void(unsigned long long, unsigned long long));
  MOCK_METHOD3(DidReceiveResponseMock,
               void(unsigned long,
                    const ResourceResponse&,
                    WebDataConsumerHandle*));
  void DidReceiveResponse(
      unsigned long identifier,
      const ResourceResponse& response,
      std::unique_ptr<WebDataConsumerHandle> handle) override {
    DidReceiveResponseMock(identifier, response, handle.get());
  }
  MOCK_METHOD2(DidReceiveData, void(const char*, unsigned));
  MOCK_METHOD2(DidReceiveCachedMetadata, void(const char*, int));
  MOCK_METHOD1(DidFinishLoading, void(unsigned long));
  MOCK_METHOD1(DidFail, void(const ResourceError&));
  MOCK_METHOD0(DidFailRedirectCheck, void());
  MOCK_METHOD1(DidReceiveResourceTiming, void(const ResourceTimingInfo&));
  MOCK_METHOD1(DidDownloadData, void(int));

 protected:
  MockThreadableLoaderClient() = default;
};

bool IsCancellation(const ResourceError& error) {
  return error.IsCancellation();
}

bool IsNotCancellation(const ResourceError& error) {
  return !error.IsCancellation();
}

KURL SuccessURL() {
  return KURL("http://example.com/success").Copy();
}
KURL ErrorURL() {
  return KURL("http://example.com/error").Copy();
}
KURL RedirectURL() {
  return KURL("http://example.com/redirect").Copy();
}
KURL RedirectLoopURL() {
  return KURL("http://example.com/loop").Copy();
}

void ServeAsynchronousRequests() {
  Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
}

void UnregisterAllURLsAndClearMemoryCache() {
  Platform::Current()
      ->GetURLLoaderMockFactory()
      ->UnregisterAllURLsAndClearMemoryCache();
}

void SetUpSuccessURL() {
  URLTestHelpers::RegisterMockedURLLoad(
      SuccessURL(), test::CoreTestDataPath(kFileName), "text/html");
}

void SetUpErrorURL() {
  URLTestHelpers::RegisterMockedErrorURLLoad(ErrorURL());
}

void SetUpRedirectURL() {
  KURL url = RedirectURL();

  WebURLLoadTiming timing;
  timing.Initialize();

  WebURLResponse response;
  response.SetURL(url);
  response.SetHTTPStatusCode(301);
  response.SetLoadTiming(timing);
  response.AddHTTPHeaderField("Location", SuccessURL().GetString());
  response.AddHTTPHeaderField("Access-Control-Allow-Origin", "null");

  URLTestHelpers::RegisterMockedURLLoadWithCustomResponse(
      url, test::CoreTestDataPath(kFileName), response);
}

void SetUpRedirectLoopURL() {
  KURL url = RedirectLoopURL();

  WebURLLoadTiming timing;
  timing.Initialize();

  WebURLResponse response;
  response.SetURL(url);
  response.SetHTTPStatusCode(301);
  response.SetLoadTiming(timing);
  response.AddHTTPHeaderField("Location", RedirectLoopURL().GetString());
  response.AddHTTPHeaderField("Access-Control-Allow-Origin", "null");

  URLTestHelpers::RegisterMockedURLLoadWithCustomResponse(
      url, test::CoreTestDataPath(kFileName), response);
}

void SetUpMockURLs() {
  SetUpSuccessURL();
  SetUpErrorURL();
  SetUpRedirectURL();
  SetUpRedirectLoopURL();
}

enum ThreadableLoaderToTest {
  kDocumentThreadableLoaderTest,
  kWorkerThreadableLoaderTest,
};

class ThreadableLoaderTestHelper {
 public:
  virtual ~ThreadableLoaderTestHelper() = default;

  virtual void CreateLoader(ThreadableLoaderClient*) = 0;
  virtual void StartLoader(const ResourceRequest&) = 0;
  virtual void CancelLoader() = 0;
  virtual void CancelAndClearLoader() = 0;
  virtual void ClearLoader() = 0;
  virtual Checkpoint& GetCheckpoint() = 0;
  virtual void CallCheckpoint(int) = 0;
  virtual void OnSetUp() = 0;
  virtual void OnServeRequests() = 0;
  virtual void OnTearDown() = 0;
};

class DocumentThreadableLoaderTestHelper : public ThreadableLoaderTestHelper {
 public:
  DocumentThreadableLoaderTestHelper()
      : dummy_page_holder_(DummyPageHolder::Create(IntSize(1, 1))) {}

  void CreateLoader(ThreadableLoaderClient* client) override {
    ThreadableLoaderOptions options;
    ResourceLoaderOptions resource_loader_options;
    loader_ = DocumentThreadableLoader::Create(
        *ThreadableLoadingContext::Create(GetDocument()), client, options,
        resource_loader_options);
  }

  void StartLoader(const ResourceRequest& request) override {
    loader_->Start(request);
  }

  void CancelLoader() override { loader_->Cancel(); }
  void CancelAndClearLoader() override {
    loader_->Cancel();
    loader_ = nullptr;
  }
  void ClearLoader() override { loader_ = nullptr; }
  Checkpoint& GetCheckpoint() override { return checkpoint_; }
  void CallCheckpoint(int n) override { checkpoint_.Call(n); }

  void OnSetUp() override { SetUpMockURLs(); }

  void OnServeRequests() override { ServeAsynchronousRequests(); }

  void OnTearDown() override {
    if (loader_) {
      loader_->Cancel();
      loader_ = nullptr;
    }
    UnregisterAllURLsAndClearMemoryCache();
  }

 private:
  Document& GetDocument() { return dummy_page_holder_->GetDocument(); }

  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
  Checkpoint checkpoint_;
  Persistent<DocumentThreadableLoader> loader_;
};

class WebWorkerFetchContextForTest : public WebWorkerFetchContext {
 public:
  WebWorkerFetchContextForTest(KURL site_for_cookies)
      : site_for_cookies_(site_for_cookies.Copy()) {}
  void SetTerminateSyncLoadEvent(base::WaitableEvent*) override {}
  void InitializeOnWorkerThread() override {}

  std::unique_ptr<WebURLLoaderFactory> CreateURLLoaderFactory() override {
    return std::make_unique<WebURLLoaderFactoryWithMock>(
        Platform::Current()->GetURLLoaderMockFactory());
  }
  std::unique_ptr<WebURLLoaderFactory> WrapURLLoaderFactory(
      mojo::ScopedMessagePipeHandle) override {
    return std::make_unique<WebURLLoaderFactoryWithMock>(
        Platform::Current()->GetURLLoaderMockFactory());
  }

  void WillSendRequest(WebURLRequest&) override {}
  bool IsControlledByServiceWorker() const override { return false; }
  WebURL SiteForCookies() const override { return site_for_cookies_; }

 private:
  WebURL site_for_cookies_;

  DISALLOW_COPY_AND_ASSIGN(WebWorkerFetchContextForTest);
};

class WorkerThreadableLoaderTestHelper : public ThreadableLoaderTestHelper {
 public:
  WorkerThreadableLoaderTestHelper()
      : dummy_page_holder_(DummyPageHolder::Create(IntSize(1, 1))) {}

  void CreateLoader(ThreadableLoaderClient* client) override {
    std::unique_ptr<WaitableEvent> completion_event =
        std::make_unique<WaitableEvent>();
    PostCrossThreadTask(
        *worker_loading_task_runner_, FROM_HERE,
        CrossThreadBind(&WorkerThreadableLoaderTestHelper::WorkerCreateLoader,
                        CrossThreadUnretained(this),
                        CrossThreadUnretained(client),
                        CrossThreadUnretained(completion_event.get())));
    completion_event->Wait();
  }

  void StartLoader(const ResourceRequest& request) override {
    std::unique_ptr<WaitableEvent> completion_event =
        std::make_unique<WaitableEvent>();
    PostCrossThreadTask(
        *worker_loading_task_runner_, FROM_HERE,
        CrossThreadBind(&WorkerThreadableLoaderTestHelper::WorkerStartLoader,
                        CrossThreadUnretained(this),
                        CrossThreadUnretained(completion_event.get()),
                        request));
    completion_event->Wait();
  }

  // Must be called on the worker thread.
  void CancelLoader() override {
    DCHECK(worker_thread_);
    DCHECK(worker_thread_->IsCurrentThread());
    loader_->Cancel();
  }

  void CancelAndClearLoader() override {
    DCHECK(worker_thread_);
    DCHECK(worker_thread_->IsCurrentThread());
    loader_->Cancel();
    loader_ = nullptr;
  }

  // Must be called on the worker thread.
  void ClearLoader() override {
    DCHECK(worker_thread_);
    DCHECK(worker_thread_->IsCurrentThread());
    loader_ = nullptr;
  }

  Checkpoint& GetCheckpoint() override { return checkpoint_; }

  void CallCheckpoint(int n) override {
    test::RunPendingTasks();

    std::unique_ptr<WaitableEvent> completion_event =
        std::make_unique<WaitableEvent>();
    PostCrossThreadTask(
        *worker_loading_task_runner_, FROM_HERE,
        CrossThreadBind(&WorkerThreadableLoaderTestHelper::WorkerCallCheckpoint,
                        CrossThreadUnretained(this),
                        CrossThreadUnretained(completion_event.get()), n));
    completion_event->Wait();
  }

  void OnSetUp() override {
    reporting_proxy_ = std::make_unique<WorkerReportingProxy>();
    security_origin_ = GetDocument().GetSecurityOrigin();
    parent_execution_context_task_runners_ =
        ParentExecutionContextTaskRunners::Create(&GetDocument());
    worker_thread_ = std::make_unique<WorkerThreadForTest>(
        ThreadableLoadingContext::Create(GetDocument()), *reporting_proxy_);
    WorkerClients* worker_clients = WorkerClients::Create();

    ProvideWorkerFetchContextToWorker(
        worker_clients, std::make_unique<WebWorkerFetchContextForTest>(
                            GetDocument().SiteForCookies()));
    worker_thread_->StartWithSourceCode(
        security_origin_.get(), "//fake source code",
        parent_execution_context_task_runners_.Get(), GetDocument().Url(),
        worker_clients);
    worker_thread_->WaitForInit();
    worker_loading_task_runner_ =
        worker_thread_->GetTaskRunner(TaskType::kInternalTest);

    PostCrossThreadTask(*worker_loading_task_runner_, FROM_HERE,
                        CrossThreadBind(&SetUpMockURLs));
    WaitForWorkerThreadSignal();
  }

  void OnServeRequests() override {
    test::RunPendingTasks();
    PostCrossThreadTask(*worker_loading_task_runner_, FROM_HERE,
                        CrossThreadBind(&ServeAsynchronousRequests));
    WaitForWorkerThreadSignal();
  }

  void OnTearDown() override {
    PostCrossThreadTask(
        *worker_loading_task_runner_, FROM_HERE,
        CrossThreadBind(&WorkerThreadableLoaderTestHelper::ClearLoader,
                        CrossThreadUnretained(this)));
    WaitForWorkerThreadSignal();
    PostCrossThreadTask(*worker_loading_task_runner_, FROM_HERE,
                        CrossThreadBind(&UnregisterAllURLsAndClearMemoryCache));
    WaitForWorkerThreadSignal();

    worker_thread_->Terminate();
    worker_thread_->WaitForShutdownForTesting();

    // Needed to clean up the things on the main thread side and
    // avoid Resource leaks.
    test::RunPendingTasks();
  }

 private:
  Document& GetDocument() { return dummy_page_holder_->GetDocument(); }

  void WorkerCreateLoader(ThreadableLoaderClient* client,
                          WaitableEvent* event) {
    DCHECK(worker_thread_);
    DCHECK(worker_thread_->IsCurrentThread());

    ThreadableLoaderOptions options;
    ResourceLoaderOptions resource_loader_options;

    // Ensure that WorkerThreadableLoader is created.
    // ThreadableLoader::create() determines whether it should create
    // a DocumentThreadableLoader or WorkerThreadableLoader based on
    // isWorkerGlobalScope().
    DCHECK(worker_thread_->GlobalScope()->IsWorkerGlobalScope());

    loader_ = ThreadableLoader::Create(*worker_thread_->GlobalScope(), client,
                                       options, resource_loader_options);
    DCHECK(loader_);
    event->Signal();
  }

  void WorkerStartLoader(
      WaitableEvent* event,
      std::unique_ptr<CrossThreadResourceRequestData> request_data) {
    DCHECK(worker_thread_);
    DCHECK(worker_thread_->IsCurrentThread());

    ResourceRequest request(request_data.get());
    request.SetFetchCredentialsMode(
        network::mojom::FetchCredentialsMode::kOmit);
    loader_->Start(request);
    event->Signal();
  }

  void WorkerCallCheckpoint(WaitableEvent* event, int n) {
    DCHECK(worker_thread_);
    DCHECK(worker_thread_->IsCurrentThread());
    checkpoint_.Call(n);
    event->Signal();
  }

  void WaitForWorkerThreadSignal() {
    WaitableEvent event;
    PostCrossThreadTask(
        *worker_loading_task_runner_, FROM_HERE,
        CrossThreadBind(&WaitableEvent::Signal, CrossThreadUnretained(&event)));
    event.Wait();
  }

  scoped_refptr<const SecurityOrigin> security_origin_;
  std::unique_ptr<WorkerReportingProxy> reporting_proxy_;
  std::unique_ptr<WorkerThreadForTest> worker_thread_;

  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
  // Accessed cross-thread when worker thread posts tasks to the parent.
  CrossThreadPersistent<ParentExecutionContextTaskRunners>
      parent_execution_context_task_runners_;
  scoped_refptr<base::SingleThreadTaskRunner> worker_loading_task_runner_;
  Checkpoint checkpoint_;
  // |m_loader| must be touched only from the worker thread only.
  CrossThreadPersistent<ThreadableLoader> loader_;
};

class ThreadableLoaderTest
    : public testing::TestWithParam<ThreadableLoaderToTest> {
 public:
  ThreadableLoaderTest() {
    switch (GetParam()) {
      case kDocumentThreadableLoaderTest:
        helper_ = std::make_unique<DocumentThreadableLoaderTestHelper>();
        break;
      case kWorkerThreadableLoaderTest:
        helper_ = std::make_unique<WorkerThreadableLoaderTestHelper>();
        break;
    }
  }

  void StartLoader(const KURL& url,
                   network::mojom::FetchRequestMode fetch_request_mode =
                       network::mojom::FetchRequestMode::kNoCORS) {
    ResourceRequest request(url);
    request.SetRequestContext(WebURLRequest::kRequestContextObject);
    request.SetFetchRequestMode(fetch_request_mode);
    request.SetFetchCredentialsMode(
        network::mojom::FetchCredentialsMode::kOmit);
    helper_->StartLoader(request);
  }

  void CancelLoader() { helper_->CancelLoader(); }
  void CancelAndClearLoader() { helper_->CancelAndClearLoader(); }
  void ClearLoader() { helper_->ClearLoader(); }
  Checkpoint& GetCheckpoint() { return helper_->GetCheckpoint(); }
  void CallCheckpoint(int n) { helper_->CallCheckpoint(n); }

  void ServeRequests() {
    helper_->OnServeRequests();
  }

  void CreateLoader() { helper_->CreateLoader(Client()); }

  MockThreadableLoaderClient* Client() const { return client_.get(); }

 private:
  void SetUp() override {
    client_ = MockThreadableLoaderClient::Create();
    helper_->OnSetUp();
  }

  void TearDown() override {
    helper_->OnTearDown();
    client_.reset();
  }
  std::unique_ptr<MockThreadableLoaderClient> client_;
  std::unique_ptr<ThreadableLoaderTestHelper> helper_;
};

INSTANTIATE_TEST_CASE_P(Document,
                        ThreadableLoaderTest,
                        testing::Values(kDocumentThreadableLoaderTest));

INSTANTIATE_TEST_CASE_P(Worker,
                        ThreadableLoaderTest,
                        testing::Values(kWorkerThreadableLoaderTest));

TEST_P(ThreadableLoaderTest, StartAndStop) {}

TEST_P(ThreadableLoaderTest, CancelAfterStart) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelLoader));
  EXPECT_CALL(*Client(), DidFail(Truly(IsCancellation)));
  EXPECT_CALL(GetCheckpoint(), Call(3));

  StartLoader(SuccessURL());
  CallCheckpoint(2);
  CallCheckpoint(3);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelAndClearAfterStart) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2))
      .WillOnce(
          InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelAndClearLoader));
  EXPECT_CALL(*Client(), DidFail(Truly(IsCancellation)));
  EXPECT_CALL(GetCheckpoint(), Call(3));

  StartLoader(SuccessURL());
  CallCheckpoint(2);
  CallCheckpoint(3);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelInDidReceiveResponse) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelLoader));
  EXPECT_CALL(*Client(), DidFail(Truly(IsCancellation)));

  StartLoader(SuccessURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelAndClearInDidReceiveResponse) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _))
      .WillOnce(
          InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelAndClearLoader));
  EXPECT_CALL(*Client(), DidFail(Truly(IsCancellation)));

  StartLoader(SuccessURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelInDidReceiveData) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidReceiveData(_, _))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelLoader));
  EXPECT_CALL(*Client(), DidFail(Truly(IsCancellation)));

  StartLoader(SuccessURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelAndClearInDidReceiveData) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidReceiveData(_, _))
      .WillOnce(
          InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelAndClearLoader));
  EXPECT_CALL(*Client(), DidFail(Truly(IsCancellation)));

  StartLoader(SuccessURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, DidFinishLoading) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidReceiveData(StrEq("fox"), 4));
  // We expect didReceiveResourceTiming() calls in DocumentThreadableLoader;
  // it's used to connect DocumentThreadableLoader to WorkerThreadableLoader,
  // not to ThreadableLoaderClient.
  EXPECT_CALL(*Client(), DidReceiveResourceTiming(_));
  EXPECT_CALL(*Client(), DidFinishLoading(_));

  StartLoader(SuccessURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelInDidFinishLoading) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidReceiveData(_, _));
  EXPECT_CALL(*Client(), DidReceiveResourceTiming(_));
  EXPECT_CALL(*Client(), DidFinishLoading(_))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelLoader));

  StartLoader(SuccessURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, ClearInDidFinishLoading) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidReceiveData(_, _));
  EXPECT_CALL(*Client(), DidReceiveResourceTiming(_));
  EXPECT_CALL(*Client(), DidFinishLoading(_))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::ClearLoader));

  StartLoader(SuccessURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, DidFail) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidFail(Truly(IsNotCancellation)));

  StartLoader(ErrorURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelInDidFail) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidFail(_))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelLoader));

  StartLoader(ErrorURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, ClearInDidFail) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidFail(_))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::ClearLoader));

  StartLoader(ErrorURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, DidFailInStart) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  String error_message = String::Format(
      "Failed to load '%s': Cross origin requests are not allowed by request "
      "mode.",
      ErrorURL().GetString().Utf8().data());
  EXPECT_CALL(*Client(), DidFail(ResourceError::CancelledDueToAccessCheckError(
                             ErrorURL(), ResourceRequestBlockedReason::kOther,
                             error_message)));
  EXPECT_CALL(GetCheckpoint(), Call(2));

  StartLoader(ErrorURL(), network::mojom::FetchRequestMode::kSameOrigin);
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelInDidFailInStart) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(*Client(), DidFail(_))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelLoader));
  EXPECT_CALL(GetCheckpoint(), Call(2));

  StartLoader(ErrorURL(), network::mojom::FetchRequestMode::kSameOrigin);
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, ClearInDidFailInStart) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(*Client(), DidFail(_))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::ClearLoader));
  EXPECT_CALL(GetCheckpoint(), Call(2));

  StartLoader(ErrorURL(), network::mojom::FetchRequestMode::kSameOrigin);
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, DidFailAccessControlCheck) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(
      *Client(),
      DidFail(ResourceError::CancelledDueToAccessCheckError(
          SuccessURL(), ResourceRequestBlockedReason::kOther,
          "No 'Access-Control-Allow-Origin' header is present on the requested "
          "resource. Origin 'null' is therefore not allowed access.")));

  StartLoader(SuccessURL(), network::mojom::FetchRequestMode::kCORS);
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, RedirectDidFinishLoading) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidReceiveData(StrEq("fox"), 4));
  EXPECT_CALL(*Client(), DidReceiveResourceTiming(_));
  EXPECT_CALL(*Client(), DidFinishLoading(_));

  StartLoader(RedirectURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelInRedirectDidFinishLoading) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidReceiveData(StrEq("fox"), 4));
  EXPECT_CALL(*Client(), DidReceiveResourceTiming(_));
  EXPECT_CALL(*Client(), DidFinishLoading(_))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelLoader));

  StartLoader(RedirectURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, ClearInRedirectDidFinishLoading) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidReceiveResponseMock(_, _, _));
  EXPECT_CALL(*Client(), DidReceiveData(StrEq("fox"), 4));
  EXPECT_CALL(*Client(), DidReceiveResourceTiming(_));
  EXPECT_CALL(*Client(), DidFinishLoading(_))
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::ClearLoader));

  StartLoader(RedirectURL());
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, DidFailRedirectCheck) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidFailRedirectCheck());

  StartLoader(RedirectLoopURL(), network::mojom::FetchRequestMode::kCORS);
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, CancelInDidFailRedirectCheck) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidFailRedirectCheck())
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::CancelLoader));

  StartLoader(RedirectLoopURL(), network::mojom::FetchRequestMode::kCORS);
  CallCheckpoint(2);
  ServeRequests();
}

TEST_P(ThreadableLoaderTest, ClearInDidFailRedirectCheck) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(GetCheckpoint(), Call(2));
  EXPECT_CALL(*Client(), DidFailRedirectCheck())
      .WillOnce(InvokeWithoutArgs(this, &ThreadableLoaderTest::ClearLoader));

  StartLoader(RedirectLoopURL(), network::mojom::FetchRequestMode::kCORS);
  CallCheckpoint(2);
  ServeRequests();
}

// This test case checks blink doesn't crash even when the response arrives
// synchronously.
TEST_P(ThreadableLoaderTest, GetResponseSynchronously) {
  InSequence s;
  EXPECT_CALL(GetCheckpoint(), Call(1));
  CreateLoader();
  CallCheckpoint(1);

  EXPECT_CALL(*Client(), DidFail(_));
  EXPECT_CALL(GetCheckpoint(), Call(2));

  // Currently didFailAccessControlCheck is dispatched synchronously. This
  // test is not saying that didFailAccessControlCheck should be dispatched
  // synchronously, but is saying that even when a response is served
  // synchronously it should not lead to a crash.
  StartLoader(KURL("about:blank"), network::mojom::FetchRequestMode::kCORS);
  CallCheckpoint(2);
}

}  // namespace

}  // namespace blink
