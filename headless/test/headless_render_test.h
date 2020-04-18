// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_TEST_HEADLESS_RENDER_TEST_H_
#define HEADLESS_TEST_HEADLESS_RENDER_TEST_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/message_loop/message_loop_current.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "headless/public/devtools/domains/emulation.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_browser_context.h"
#include "headless/public/util/testing/test_in_memory_protocol_handler.h"
#include "headless/test/headless_browser_test.h"
#include "net/test/embedded_test_server/http_request.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace headless {
class CompositorController;
class HeadlessDevToolsClient;
class VirtualTimeController;
namespace dom_snapshot {
class GetSnapshotResult;
}  // namespace dom_snapshot

// Base class for tests that render a particular page and verify the output.
class HeadlessRenderTest : public HeadlessAsyncDevTooledBrowserTest,
                           public HeadlessBrowserContext::Observer,
                           public page::ExperimentalObserver,
                           public runtime::ExperimentalObserver,
                           public TestInMemoryProtocolHandler::RequestDeferrer {
 public:
  typedef std::pair<std::string, page::FrameScheduledNavigationReason> Redirect;

  void RunDevTooledTest() override;

 protected:
  // Automatically waits in destructor until callback is called.
  class Sync {
   public:
    Sync() {}
    ~Sync() {
      base::MessageLoopCurrent::ScopedNestableTaskAllower nest_loop;
      run_loop.Run();
    }
    operator base::OnceClosure() { return run_loop.QuitClosure(); }

   private:
    base::RunLoop run_loop;
    DISALLOW_COPY_AND_ASSIGN(Sync);
  };

  struct ScreenshotOptions {
    ScreenshotOptions(const std::string& golden_file_name,
                      int x,
                      int y,
                      int width,
                      int height,
                      double scale);

    std::string golden_file_name;
    int x;
    int y;
    int width;
    int height;
    double scale;
  };

  HeadlessRenderTest();
  ~HeadlessRenderTest() override;

  // Marks that the test case reached the final conclusion.
  void SetTestCompleted() { state_ = FINISHED; }

  // The protocol handler used to respond to requests.
  TestInMemoryProtocolHandler* GetProtocolHandler() { return http_handler_; }

  // Do necessary preparations and return a URL to render.
  virtual GURL GetPageUrl(HeadlessDevToolsClient* client) = 0;

  // Check if the DOM snapshot is as expected.
  virtual void VerifyDom(dom_snapshot::GetSnapshotResult* dom_snapshot);

  // Called when all steps needed to load and present page are done.
  virtual void OnPageRenderCompleted();

  // Called if page rendering wasn't completed within reasonable time.
  virtual void OnTimeout();

  // Override to set specific options for requests.
  virtual void OverrideWebPreferences(WebPreferences* preferences);

  // Determines whether a screenshot will be taken or not. If one is taken,
  // ScreenshotOptions specifies the area to capture as well as a golden data
  // file to compare the result to. By default, no screenshot is taken.
  virtual base::Optional<ScreenshotOptions> GetScreenshotOptions();

  // Returns the emulated width/height of the window. Defaults to 800x600.
  virtual gfx::Size GetEmulatedWindowSize();

  // Setting up the browsertest.
  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUp() override;
  void CustomizeHeadlessBrowserContext(
      HeadlessBrowserContext::Builder& builder) override;
  bool GetEnableBeginFrameControl() override;
  void PostRunAsynchronousTest() override;
  ProtocolHandlerMap GetProtocolHandlers() override;

  // HeadlessBrowserContext::Observer
  void UrlRequestFailed(net::URLRequest* request,
                        int net_error,
                        DevToolsStatus devtools_status) override;

  // page::ExperimentalObserver implementation:
  void OnLoadEventFired(const page::LoadEventFiredParams& params) override;
  void OnFrameStartedLoading(
      const page::FrameStartedLoadingParams& params) override;
  void OnFrameScheduledNavigation(
      const page::FrameScheduledNavigationParams& params) override;
  void OnFrameClearedScheduledNavigation(
      const page::FrameClearedScheduledNavigationParams& params) override;
  void OnFrameNavigated(const page::FrameNavigatedParams& params) override;

  // runtime::ExperimentalObserver implementation:
  void OnConsoleAPICalled(
      const runtime::ConsoleAPICalledParams& params) override;
  void OnExceptionThrown(const runtime::ExceptionThrownParams& params) override;

  // TestInMemoryProtocolHandler::RequestDeferrer
  void OnRequest(const GURL& url, base::Closure complete_request) override;

  std::map<std::string, std::vector<Redirect>> confirmed_frame_redirects_;
  std::map<std::string, Redirect> unconfirmed_frame_redirects_;
  std::map<std::string, std::vector<std::unique_ptr<page::Frame>>> frames_;
  std::string main_frame_;
  std::vector<std::string> console_log_;
  std::vector<std::string> js_exceptions_;

 private:
  class AdditionalVirtualTimeBudget;

  void SetDeviceMetricsOverride(
      std::unique_ptr<headless::page::Viewport> viewport);
  void HandleVirtualTimeExhausted();
  void OnGetDomSnapshotDone(
      std::unique_ptr<dom_snapshot::GetSnapshotResult> result);
  void CaptureScreenshot(const ScreenshotOptions& options);
  void ScreenshotCaptured(const ScreenshotOptions& options,
                          const std::string& data);
  void RenderComplete();
  void HandleTimeout();
  void CleanUp();

  enum State {
    INIT,        // Setting up the client, no navigation performed yet.
    STARTING,    // Navigation request issued but URL not being loaded yet.
    LOADING,     // URL was requested but resources are not fully loaded yet.
    RENDERING,   // Main resources were loaded but page is still being rendered.
    DONE,        // Page considered to be rendered, DOM snapshot is being taken.
    SCREENSHOT,  // DOM snapshot has completed, screenshot is being taken.
    FINISHED,    // Test has finished.
  };
  State state_ = INIT;

  std::unique_ptr<VirtualTimeController> virtual_time_controller_;
  std::unique_ptr<CompositorController> compositor_controller_;
  TestInMemoryProtocolHandler* http_handler_;  // NOT OWNED

  base::test::ScopedFeatureList scoped_feature_list_;

  base::WeakPtrFactory<HeadlessRenderTest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessRenderTest);
};

}  // namespace headless

#endif  // HEADLESS_TEST_HEADLESS_RENDER_TEST_H_
