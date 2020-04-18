// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/throttling_url_loader.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/url_loader_throttle.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

GURL request_url = GURL("http://example.org");
GURL redirect_url = GURL("http://example.com");

class TestURLLoaderFactory : public network::mojom::URLLoaderFactory,
                             public network::mojom::URLLoader {
 public:
  TestURLLoaderFactory() : binding_(this), url_loader_binding_(this) {
    binding_.Bind(mojo::MakeRequest(&factory_ptr_));
    shared_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            factory_ptr_.get());
  }

  ~TestURLLoaderFactory() override { shared_factory_->Detach(); }

  network::mojom::URLLoaderFactoryPtr& factory_ptr() { return factory_ptr_; }
  network::mojom::URLLoaderClientPtr& client_ptr() { return client_ptr_; }
  mojo::Binding<network::mojom::URLLoader>& url_loader_binding() {
    return url_loader_binding_;
  }
  scoped_refptr<network::SharedURLLoaderFactory> shared_factory() {
    return shared_factory_;
  }

  size_t create_loader_and_start_called() const {
    return create_loader_and_start_called_;
  }

  size_t pause_reading_body_from_net_called() const {
    return pause_reading_body_from_net_called_;
  }

  size_t resume_reading_body_from_net_called() const {
    return resume_reading_body_from_net_called_;
  }

  void NotifyClientOnReceiveResponse() {
    client_ptr_->OnReceiveResponse(network::ResourceResponseHead(), nullptr);
  }

  void NotifyClientOnReceiveRedirect() {
    net::RedirectInfo info;
    info.new_url = redirect_url;
    client_ptr_->OnReceiveRedirect(info, network::ResourceResponseHead());
  }

  void NotifyClientOnComplete(int error_code) {
    network::URLLoaderCompletionStatus data;
    data.error_code = error_code;
    client_ptr_->OnComplete(data);
  }

  void CloseClientPipe() { client_ptr_.reset(); }

 private:
  // network::mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& url_request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    create_loader_and_start_called_++;

    DCHECK(!url_loader_binding_.is_bound())
        << "TestURLLoaderFactory is not able to handle multiple requests.";
    url_loader_binding_.Bind(std::move(request));
    client_ptr_ = std::move(client);
  }

  void Clone(network::mojom::URLLoaderFactoryRequest request) override {
    NOTREACHED();
  }

  // network::mojom::URLLoader implementation.
  void FollowRedirect(const base::Optional<net::HttpRequestHeaders>&
                          modified_request_headers) override {}
  void ProceedWithResponse() override {}

  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}

  void PauseReadingBodyFromNet() override {
    pause_reading_body_from_net_called_++;
  }

  void ResumeReadingBodyFromNet() override {
    resume_reading_body_from_net_called_++;
  }

  size_t create_loader_and_start_called_ = 0;
  size_t pause_reading_body_from_net_called_ = 0;
  size_t resume_reading_body_from_net_called_ = 0;

  mojo::Binding<network::mojom::URLLoaderFactory> binding_;
  mojo::Binding<network::mojom::URLLoader> url_loader_binding_;
  network::mojom::URLLoaderFactoryPtr factory_ptr_;
  network::mojom::URLLoaderClientPtr client_ptr_;
  scoped_refptr<network::WeakWrapperSharedURLLoaderFactory> shared_factory_;
  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderFactory);
};

class TestURLLoaderClient : public network::mojom::URLLoaderClient {
 public:
  TestURLLoaderClient() {}

  size_t on_received_response_called() const {
    return on_received_response_called_;
  }

  size_t on_received_redirect_called() const {
    return on_received_redirect_called_;
  }

  size_t on_complete_called() const { return on_complete_called_; }

  void set_on_received_redirect_callback(
      const base::RepeatingClosure& callback) {
    on_received_redirect_callback_ = callback;
  }

  void set_on_received_response_callback(
      const base::RepeatingClosure& callback) {
    on_received_response_callback_ = callback;
  }

  using OnCompleteCallback = base::Callback<void(int error_code)>;
  void set_on_complete_callback(const OnCompleteCallback& callback) {
    on_complete_callback_ = callback;
  }

 private:
  // network::mojom::URLLoaderClient implementation:
  void OnReceiveResponse(
      const network::ResourceResponseHead& response_head,
      network::mojom::DownloadedTempFilePtr downloaded_file) override {
    on_received_response_called_++;
    if (on_received_response_callback_)
      on_received_response_callback_.Run();
  }
  void OnReceiveRedirect(
      const net::RedirectInfo& redirect_info,
      const network::ResourceResponseHead& response_head) override {
    on_received_redirect_called_++;
    if (on_received_redirect_callback_)
      on_received_redirect_callback_.Run();
  }
  void OnDataDownloaded(int64_t data_len, int64_t encoded_data_len) override {}
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override {}
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override {}
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override {}
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override {}
  void OnComplete(const network::URLLoaderCompletionStatus& status) override {
    on_complete_called_++;
    if (on_complete_callback_)
      on_complete_callback_.Run(status.error_code);
  }

  size_t on_received_response_called_ = 0;
  size_t on_received_redirect_called_ = 0;
  size_t on_complete_called_ = 0;

  base::RepeatingClosure on_received_redirect_callback_;
  base::RepeatingClosure on_received_response_callback_;
  OnCompleteCallback on_complete_callback_;

  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderClient);
};

class TestURLLoaderThrottle : public URLLoaderThrottle {
 public:
  TestURLLoaderThrottle() {}
  explicit TestURLLoaderThrottle(const base::Closure& destruction_notifier)
      : destruction_notifier_(destruction_notifier) {}

  ~TestURLLoaderThrottle() override {
    if (destruction_notifier_)
      destruction_notifier_.Run();
  }

  using ThrottleCallback =
      base::Callback<void(URLLoaderThrottle::Delegate* delegate, bool* defer)>;

  size_t will_start_request_called() const {
    return will_start_request_called_;
  }
  size_t will_redirect_request_called() const {
    return will_redirect_request_called_;
  }
  size_t will_process_response_called() const {
    return will_process_response_called_;
  }

  GURL observed_response_url() const { return response_url_; }

  void set_will_start_request_callback(const ThrottleCallback& callback) {
    will_start_request_callback_ = callback;
  }

  void set_will_redirect_request_callback(const ThrottleCallback& callback) {
    will_redirect_request_callback_ = callback;
  }

  void set_will_process_response_callback(const ThrottleCallback& callback) {
    will_process_response_callback_ = callback;
  }

  Delegate* delegate() const { return delegate_; }

 private:
  // URLLoaderThrottle implementation.
  void WillStartRequest(network::ResourceRequest* request,
                        bool* defer) override {
    will_start_request_called_++;
    if (will_start_request_callback_)
      will_start_request_callback_.Run(delegate_, defer);
  }

  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           const network::ResourceResponseHead& response_head,
                           bool* defer) override {
    will_redirect_request_called_++;
    if (will_redirect_request_callback_)
      will_redirect_request_callback_.Run(delegate_, defer);
  }

  void WillProcessResponse(const GURL& response_url,
                           const network::ResourceResponseHead& response_head,
                           bool* defer) override {
    will_process_response_called_++;
    if (will_process_response_callback_)
      will_process_response_callback_.Run(delegate_, defer);
    response_url_ = response_url;
  }

  size_t will_start_request_called_ = 0;
  size_t will_redirect_request_called_ = 0;
  size_t will_process_response_called_ = 0;

  GURL response_url_;

  ThrottleCallback will_start_request_callback_;
  ThrottleCallback will_redirect_request_callback_;
  ThrottleCallback will_process_response_callback_;

  base::Closure destruction_notifier_;

  DISALLOW_COPY_AND_ASSIGN(TestURLLoaderThrottle);
};

class ThrottlingURLLoaderTest : public testing::Test {
 public:
  ThrottlingURLLoaderTest() : weak_factory_(this) {}

  std::unique_ptr<ThrottlingURLLoader>& loader() { return loader_; }
  TestURLLoaderThrottle* throttle() const { return throttle_; }

 protected:
  // testing::Test implementation.
  void SetUp() override {
    auto throttle = std::make_unique<TestURLLoaderThrottle>(
        base::Bind(&ThrottlingURLLoaderTest::ResetThrottleRawPointer,
                   weak_factory_.GetWeakPtr()));

    throttle_ = throttle.get();

    throttles_.push_back(std::move(throttle));
  }

  void CreateLoaderAndStart(bool sync = false) {
    uint32_t options = 0;
    if (sync)
      options |= network::mojom::kURLLoadOptionSynchronous;
    network::ResourceRequest request;
    request.url = request_url;
    loader_ = ThrottlingURLLoader::CreateLoaderAndStart(
        factory_.shared_factory(), std::move(throttles_), 0, 0, options,
        &request, &client_, TRAFFIC_ANNOTATION_FOR_TESTS,
        base::ThreadTaskRunnerHandle::Get());
    factory_.factory_ptr().FlushForTesting();
  }

  void ResetThrottleRawPointer() { throttle_ = nullptr; }

  // Be the first member so it is destroyed last.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<ThrottlingURLLoader> loader_;
  std::vector<std::unique_ptr<URLLoaderThrottle>> throttles_;

  TestURLLoaderFactory factory_;
  TestURLLoaderClient client_;

  // Owned by |throttles_| or |loader_|.
  TestURLLoaderThrottle* throttle_ = nullptr;

  base::WeakPtrFactory<ThrottlingURLLoaderTest> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThrottlingURLLoaderTest);
};

TEST_F(ThrottlingURLLoaderTest, CancelBeforeStart) {
  throttle_->set_will_start_request_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        delegate->CancelWithError(net::ERR_ACCESS_DENIED);
      }));

  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::ERR_ACCESS_DENIED, error);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart();
  run_loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());

  EXPECT_EQ(0u, factory_.create_loader_and_start_called());

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, DeferBeforeStart) {
  throttle_->set_will_start_request_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        *defer = true;
      }));

  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::OK, error);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());

  EXPECT_EQ(0u, factory_.create_loader_and_start_called());

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(0u, client_.on_complete_called());

  throttle_->delegate()->Resume();
  factory_.factory_ptr().FlushForTesting();

  EXPECT_EQ(1u, factory_.create_loader_and_start_called());

  factory_.NotifyClientOnReceiveResponse();
  factory_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(1u, throttle_->will_process_response_called());

  EXPECT_TRUE(
      throttle_->observed_response_url().EqualsIgnoringRef(request_url));

  EXPECT_EQ(1u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, CancelBeforeRedirect) {
  throttle_->set_will_redirect_request_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        delegate->CancelWithError(net::ERR_ACCESS_DENIED);
      }));

  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::ERR_ACCESS_DENIED, error);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart();

  factory_.NotifyClientOnReceiveRedirect();

  run_loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, DeferBeforeRedirect) {
  base::RunLoop run_loop1;
  throttle_->set_will_redirect_request_callback(base::Bind(
      [](const base::Closure& quit_closure,
         URLLoaderThrottle::Delegate* delegate, bool* defer) {
        *defer = true;
        quit_closure.Run();
      },
      run_loop1.QuitClosure()));

  base::RunLoop run_loop2;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::ERR_UNEXPECTED, error);
        quit_closure.Run();
      },
      run_loop2.QuitClosure()));

  CreateLoaderAndStart();

  factory_.NotifyClientOnReceiveRedirect();

  run_loop1.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());

  factory_.NotifyClientOnComplete(net::ERR_UNEXPECTED);

  base::RunLoop run_loop3;
  run_loop3.RunUntilIdle();

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(0u, client_.on_complete_called());

  throttle_->delegate()->Resume();
  run_loop2.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(1u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, CancelBeforeResponse) {
  throttle_->set_will_process_response_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        delegate->CancelWithError(net::ERR_ACCESS_DENIED);
      }));

  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::ERR_ACCESS_DENIED, error);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart();

  factory_.NotifyClientOnReceiveResponse();

  run_loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(1u, throttle_->will_process_response_called());

  EXPECT_TRUE(
      throttle_->observed_response_url().EqualsIgnoringRef(request_url));

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, DeferBeforeResponse) {
  base::RunLoop run_loop1;
  throttle_->set_will_process_response_callback(base::Bind(
      [](const base::Closure& quit_closure,
         URLLoaderThrottle::Delegate* delegate, bool* defer) {
        *defer = true;
        quit_closure.Run();
      },
      run_loop1.QuitClosure()));

  base::RunLoop run_loop2;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::ERR_UNEXPECTED, error);
        quit_closure.Run();
      },
      run_loop2.QuitClosure()));

  CreateLoaderAndStart();

  factory_.NotifyClientOnReceiveResponse();

  run_loop1.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(1u, throttle_->will_process_response_called());

  EXPECT_TRUE(
      throttle_->observed_response_url().EqualsIgnoringRef(request_url));

  factory_.NotifyClientOnComplete(net::ERR_UNEXPECTED);

  base::RunLoop run_loop3;
  run_loop3.RunUntilIdle();

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(0u, client_.on_complete_called());

  throttle_->delegate()->Resume();
  run_loop2.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(1u, throttle_->will_process_response_called());

  EXPECT_TRUE(
      throttle_->observed_response_url().EqualsIgnoringRef(request_url));

  EXPECT_EQ(1u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, PipeClosureBeforeSyncResponse) {
  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::ERR_ABORTED, error);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart(true);

  factory_.CloseClientPipe();

  run_loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

// Once browser-side navigation is the only option these two tests should be
// merged as the sync and async cases will be identical.
TEST_F(ThrottlingURLLoaderTest, PipeClosureBeforeAsyncResponse) {
  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::ERR_ABORTED, error);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart();

  factory_.CloseClientPipe();

  run_loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, ResumeNoOpIfNotDeferred) {
  auto resume_callback =
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        delegate->Resume();
        delegate->Resume();
      });
  throttle_->set_will_start_request_callback(resume_callback);
  throttle_->set_will_redirect_request_callback(resume_callback);
  throttle_->set_will_process_response_callback(std::move(resume_callback));

  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::OK, error);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart();
  factory_.NotifyClientOnReceiveRedirect();
  factory_.NotifyClientOnReceiveResponse();
  factory_.NotifyClientOnComplete(net::OK);

  run_loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle_->will_redirect_request_called());
  EXPECT_EQ(1u, throttle_->will_process_response_called());

  EXPECT_TRUE(
      throttle_->observed_response_url().EqualsIgnoringRef(redirect_url));

  EXPECT_EQ(1u, client_.on_received_response_called());
  EXPECT_EQ(1u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, CancelNoOpIfAlreadyCanceled) {
  throttle_->set_will_start_request_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        delegate->CancelWithError(net::ERR_ACCESS_DENIED);
        delegate->CancelWithError(net::ERR_UNEXPECTED);
      }));

  base::RunLoop run_loop;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::ERR_ACCESS_DENIED, error);
        quit_closure.Run();
      },
      run_loop.QuitClosure()));

  CreateLoaderAndStart();
  throttle_->delegate()->CancelWithError(net::ERR_INVALID_ARGUMENT);
  run_loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());

  EXPECT_EQ(0u, factory_.create_loader_and_start_called());

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, ResumeNoOpIfAlreadyCanceled) {
  throttle_->set_will_process_response_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        delegate->CancelWithError(net::ERR_ACCESS_DENIED);
        delegate->Resume();
      }));

  base::RunLoop run_loop1;
  client_.set_on_complete_callback(base::Bind(
      [](const base::Closure& quit_closure, int error) {
        EXPECT_EQ(net::ERR_ACCESS_DENIED, error);
        quit_closure.Run();
      },
      run_loop1.QuitClosure()));

  CreateLoaderAndStart();

  factory_.NotifyClientOnReceiveResponse();

  run_loop1.Run();

  throttle_->delegate()->Resume();

  base::RunLoop run_loop2;
  run_loop2.RunUntilIdle();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(1u, throttle_->will_process_response_called());

  EXPECT_TRUE(
      throttle_->observed_response_url().EqualsIgnoringRef(request_url));

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, MultipleThrottlesBasicSupport) {
  throttles_.emplace_back(std::make_unique<TestURLLoaderThrottle>());
  auto* throttle2 =
      static_cast<TestURLLoaderThrottle*>(throttles_.back().get());
  CreateLoaderAndStart();
  factory_.NotifyClientOnReceiveResponse();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle2->will_start_request_called());
}

TEST_F(ThrottlingURLLoaderTest, BlockWithOneOfMultipleThrottles) {
  throttles_.emplace_back(std::make_unique<TestURLLoaderThrottle>());
  auto* throttle2 =
      static_cast<TestURLLoaderThrottle*>(throttles_.back().get());
  throttle2->set_will_start_request_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        *defer = true;
      }));

  base::RunLoop loop;
  client_.set_on_complete_callback(base::Bind(
      [](base::RunLoop* loop, int error) {
        EXPECT_EQ(net::OK, error);
        loop->Quit();
      },
      &loop));

  CreateLoaderAndStart();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle2->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle2->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());
  EXPECT_EQ(0u, throttle2->will_process_response_called());

  EXPECT_EQ(0u, factory_.create_loader_and_start_called());

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(0u, client_.on_complete_called());

  throttle2->delegate()->Resume();
  factory_.factory_ptr().FlushForTesting();

  EXPECT_EQ(1u, factory_.create_loader_and_start_called());

  factory_.NotifyClientOnReceiveResponse();
  factory_.NotifyClientOnComplete(net::OK);

  loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle2->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle2->will_redirect_request_called());
  EXPECT_EQ(1u, throttle_->will_process_response_called());
  EXPECT_EQ(1u, throttle2->will_process_response_called());

  EXPECT_TRUE(
      throttle_->observed_response_url().EqualsIgnoringRef(request_url));
  EXPECT_TRUE(
      throttle2->observed_response_url().EqualsIgnoringRef(request_url));

  EXPECT_EQ(1u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, BlockWithMultipleThrottles) {
  throttles_.emplace_back(std::make_unique<TestURLLoaderThrottle>());
  auto* throttle2 =
      static_cast<TestURLLoaderThrottle*>(throttles_.back().get());

  // Defers a request on both throttles.
  throttle_->set_will_start_request_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        *defer = true;
      }));
  throttle2->set_will_start_request_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        *defer = true;
      }));

  base::RunLoop loop;
  client_.set_on_complete_callback(base::Bind(
      [](base::RunLoop* loop, int error) {
        EXPECT_EQ(net::OK, error);
        loop->Quit();
      },
      &loop));

  CreateLoaderAndStart();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle2->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle2->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());
  EXPECT_EQ(0u, throttle2->will_process_response_called());

  EXPECT_EQ(0u, factory_.create_loader_and_start_called());

  EXPECT_EQ(0u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(0u, client_.on_complete_called());

  throttle_->delegate()->Resume();

  // Should still not have started because there's |throttle2| is still blocking
  // the request.
  factory_.factory_ptr().FlushForTesting();
  EXPECT_EQ(0u, factory_.create_loader_and_start_called());

  throttle2->delegate()->Resume();

  // Now it should have started.
  factory_.factory_ptr().FlushForTesting();
  EXPECT_EQ(1u, factory_.create_loader_and_start_called());

  factory_.NotifyClientOnReceiveResponse();
  factory_.NotifyClientOnComplete(net::OK);

  loop.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle2->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle2->will_redirect_request_called());
  EXPECT_EQ(1u, throttle_->will_process_response_called());
  EXPECT_EQ(1u, throttle2->will_process_response_called());

  EXPECT_TRUE(
      throttle_->observed_response_url().EqualsIgnoringRef(request_url));
  EXPECT_TRUE(
      throttle2->observed_response_url().EqualsIgnoringRef(request_url));

  EXPECT_EQ(1u, client_.on_received_response_called());
  EXPECT_EQ(0u, client_.on_received_redirect_called());
  EXPECT_EQ(1u, client_.on_complete_called());
}

TEST_F(ThrottlingURLLoaderTest, PauseResumeReadingBodyFromNet) {
  throttles_.emplace_back(std::make_unique<TestURLLoaderThrottle>());
  auto* throttle2 =
      static_cast<TestURLLoaderThrottle*>(throttles_.back().get());

  // Test that it is okay to call delegate->PauseReadingBodyFromNet() even
  // before the loader is created.
  throttle_->set_will_start_request_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        delegate->PauseReadingBodyFromNet();
        *defer = true;
      }));
  throttle2->set_will_start_request_callback(
      base::Bind([](URLLoaderThrottle::Delegate* delegate, bool* defer) {
        delegate->PauseReadingBodyFromNet();
      }));

  CreateLoaderAndStart();

  throttle_->delegate()->Resume();

  factory_.factory_ptr().FlushForTesting();
  EXPECT_EQ(1u, factory_.create_loader_and_start_called());

  // Make sure all URLLoader calls before this point are delivered to the impl
  // side.
  factory_.url_loader_binding().FlushForTesting();

  // Although there were two calls to delegate->PauseReadingBodyFromNet(), only
  // one URLLoader::PauseReadingBodyFromNet() Mojo call was made.
  EXPECT_EQ(1u, factory_.pause_reading_body_from_net_called());
  EXPECT_EQ(0u, factory_.resume_reading_body_from_net_called());

  // Reading body from network is still paused by |throttle2|. Calling
  // ResumeReadingBodyFromNet() on |throttle_| shouldn't have any effect.
  throttle_->delegate()->ResumeReadingBodyFromNet();
  factory_.url_loader_binding().FlushForTesting();
  EXPECT_EQ(1u, factory_.pause_reading_body_from_net_called());
  EXPECT_EQ(0u, factory_.resume_reading_body_from_net_called());

  // Even if we call ResumeReadingBodyFromNet() on |throttle_| one more time.
  throttle_->delegate()->ResumeReadingBodyFromNet();
  factory_.url_loader_binding().FlushForTesting();
  EXPECT_EQ(1u, factory_.pause_reading_body_from_net_called());
  EXPECT_EQ(0u, factory_.resume_reading_body_from_net_called());

  throttle2->delegate()->ResumeReadingBodyFromNet();
  factory_.url_loader_binding().FlushForTesting();
  EXPECT_EQ(1u, factory_.pause_reading_body_from_net_called());
  EXPECT_EQ(1u, factory_.resume_reading_body_from_net_called());
}

TEST_F(ThrottlingURLLoaderTest,
       DestroyingThrottlingURLLoaderInDelegateCall_Response) {
  base::RunLoop run_loop1;
  throttle_->set_will_process_response_callback(base::Bind(
      [](const base::Closure& quit_closure,
         URLLoaderThrottle::Delegate* delegate, bool* defer) {
        *defer = true;
        quit_closure.Run();
      },
      run_loop1.QuitClosure()));

  base::RunLoop run_loop2;
  client_.set_on_received_response_callback(base::Bind(
      [](ThrottlingURLLoaderTest* test, const base::Closure& quit_closure) {
        // Destroy the ThrottlingURLLoader while inside a delegate call from a
        // throttle.
        test->loader().reset();

        // The throttle should stay alive.
        EXPECT_NE(nullptr, test->throttle());

        quit_closure.Run();
      },
      base::Unretained(this), run_loop2.QuitClosure()));

  CreateLoaderAndStart();

  factory_.NotifyClientOnReceiveResponse();

  run_loop1.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(0u, throttle_->will_redirect_request_called());
  EXPECT_EQ(1u, throttle_->will_process_response_called());

  EXPECT_TRUE(
      throttle_->observed_response_url().EqualsIgnoringRef(request_url));

  throttle_->delegate()->Resume();
  run_loop2.Run();

  // The ThrottlingURLLoader should be gone.
  EXPECT_EQ(nullptr, loader_);
  // The throttle should stay alive and destroyed later.
  EXPECT_NE(nullptr, throttle_);

  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(nullptr, throttle_);
}

// Regression test for crbug.com/833292.
TEST_F(ThrottlingURLLoaderTest,
       DestroyingThrottlingURLLoaderInDelegateCall_Redirect) {
  base::RunLoop run_loop1;
  throttle_->set_will_redirect_request_callback(base::BindRepeating(
      [](const base::RepeatingClosure& quit_closure,
         URLLoaderThrottle::Delegate* delegate, bool* defer) {
        *defer = true;
        quit_closure.Run();
      },
      run_loop1.QuitClosure()));

  base::RunLoop run_loop2;
  client_.set_on_received_redirect_callback(base::BindRepeating(
      [](ThrottlingURLLoaderTest* test,
         const base::RepeatingClosure& quit_closure) {
        // Destroy the ThrottlingURLLoader while inside a delegate call from a
        // throttle.
        test->loader().reset();

        // The throttle should stay alive.
        EXPECT_NE(nullptr, test->throttle());

        quit_closure.Run();
      },
      base::Unretained(this), run_loop2.QuitClosure()));

  CreateLoaderAndStart();

  factory_.NotifyClientOnReceiveRedirect();

  run_loop1.Run();

  EXPECT_EQ(1u, throttle_->will_start_request_called());
  EXPECT_EQ(1u, throttle_->will_redirect_request_called());
  EXPECT_EQ(0u, throttle_->will_process_response_called());

  throttle_->delegate()->Resume();
  run_loop2.Run();

  // The ThrottlingURLLoader should be gone.
  EXPECT_EQ(nullptr, loader_);
  // The throttle should stay alive and destroyed later.
  EXPECT_NE(nullptr, throttle_);

  scoped_task_environment_.RunUntilIdle();
  EXPECT_EQ(nullptr, throttle_);
}

}  // namespace
}  // namespace content
