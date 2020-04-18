// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/download_protection/two_phase_uploader.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "chrome/browser/safe_browsing/local_two_phase_testserver.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/base/net_errors.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "services/network/network_context.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

using content::BrowserThread;
using content::MessageLoopRunner;

namespace safe_browsing {

namespace {

class Delegate {
 public:
  Delegate() : state_(TwoPhaseUploader::STATE_NONE) {}

  void FinishCallback(scoped_refptr<MessageLoopRunner> runner,
                      TwoPhaseUploader::State state,
                      int net_error,
                      int response_code,
                      const std::string& response) {
    state_ = state;
    net_error_ = net_error;
    response_code_ = response_code;
    response_ = response;
    runner->Quit();
  }

  TwoPhaseUploader::State state_;
  int net_error_;
  int response_code_;
  std::string response_;
};

base::FilePath GetTestFilePath() {
  base::FilePath file_path;
  base::PathService::Get(base::DIR_SOURCE_ROOT, &file_path);
  file_path = file_path.AppendASCII("net");
  file_path = file_path.AppendASCII("data");
  file_path = file_path.AppendASCII("url_request_unittest");
  file_path = file_path.AppendASCII("BullRunSpeech.txt");
  return file_path;
}

class SharedURLLoaderFactory : public network::SharedURLLoaderFactory {
 public:
  explicit SharedURLLoaderFactory(
      network::mojom::URLLoaderFactory* url_loader_factory)
      : url_loader_factory_(url_loader_factory) {}

  // network::URLLoaderFactory implementation:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override {
    url_loader_factory_->CreateLoaderAndStart(
        std::move(loader), routing_id, request_id, options, std::move(request),
        std::move(client), traffic_annotation);
  }

  void Clone(network::mojom::URLLoaderFactoryRequest request) override {
    NOTREACHED();
  }

  // network::SharedURLLoaderFactoryInfo implementation
  std::unique_ptr<network::SharedURLLoaderFactoryInfo> Clone() override {
    NOTREACHED();
    return nullptr;
  }

 private:
  friend class base::RefCounted<SharedURLLoaderFactory>;
  ~SharedURLLoaderFactory() override = default;

  network::mojom::URLLoaderFactory* url_loader_factory_;
};

}  // namespace

class TwoPhaseUploaderTest : public testing::Test {
 public:
  TwoPhaseUploaderTest()
      : thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        url_request_context_(std::make_unique<net::TestURLRequestContext>()) {
    network::mojom::NetworkContextPtr network_context;
    network_context_ = std::make_unique<network::NetworkContext>(
        nullptr, mojo::MakeRequest(&network_context),
        url_request_context_.get());
    network::mojom::URLLoaderFactoryParamsPtr params =
        network::mojom::URLLoaderFactoryParams::New();
    params->process_id = network::mojom::kBrowserProcessId;
    params->is_corb_enabled = false;
    network_context_->CreateURLLoaderFactory(
        mojo::MakeRequest(&url_loader_factory_), std::move(params));
    shared_url_loader_factory_ =
        base::MakeRefCounted<SharedURLLoaderFactory>(url_loader_factory_.get());
  }

 protected:
  content::TestBrowserThreadBundle thread_bundle_;

  std::unique_ptr<net::TestURLRequestContext> url_request_context_;
  const scoped_refptr<base::SequencedTaskRunner> task_runner_ =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});
  std::unique_ptr<network::NetworkContext> network_context_;
  network::mojom::URLLoaderFactoryPtr url_loader_factory_;
  scoped_refptr<SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(TwoPhaseUploaderTest, UploadFile) {
  scoped_refptr<MessageLoopRunner> runner = new MessageLoopRunner;
  LocalTwoPhaseTestServer test_server;
  ASSERT_TRUE(test_server.Start());
  Delegate delegate;
  std::unique_ptr<TwoPhaseUploader> uploader(TwoPhaseUploader::Create(
      shared_url_loader_factory_, task_runner_.get(),
      test_server.GetURL("start"), "metadata", GetTestFilePath(),
      base::Bind(&Delegate::FinishCallback, base::Unretained(&delegate),
                 runner),
      TRAFFIC_ANNOTATION_FOR_TESTS));
  uploader->Start();
  runner->Run();
  EXPECT_EQ(TwoPhaseUploader::STATE_SUCCESS, delegate.state_);
  EXPECT_EQ(net::OK, delegate.net_error_);
  EXPECT_EQ(200, delegate.response_code_);
  EXPECT_EQ(
      "/start\n"                                     // path of start request
      "4c24b2612e94e2ae622e54397663f2b7bf0a2e17\n"   // sha1sum of "metadata"
      "944857cc626f2cafe232521986b4c6d3f9993c97\n",  // sha1sum of test file
      delegate.response_);
}

TEST_F(TwoPhaseUploaderTest, BadPhaseOneResponse) {
  scoped_refptr<MessageLoopRunner> runner = new MessageLoopRunner;
  LocalTwoPhaseTestServer test_server;
  ASSERT_TRUE(test_server.Start());
  Delegate delegate;
  std::unique_ptr<TwoPhaseUploader> uploader(TwoPhaseUploader::Create(
      shared_url_loader_factory_, task_runner_.get(),
      test_server.GetURL("start?p1code=500"), "metadata", GetTestFilePath(),
      base::Bind(&Delegate::FinishCallback, base::Unretained(&delegate),
                 runner),
      TRAFFIC_ANNOTATION_FOR_TESTS));
  uploader->Start();
  runner->Run();
  EXPECT_EQ(TwoPhaseUploader::UPLOAD_METADATA, delegate.state_);
  EXPECT_EQ(net::OK, delegate.net_error_);
  EXPECT_EQ(500, delegate.response_code_);
  EXPECT_EQ("", delegate.response_);
}

TEST_F(TwoPhaseUploaderTest, BadPhaseTwoResponse) {
  scoped_refptr<MessageLoopRunner> runner = new MessageLoopRunner;
  LocalTwoPhaseTestServer test_server;
  ASSERT_TRUE(test_server.Start());
  Delegate delegate;
  std::unique_ptr<TwoPhaseUploader> uploader(TwoPhaseUploader::Create(
      shared_url_loader_factory_, task_runner_.get(),
      test_server.GetURL("start?p2code=500"), "metadata", GetTestFilePath(),
      base::Bind(&Delegate::FinishCallback, base::Unretained(&delegate),
                 runner),
      TRAFFIC_ANNOTATION_FOR_TESTS));
  uploader->Start();
  runner->Run();
  EXPECT_EQ(TwoPhaseUploader::UPLOAD_FILE, delegate.state_);
  EXPECT_EQ(net::OK, delegate.net_error_);
  EXPECT_EQ(500, delegate.response_code_);
  EXPECT_EQ(
      "/start\n"                                     // path of start request
      "4c24b2612e94e2ae622e54397663f2b7bf0a2e17\n"   // sha1sum of "metadata"
      "944857cc626f2cafe232521986b4c6d3f9993c97\n",  // sha1sum of test file
      delegate.response_);
}

TEST_F(TwoPhaseUploaderTest, PhaseOneConnectionClosed) {
  scoped_refptr<MessageLoopRunner> runner = new MessageLoopRunner;
  LocalTwoPhaseTestServer test_server;
  ASSERT_TRUE(test_server.Start());
  Delegate delegate;
  std::unique_ptr<TwoPhaseUploader> uploader(TwoPhaseUploader::Create(
      shared_url_loader_factory_, task_runner_.get(),
      test_server.GetURL("start?p1close=1"), "metadata", GetTestFilePath(),
      base::Bind(&Delegate::FinishCallback, base::Unretained(&delegate),
                 runner),
      TRAFFIC_ANNOTATION_FOR_TESTS));
  uploader->Start();
  runner->Run();
  EXPECT_EQ(TwoPhaseUploader::UPLOAD_METADATA, delegate.state_);
  EXPECT_EQ(net::ERR_EMPTY_RESPONSE, delegate.net_error_);
  EXPECT_EQ("", delegate.response_);
}

TEST_F(TwoPhaseUploaderTest, PhaseTwoConnectionClosed) {
  scoped_refptr<MessageLoopRunner> runner = new MessageLoopRunner;
  LocalTwoPhaseTestServer test_server;
  ASSERT_TRUE(test_server.Start());
  Delegate delegate;
  std::unique_ptr<TwoPhaseUploader> uploader(TwoPhaseUploader::Create(
      shared_url_loader_factory_, task_runner_.get(),
      test_server.GetURL("start?p2close=1"), "metadata", GetTestFilePath(),
      base::Bind(&Delegate::FinishCallback, base::Unretained(&delegate),
                 runner),
      TRAFFIC_ANNOTATION_FOR_TESTS));
  uploader->Start();
  runner->Run();
  EXPECT_EQ(TwoPhaseUploader::UPLOAD_FILE, delegate.state_);
  EXPECT_EQ(net::ERR_EMPTY_RESPONSE, delegate.net_error_);
  EXPECT_EQ("", delegate.response_);
}

}  // namespace safe_browsing
