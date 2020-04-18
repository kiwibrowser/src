// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/test/headless_render_test.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/threading/thread_restrictions.h"
#include "cc/base/switches.h"
#include "components/viz/common/features.h"
#include "components/viz/common/switches.h"
#include "content/public/common/content_switches.h"
#include "headless/public/devtools/domains/dom_snapshot.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/util/compositor_controller.h"
#include "headless/public/util/virtual_time_controller.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/url_request/url_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/skia_util.h"

namespace headless {

namespace {

static constexpr int kAnimationIntervalMs = 100;
static constexpr bool kUpdateDisplayForAnimations = false;
static const char kUpdateGoldens[] = "update-goldens";

void SetVirtualTimePolicyDoneCallback(
    base::RunLoop* run_loop,
    std::unique_ptr<emulation::SetVirtualTimePolicyResult>) {
  run_loop->Quit();
}

bool DecodePNG(const std::string& data, SkBitmap* bitmap) {
  return gfx::PNGCodec::Decode(
      reinterpret_cast<unsigned const char*>(data.data()), data.size(), bitmap);
}

bool ColorsMatchWithinLimit(SkColor color1, SkColor color2, int error_limit) {
  auto a_diff = static_cast<int>(SkColorGetA(color1)) -
                static_cast<int>(SkColorGetA(color2));
  auto r_diff = static_cast<int>(SkColorGetR(color1)) -
                static_cast<int>(SkColorGetR(color2));
  auto g_diff = static_cast<int>(SkColorGetG(color1)) -
                static_cast<int>(SkColorGetG(color2));
  auto b_diff = static_cast<int>(SkColorGetB(color1)) -
                static_cast<int>(SkColorGetB(color2));
  return a_diff * a_diff + r_diff * r_diff + g_diff * g_diff +
             b_diff * b_diff <=
         error_limit * error_limit;
}

bool MatchesBitmap(const SkBitmap& expected_bmp,
                   const SkBitmap& actual_bmp,
                   int error_limit) {
  // Number of pixels with an error
  int error_pixels_count = 0;
  gfx::Rect error_bounding_rect = gfx::Rect();

  // Check that bitmaps have identical dimensions.
  EXPECT_EQ(expected_bmp.width(), actual_bmp.width());
  EXPECT_EQ(expected_bmp.height(), actual_bmp.height());
  if (expected_bmp.width() != actual_bmp.width() ||
      expected_bmp.height() != actual_bmp.height()) {
    LOG(ERROR) << "To update goldens, use --update-goldens.";
    return false;
  }

  for (int y = 0; y < actual_bmp.height(); ++y) {
    for (int x = 0; x < actual_bmp.width(); ++x) {
      SkColor actual_color = actual_bmp.getColor(x, y);
      SkColor expected_color = expected_bmp.getColor(x, y);
      if (!ColorsMatchWithinLimit(actual_color, expected_color, error_limit)) {
        if (error_pixels_count < 10) {
          LOG(ERROR) << "Pixel (" << x << "," << y << "): expected " << std::hex
                     << expected_color << " actual " << actual_color;
        }
        error_pixels_count++;
        error_bounding_rect.Union(gfx::Rect(x, y, 1, 1));
      }
    }
  }

  if (error_pixels_count != 0) {
    LOG(ERROR) << "Number of pixel with an error: " << error_pixels_count;
    LOG(ERROR) << "Error Bounding Box : " << error_bounding_rect.ToString();
    LOG(ERROR) << "To update goldens, use --update-goldens.";
    return false;
  }

  return true;
}

bool WriteStringToFile(const base::FilePath& file_path,
                       const std::string& content) {
  int result = base::WriteFile(file_path, content.data(),
                               static_cast<int>(content.size()));
  return content.size() == static_cast<size_t>(result);
}

bool ScreenshotMatchesGolden(const std::string& screenshot_data,
                             const std::string& golden_file_name) {
  static const base::FilePath kGoldenDirectory(
      FILE_PATH_LITERAL("headless/test/data/golden"));

  SkBitmap actual_bitmap;
  EXPECT_TRUE(DecodePNG(screenshot_data, &actual_bitmap));
  if (actual_bitmap.empty())
    return false;

  base::ScopedAllowBlockingForTesting allow_blocking;

  base::FilePath src_dir;
  CHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &src_dir));
  base::FilePath golden_path =
      src_dir.Append(kGoldenDirectory).Append(golden_file_name);

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kUpdateGoldens)) {
    LOG(INFO) << "Updating golden file at " << golden_path;
    CHECK(WriteStringToFile(golden_path, screenshot_data));
  }

  std::string golden_data;
  CHECK(base::ReadFileToString(golden_path, &golden_data));

  SkBitmap expected_bitmap;
  EXPECT_TRUE(DecodePNG(golden_data, &expected_bitmap));
  if (expected_bitmap.empty())
    return false;

  return MatchesBitmap(expected_bitmap, actual_bitmap, 5);
}

}  // namespace

HeadlessRenderTest::HeadlessRenderTest() : weak_ptr_factory_(this) {}

HeadlessRenderTest::~HeadlessRenderTest() = default;

void HeadlessRenderTest::PostRunAsynchronousTest() {
  // Make sure the test did complete.
  EXPECT_EQ(FINISHED, state_) << "The test did not finish.";
}

class HeadlessRenderTest::AdditionalVirtualTimeBudget
    : public VirtualTimeController::RepeatingTask,
      public VirtualTimeController::Observer {
 public:
  AdditionalVirtualTimeBudget(VirtualTimeController* virtual_time_controller,
                              HeadlessRenderTest* test,
                              base::RunLoop* run_loop,
                              int budget_ms)
      : RepeatingTask(StartPolicy::WAIT_FOR_NAVIGATION, 0),
        virtual_time_controller_(virtual_time_controller),
        test_(test),
        run_loop_(run_loop) {
    virtual_time_controller_->ScheduleRepeatingTask(
        this, base::TimeDelta::FromMilliseconds(budget_ms));
    virtual_time_controller_->AddObserver(this);
    virtual_time_controller_->StartVirtualTime();
  }

  ~AdditionalVirtualTimeBudget() override {
    virtual_time_controller_->RemoveObserver(this);
    virtual_time_controller_->CancelRepeatingTask(this);
  }

  // headless::VirtualTimeController::RepeatingTask implementation:
  void IntervalElapsed(
      base::TimeDelta virtual_time,
      base::OnceCallback<void(ContinuePolicy)> continue_callback) override {
    std::move(continue_callback).Run(ContinuePolicy::NOT_REQUIRED);
  }

  // headless::VirtualTimeController::Observer:
  void VirtualTimeStarted(base::TimeDelta virtual_time_offset) override {
    run_loop_->Quit();
  }

  void VirtualTimeStopped(base::TimeDelta virtual_time_offset) override {
    test_->HandleVirtualTimeExhausted();
    delete this;
  }

 private:
  headless::VirtualTimeController* const virtual_time_controller_;
  HeadlessRenderTest* test_;
  base::RunLoop* run_loop_;
};

void HeadlessRenderTest::RunDevTooledTest() {
  http_handler_->SetHeadlessBrowserContext(browser_context_);

  virtual_time_controller_ =
      std::make_unique<VirtualTimeController>(devtools_client_.get());

  SetDeviceMetricsOverride(headless::page::Viewport::Builder()
                               .SetX(0)
                               .SetY(0)
                               .SetWidth(1)
                               .SetHeight(1)
                               .SetScale(1)
                               .Build());

  compositor_controller_ = std::make_unique<CompositorController>(
      browser()->BrowserMainThread(), devtools_client_.get(),
      virtual_time_controller_.get(),
      base::TimeDelta::FromMilliseconds(kAnimationIntervalMs),
      kUpdateDisplayForAnimations);

  devtools_client_->GetPage()->GetExperimental()->AddObserver(this);
  devtools_client_->GetPage()->Enable(Sync());
  devtools_client_->GetRuntime()->GetExperimental()->AddObserver(this);
  devtools_client_->GetRuntime()->Enable(Sync());

  GURL url = GetPageUrl(devtools_client_.get());

  // Pause virtual time until we actually start loading content.
  {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    devtools_client_->GetEmulation()->GetExperimental()->SetVirtualTimePolicy(
        emulation::SetVirtualTimePolicyParams::Builder()
            .SetPolicy(emulation::VirtualTimePolicy::PAUSE)
            .Build());
    devtools_client_->GetEmulation()->GetExperimental()->SetVirtualTimePolicy(
        emulation::SetVirtualTimePolicyParams::Builder()
            .SetPolicy(
                emulation::VirtualTimePolicy::PAUSE_IF_NETWORK_FETCHES_PENDING)
            .SetBudget(4001)
            .SetWaitForNavigation(true)
            .Build(),
        base::BindOnce(&SetVirtualTimePolicyDoneCallback, &run_loop));

    run_loop.Run();
  }

  {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    // Note AdditionalVirtualTimeBudget will self delete.
    new AdditionalVirtualTimeBudget(virtual_time_controller_.get(), this,
                                    &run_loop, 5000);
    run_loop.Run();
  }

  state_ = STARTING;
  devtools_client_->GetPage()->Navigate(url.spec());
  browser()->BrowserMainThread()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&HeadlessRenderTest::HandleTimeout,
                     weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(10));

  // The caller will loop until FinishAsynchronousTest() is called either
  // from OnGetDomSnapshotDone() or from HandleTimeout().
}

void HeadlessRenderTest::SetDeviceMetricsOverride(
    std::unique_ptr<headless::page::Viewport> viewport) {
  gfx::Size size = GetEmulatedWindowSize();
  devtools_client_->GetEmulation()->GetExperimental()->SetDeviceMetricsOverride(
      headless::emulation::SetDeviceMetricsOverrideParams::Builder()
          .SetDeviceScaleFactor(0)
          .SetMobile(false)
          .SetWidth(size.width())
          .SetHeight(size.height())
          .SetScreenWidth(size.width())
          .SetScreenHeight(size.height())
          .SetViewport(std::move(viewport))
          .Build(),
      Sync());
}

void HeadlessRenderTest::OnTimeout() {
  ADD_FAILURE() << "Rendering timed out!";
}

void HeadlessRenderTest::SetUpCommandLine(base::CommandLine* command_line) {
  HeadlessAsyncDevTooledBrowserTest::SetUpCommandLine(command_line);
  // See bit.ly/headless-rendering for why we use these flags.
  command_line->AppendSwitch(switches::kRunAllCompositorStagesBeforeDraw);
  command_line->AppendSwitch(switches::kDisableNewContentRenderingTimeout);
  command_line->AppendSwitch(cc::switches::kDisableCheckerImaging);
  command_line->AppendSwitch(cc::switches::kDisableThreadedAnimation);
  command_line->AppendSwitch(switches::kDisableImageAnimationResync);
  command_line->AppendSwitch(switches::kDisableThreadedScrolling);

  scoped_feature_list_.InitAndEnableFeature(
      features::kEnableSurfaceSynchronization);
}

void HeadlessRenderTest::SetUp() {
  EnablePixelOutput();
  HeadlessAsyncDevTooledBrowserTest::SetUp();
}

void HeadlessRenderTest::CustomizeHeadlessBrowserContext(
    HeadlessBrowserContext::Builder& builder) {
  builder.SetOverrideWebPreferencesCallback(
      base::Bind(&HeadlessRenderTest::OverrideWebPreferences,
                 weak_ptr_factory_.GetWeakPtr()));
}

bool HeadlessRenderTest::GetEnableBeginFrameControl() {
  return true;
}

ProtocolHandlerMap HeadlessRenderTest::GetProtocolHandlers() {
  ProtocolHandlerMap protocol_handlers;
  std::unique_ptr<TestInMemoryProtocolHandler> http_handler(
      new TestInMemoryProtocolHandler(browser()->BrowserIOThread(), this));
  http_handler_ = http_handler.get();
  protocol_handlers[url::kHttpScheme] = std::move(http_handler);
  return protocol_handlers;
}

void HeadlessRenderTest::OverrideWebPreferences(WebPreferences* preferences) {
  preferences->hide_scrollbars = true;
  preferences->javascript_enabled = true;
  preferences->autoplay_policy = content::AutoplayPolicy::kUserGestureRequired;
}

base::Optional<HeadlessRenderTest::ScreenshotOptions>
HeadlessRenderTest::GetScreenshotOptions() {
  return base::nullopt;
}

gfx::Size HeadlessRenderTest::GetEmulatedWindowSize() {
  return gfx::Size(800, 600);
}

void HeadlessRenderTest::UrlRequestFailed(net::URLRequest* request,
                                          int net_error,
                                          DevToolsStatus devtools_status) {
  if (devtools_status != DevToolsStatus::kNotCanceled)
    return;
  ADD_FAILURE() << "Network request failed: " << net_error << " for "
                << request->url().spec();
}

void HeadlessRenderTest::OnLoadEventFired(const page::LoadEventFiredParams&) {
  CHECK_NE(INIT, state_);
  if (state_ == LOADING || state_ == STARTING) {
    state_ = RENDERING;
  }
}

void HeadlessRenderTest::OnFrameStartedLoading(
    const page::FrameStartedLoadingParams& params) {
  CHECK_NE(INIT, state_);
  if (state_ == STARTING) {
    state_ = LOADING;
    main_frame_ = params.GetFrameId();
  }

  auto it = unconfirmed_frame_redirects_.find(params.GetFrameId());
  if (it != unconfirmed_frame_redirects_.end()) {
    confirmed_frame_redirects_[params.GetFrameId()].push_back(it->second);
    unconfirmed_frame_redirects_.erase(it);
  }
}

void HeadlessRenderTest::OnFrameScheduledNavigation(
    const page::FrameScheduledNavigationParams& params) {
  CHECK(unconfirmed_frame_redirects_.find(params.GetFrameId()) ==
        unconfirmed_frame_redirects_.end());
  unconfirmed_frame_redirects_[params.GetFrameId()] =
      Redirect(params.GetUrl(), params.GetReason());
}

void HeadlessRenderTest::OnFrameClearedScheduledNavigation(
    const page::FrameClearedScheduledNavigationParams& params) {
  auto it = unconfirmed_frame_redirects_.find(params.GetFrameId());
  if (it != unconfirmed_frame_redirects_.end())
    unconfirmed_frame_redirects_.erase(it);
}

void HeadlessRenderTest::OnFrameNavigated(
    const page::FrameNavigatedParams& params) {
  frames_[params.GetFrame()->GetId()].push_back(params.GetFrame()->Clone());
}

void HeadlessRenderTest::OnConsoleAPICalled(
    const runtime::ConsoleAPICalledParams& params) {
  std::stringstream str;
  switch (params.GetType()) {
    case runtime::ConsoleAPICalledType::WARNING:
      str << "W";
      break;
    case runtime::ConsoleAPICalledType::ASSERT:
    case runtime::ConsoleAPICalledType::ERR:
      str << "E";
      break;
    case runtime::ConsoleAPICalledType::DEBUG:
      str << "D";
      break;
    case runtime::ConsoleAPICalledType::INFO:
      str << "I";
      break;
    default:
      str << "L";
      break;
  }
  const auto& args = *params.GetArgs();
  for (const auto& arg : args) {
    str << " ";
    if (arg->HasDescription()) {
      str << arg->GetDescription();
    } else if (arg->GetType() == runtime::RemoteObjectType::UNDEFINED) {
      str << "undefined";
    } else if (arg->HasValue()) {
      const base::Value* v = arg->GetValue();
      switch (v->type()) {
        case base::Value::Type::NONE:
          str << "null";
          break;
        case base::Value::Type::BOOLEAN:
          str << v->GetBool();
          break;
        case base::Value::Type::INTEGER:
          str << v->GetInt();
          break;
        case base::Value::Type::DOUBLE:
          str << v->GetDouble();
          break;
        case base::Value::Type::STRING:
          str << v->GetString();
          break;
        default:
          DCHECK(false);
          break;
      }
    } else {
      DCHECK(false);
    }
  }
  console_log_.push_back(str.str());
}

void HeadlessRenderTest::OnExceptionThrown(
    const runtime::ExceptionThrownParams& params) {
  const runtime::ExceptionDetails* details = params.GetExceptionDetails();
  js_exceptions_.push_back(details->GetText() + " " +
                           details->GetException()->GetDescription());
}

void HeadlessRenderTest::OnRequest(const GURL& url,
                                   base::Closure complete_request) {
  complete_request.Run();
}

void HeadlessRenderTest::VerifyDom(
    dom_snapshot::GetSnapshotResult* dom_snapshot) {}

void HeadlessRenderTest::OnPageRenderCompleted() {
  CHECK_GE(state_, LOADING);
  if (state_ >= DONE)
    return;
  state_ = DONE;

  devtools_client_->GetDOMSnapshot()->GetExperimental()->GetSnapshot(
      dom_snapshot::GetSnapshotParams::Builder()
          .SetComputedStyleWhitelist(std::vector<std::string>())
          .Build(),
      base::BindOnce(&HeadlessRenderTest::OnGetDomSnapshotDone,
                     weak_ptr_factory_.GetWeakPtr()));
}

void HeadlessRenderTest::HandleVirtualTimeExhausted() {
  if (state_ < DONE) {
    OnPageRenderCompleted();
  }
}

void HeadlessRenderTest::OnGetDomSnapshotDone(
    std::unique_ptr<dom_snapshot::GetSnapshotResult> result) {
  CHECK_EQ(DONE, state_);
  VerifyDom(result.get());

  base::Optional<ScreenshotOptions> screenshot_options = GetScreenshotOptions();
  if (screenshot_options) {
    state_ = SCREENSHOT;
    CaptureScreenshot(*screenshot_options);
    return;
  }
  RenderComplete();
}

void HeadlessRenderTest::CaptureScreenshot(const ScreenshotOptions& options) {
  // Set up emulation according to options.
  auto clip = headless::page::Viewport::Builder()
                  .SetX(options.x)
                  .SetY(options.y)
                  .SetWidth(options.width)
                  .SetHeight(options.height)
                  .SetScale(options.scale)
                  .Build();

  SetDeviceMetricsOverride(std::move(clip));

  compositor_controller_->CaptureScreenshot(
      CompositorController::ScreenshotParamsFormat::PNG, 100,
      base::BindRepeating(&HeadlessRenderTest::ScreenshotCaptured,
                          base::Unretained(this), options));
}

void HeadlessRenderTest::ScreenshotCaptured(const ScreenshotOptions& options,
                                            const std::string& data) {
  EXPECT_TRUE(ScreenshotMatchesGolden(data, options.golden_file_name));
  RenderComplete();
}

void HeadlessRenderTest::RenderComplete() {
  state_ = FINISHED;
  CleanUp();
  FinishAsynchronousTest();
}

void HeadlessRenderTest::HandleTimeout() {
  if (state_ != FINISHED) {
    CleanUp();
    FinishAsynchronousTest();
    OnTimeout();
  }
}

void HeadlessRenderTest::CleanUp() {
  devtools_client_->GetRuntime()->Disable(Sync());
  devtools_client_->GetRuntime()->GetExperimental()->RemoveObserver(this);
  devtools_client_->GetPage()->Disable(Sync());
  devtools_client_->GetPage()->GetExperimental()->RemoveObserver(this);
}

HeadlessRenderTest::ScreenshotOptions::ScreenshotOptions(
    const std::string& golden_file_name,
    int x,
    int y,
    int width,
    int height,
    double scale)
    : golden_file_name(golden_file_name),
      x(x),
      y(y),
      width(width),
      height(height),
      scale(scale) {}

}  // namespace headless
