// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/update_client/ping_manager.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/version.h"
#include "components/update_client/component.h"
#include "components/update_client/protocol_builder.h"
#include "components/update_client/test_configurator.h"
#include "components/update_client/update_engine.h"
#include "components/update_client/url_request_post_interceptor.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using std::string;

namespace update_client {

class PingManagerTest : public testing::Test {
 public:
  PingManagerTest();
  ~PingManagerTest() override {}

  PingManager::Callback MakePingCallback();
  scoped_refptr<UpdateContext> MakeMockUpdateContext() const;

  // Overrides from testing::Test.
  void SetUp() override;
  void TearDown() override;

  void PingSentCallback(int error, const std::string& response);

 protected:
  void Quit();
  void RunThreads();

  scoped_refptr<TestConfigurator> config_;
  scoped_refptr<PingManager> ping_manager_;

  int error_ = -1;
  std::string response_;

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::OnceClosure quit_closure_;
};

PingManagerTest::PingManagerTest()
    : scoped_task_environment_(
          base::test::ScopedTaskEnvironment::MainThreadType::IO) {
  config_ = base::MakeRefCounted<TestConfigurator>();
}

void PingManagerTest::SetUp() {
  ping_manager_ = base::MakeRefCounted<PingManager>(config_);
}

void PingManagerTest::TearDown() {
  // Run the threads until they are idle to allow the clean up
  // of the network interceptors on the IO thread.
  scoped_task_environment_.RunUntilIdle();
  ping_manager_ = nullptr;
}

void PingManagerTest::RunThreads() {
  base::RunLoop runloop;
  quit_closure_ = runloop.QuitClosure();
  runloop.Run();
}

void PingManagerTest::Quit() {
  if (!quit_closure_.is_null())
    std::move(quit_closure_).Run();
}

PingManager::Callback PingManagerTest::MakePingCallback() {
  return base::BindOnce(&PingManagerTest::PingSentCallback,
                        base::Unretained(this));
}

void PingManagerTest::PingSentCallback(int error, const std::string& response) {
  error_ = error;
  response_ = response;
  Quit();
}

scoped_refptr<UpdateContext> PingManagerTest::MakeMockUpdateContext() const {
  return base::MakeRefCounted<UpdateContext>(
      config_, false, std::vector<std::string>(),
      UpdateClient::CrxDataCallback(), UpdateEngine::NotifyObserversCallback(),
      UpdateEngine::Callback(), nullptr);
}

TEST_F(PingManagerTest, SendPing) {
  auto interceptor_factory =
      std::make_unique<InterceptorFactory>(base::ThreadTaskRunnerHandle::Get());
  auto interceptor = interceptor_factory->CreateInterceptor();
  EXPECT_TRUE(interceptor);

  // Test eventresult="1" is sent for successful updates.
  const auto update_context = MakeMockUpdateContext();

  {
    Component component(*update_context, "abc");
    component.crx_component_ = std::make_unique<CrxComponent>();
    component.state_ = std::make_unique<Component::StateUpdated>(&component);
    component.previous_version_ = base::Version("1.0");
    component.next_version_ = base::Version("2.0");
    component.AppendEvent(BuildUpdateCompleteEventElement(component));

    EXPECT_TRUE(interceptor->ExpectRequest(std::make_unique<AnyMatch>()));
    ping_manager_->SendPing(component, MakePingCallback());
    RunThreads();

    EXPECT_EQ(1, interceptor->GetCount()) << interceptor->GetRequestsAsString();
    EXPECT_NE(string::npos,
              interceptor->GetRequestBody(0).find(
                  "<app appid=\"abc\">"
                  "<event eventtype=\"3\" eventresult=\"1\" "
                  "previousversion=\"1.0\" nextversion=\"2.0\"/></app>"))
        << interceptor->GetRequestsAsString();
    EXPECT_NE(string::npos, interceptor->GetRequestBody(0).find(" sessionid="));

    // Check the ping request does not carry the specific extra request headers.
    EXPECT_FALSE(interceptor->GetRequests()[0].second.HasHeader(
        "X-Goog-Update-Interactivity"));
    EXPECT_FALSE(interceptor->GetRequests()[0].second.HasHeader(
        "X-Goog-Update-Updater"));
    EXPECT_FALSE(
        interceptor->GetRequests()[0].second.HasHeader("X-Goog-Update-AppId"));

    interceptor->Reset();
  }

  {
    // Test eventresult="0" is sent for failed updates.
    Component component(*update_context, "abc");
    component.crx_component_ = std::make_unique<CrxComponent>();
    component.state_ =
        std::make_unique<Component::StateUpdateError>(&component);
    component.previous_version_ = base::Version("1.0");
    component.next_version_ = base::Version("2.0");
    component.AppendEvent(BuildUpdateCompleteEventElement(component));

    EXPECT_TRUE(interceptor->ExpectRequest(std::make_unique<AnyMatch>()));
    ping_manager_->SendPing(component, MakePingCallback());
    RunThreads();

    EXPECT_EQ(1, interceptor->GetCount()) << interceptor->GetRequestsAsString();
    EXPECT_NE(string::npos,
              interceptor->GetRequestBody(0).find(
                  "<app appid=\"abc\">"
                  "<event eventtype=\"3\" eventresult=\"0\" "
                  "previousversion=\"1.0\" nextversion=\"2.0\"/></app>"))
        << interceptor->GetRequestsAsString();
    interceptor->Reset();
  }

  {
    // Test the error values and the fingerprints.
    Component component(*update_context, "abc");
    component.crx_component_ = std::make_unique<CrxComponent>();
    component.state_ =
        std::make_unique<Component::StateUpdateError>(&component);
    component.previous_version_ = base::Version("1.0");
    component.next_version_ = base::Version("2.0");
    component.previous_fp_ = "prev fp";
    component.next_fp_ = "next fp";
    component.error_category_ = ErrorCategory::kDownload;
    component.error_code_ = 2;
    component.extra_code1_ = -1;
    component.diff_error_category_ = ErrorCategory::kService;
    component.diff_error_code_ = 20;
    component.diff_extra_code1_ = -10;
    component.crx_diffurls_.push_back(GURL("http://host/path"));
    component.AppendEvent(BuildUpdateCompleteEventElement(component));

    EXPECT_TRUE(interceptor->ExpectRequest(std::make_unique<AnyMatch>()));
    ping_manager_->SendPing(component, MakePingCallback());
    RunThreads();

    EXPECT_EQ(1, interceptor->GetCount()) << interceptor->GetRequestsAsString();
    EXPECT_NE(string::npos,
              interceptor->GetRequestBody(0).find(
                  "<app appid=\"abc\">"
                  "<event eventtype=\"3\" eventresult=\"0\" errorcat=\"1\" "
                  "errorcode=\"2\" extracode1=\"-1\" diffresult=\"0\" "
                  "differrorcat=\"4\" "
                  "differrorcode=\"20\" diffextracode1=\"-10\" "
                  "previousfp=\"prev fp\" nextfp=\"next fp\" "
                  "previousversion=\"1.0\" nextversion=\"2.0\"/></app>"))
        << interceptor->GetRequestsAsString();
    interceptor->Reset();
  }

  {
    // Test an invalid |next_version| is not serialized.
    Component component(*update_context, "abc");
    component.crx_component_ = std::make_unique<CrxComponent>();
    component.state_ =
        std::make_unique<Component::StateUpdateError>(&component);
    component.previous_version_ = base::Version("1.0");

    component.AppendEvent(BuildUpdateCompleteEventElement(component));

    EXPECT_TRUE(interceptor->ExpectRequest(std::make_unique<AnyMatch>()));
    ping_manager_->SendPing(component, MakePingCallback());
    RunThreads();

    EXPECT_EQ(1, interceptor->GetCount()) << interceptor->GetRequestsAsString();
    EXPECT_NE(string::npos,
              interceptor->GetRequestBody(0).find(
                  "<app appid=\"abc\"><event eventtype=\"3\" eventresult=\"0\" "
                  "previousversion=\"1.0\"/></app>"))
        << interceptor->GetRequestsAsString();
    interceptor->Reset();
  }

  {
    // Test a valid |previouversion| and |next_version| = base::Version("0")
    // are serialized correctly under <event...> for uninstall.
    Component component(*update_context, "abc");
    component.crx_component_ = std::make_unique<CrxComponent>();
    component.Uninstall(base::Version("1.2.3.4"), 0);
    component.AppendEvent(BuildUninstalledEventElement(component));

    EXPECT_TRUE(interceptor->ExpectRequest(std::make_unique<AnyMatch>()));
    ping_manager_->SendPing(component, MakePingCallback());
    RunThreads();

    EXPECT_EQ(1, interceptor->GetCount()) << interceptor->GetRequestsAsString();
    EXPECT_NE(string::npos,
              interceptor->GetRequestBody(0).find(
                  "<app appid=\"abc\">"
                  "<event eventtype=\"4\" eventresult=\"1\" "
                  "previousversion=\"1.2.3.4\" nextversion=\"0\"/></app>"))
        << interceptor->GetRequestsAsString();
    interceptor->Reset();
  }

  {
    // Test the download metrics.
    Component component(*update_context, "abc");
    component.crx_component_ = std::make_unique<CrxComponent>();
    component.state_ = std::make_unique<Component::StateUpdated>(&component);
    component.previous_version_ = base::Version("1.0");
    component.next_version_ = base::Version("2.0");
    component.AppendEvent(BuildUpdateCompleteEventElement(component));

    CrxDownloader::DownloadMetrics download_metrics;
    download_metrics.url = GURL("http://host1/path1");
    download_metrics.downloader = CrxDownloader::DownloadMetrics::kUrlFetcher;
    download_metrics.error = -1;
    download_metrics.downloaded_bytes = 123;
    download_metrics.total_bytes = 456;
    download_metrics.download_time_ms = 987;
    component.AppendEvent(
        BuildDownloadCompleteEventElement(component, download_metrics));

    download_metrics = CrxDownloader::DownloadMetrics();
    download_metrics.url = GURL("http://host2/path2");
    download_metrics.downloader = CrxDownloader::DownloadMetrics::kBits;
    download_metrics.error = 0;
    download_metrics.downloaded_bytes = 1230;
    download_metrics.total_bytes = 4560;
    download_metrics.download_time_ms = 9870;
    component.AppendEvent(
        BuildDownloadCompleteEventElement(component, download_metrics));

    EXPECT_TRUE(interceptor->ExpectRequest(std::make_unique<AnyMatch>()));
    ping_manager_->SendPing(component, MakePingCallback());
    RunThreads();

    EXPECT_EQ(1, interceptor->GetCount()) << interceptor->GetRequestsAsString();
    EXPECT_NE(
        string::npos,
        interceptor->GetRequestBody(0).find(
            "<app appid=\"abc\">"
            "<event eventtype=\"3\" eventresult=\"1\" previousversion=\"1.0\" "
            "nextversion=\"2.0\"/>"
            "<event eventtype=\"14\" eventresult=\"0\" downloader=\"direct\" "
            "errorcode=\"-1\" url=\"http://host1/path1\" downloaded=\"123\" "
            "total=\"456\" download_time_ms=\"987\" previousversion=\"1.0\" "
            "nextversion=\"2.0\"/>"
            "<event eventtype=\"14\" eventresult=\"1\" downloader=\"bits\" "
            "url=\"http://host2/path2\" downloaded=\"1230\" total=\"4560\" "
            "download_time_ms=\"9870\" previousversion=\"1.0\" "
            "nextversion=\"2.0\"/></app>"))
        << interceptor->GetRequestsAsString();
    interceptor->Reset();
  }
}

// Tests that sending the ping fails when the component requires encryption but
// the ping URL is unsecure.
TEST_F(PingManagerTest, RequiresEncryption) {
  config_->SetPingUrl(GURL("http:\\foo\bar"));

  const auto update_context = MakeMockUpdateContext();

  Component component(*update_context, "abc");
  component.crx_component_ = std::make_unique<CrxComponent>();

  // The default value for |requires_network_encryption| is true.
  EXPECT_TRUE(component.crx_component_->requires_network_encryption);

  component.state_ = std::make_unique<Component::StateUpdated>(&component);
  component.previous_version_ = base::Version("1.0");
  component.next_version_ = base::Version("2.0");
  component.AppendEvent(BuildUpdateCompleteEventElement(component));

  ping_manager_->SendPing(component, MakePingCallback());
  RunThreads();

  EXPECT_EQ(-2, error_);
}

}  // namespace update_client
