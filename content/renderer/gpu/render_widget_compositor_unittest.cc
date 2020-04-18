// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/gpu/render_widget_compositor.h"

#include <utility>

#include "base/location.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "cc/animation/animation_host.h"
#include "cc/test/fake_layer_tree_frame_sink.h"
#include "cc/trees/layer_tree_host.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/test/test_context_provider.h"
#include "content/public/common/screen_info.h"
#include "content/public/test/mock_render_thread.h"
#include "content/renderer/render_widget.h"
#include "content/test/fake_compositor_dependencies.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"

using testing::AllOf;
using testing::Field;

namespace content {
namespace {

enum FailureMode {
  NO_FAILURE,
  BIND_CONTEXT_FAILURE,
  GPU_CHANNEL_FAILURE,
};

class StubRenderWidgetCompositorDelegate
    : public RenderWidgetCompositorDelegate {
 public:
  // RenderWidgetCompositorDelegate implementation.
  void ApplyViewportDeltas(const gfx::Vector2dF& inner_delta,
                           const gfx::Vector2dF& outer_delta,
                           const gfx::Vector2dF& elastic_overscroll_delta,
                           float page_scale,
                           float top_controls_delta) override {}
  void RecordWheelAndTouchScrollingCount(bool has_scrolled_by_wheel,
                                         bool has_scrolled_by_touch) override {}
  void BeginMainFrame(base::TimeTicks frame_time) override {}
  void RequestNewLayerTreeFrameSink(
      const LayerTreeFrameSinkCallback& callback) override {
    callback.Run(nullptr);
  }
  void DidCommitAndDrawCompositorFrame() override {}
  void DidCommitCompositorFrame() override {}
  void DidCompletePageScaleAnimation() override {}
  void DidReceiveCompositorFrameAck() override {}
  bool IsClosing() const override { return false; }
  void RequestScheduleAnimation() override {}
  void UpdateVisualState(VisualStateUpdate requested_update) override {}
  void WillBeginCompositorFrame() override {}
  std::unique_ptr<cc::SwapPromise> RequestCopyOfOutputForLayoutTest(
      std::unique_ptr<viz::CopyOutputRequest> request) override {
    return nullptr;
  }
};

class FakeRenderWidgetCompositorDelegate
    : public StubRenderWidgetCompositorDelegate {
 public:
  FakeRenderWidgetCompositorDelegate() = default;

  void RequestNewLayerTreeFrameSink(
      const LayerTreeFrameSinkCallback& callback) override {
    // Subtract one cuz the current request has already been counted but should
    // not be included for this.
    if (num_requests_since_last_success_ - 1 < num_requests_before_success_) {
      callback.Run(std::unique_ptr<cc::LayerTreeFrameSink>());
      return;
    }

    auto context_provider = viz::TestContextProvider::Create();
    if (num_failures_since_last_success_ < num_failures_before_success_) {
      context_provider->UnboundTestContext3d()->loseContextCHROMIUM(
          GL_GUILTY_CONTEXT_RESET_ARB, GL_INNOCENT_CONTEXT_RESET_ARB);
    }
    callback.Run(
        cc::FakeLayerTreeFrameSink::Create3d(std::move(context_provider)));
  }

  void Reset() {
    num_requests_ = 0;
    num_requests_before_success_ = 0;
    num_requests_since_last_success_ = 0;
    num_failures_ = 0;
    num_failures_before_success_ = 0;
    num_failures_since_last_success_ = 0;
    num_successes_ = 0;
  }

  void add_success() {
    ++num_successes_;
    num_requests_since_last_success_ = 0;
    num_failures_since_last_success_ = 0;
  }
  int num_successes() const { return num_successes_; }

  void add_request() {
    ++num_requests_since_last_success_;
    ++num_requests_;
  }
  int num_requests() const { return num_requests_; }

  void add_failure() {
    ++num_failures_since_last_success_;
    ++num_failures_;
  }
  int num_failures() const { return num_failures_; }

  void set_num_requests_before_success(int n) {
    num_requests_before_success_ = n;
  }
  void set_num_failures_before_success(int n) {
    num_failures_before_success_ = n;
  }
  int num_failures_before_success() const {
    return num_failures_before_success_;
  }

 private:
  int num_requests_ = 0;
  int num_requests_before_success_ = 0;
  int num_requests_since_last_success_ = 0;
  int num_failures_ = 0;
  int num_failures_before_success_ = 0;
  int num_failures_since_last_success_ = 0;
  int num_successes_ = 0;

  DISALLOW_COPY_AND_ASSIGN(FakeRenderWidgetCompositorDelegate);
};

// Verify that failing to create an output surface will cause the compositor
// to attempt to repeatedly create another output surface.
// The use null output surface parameter allows testing whether failures
// from RenderWidget (couldn't create an output surface) vs failures from
// the compositor (couldn't bind the output surface) are handled identically.
class RenderWidgetLayerTreeFrameSink : public RenderWidgetCompositor {
 public:
  RenderWidgetLayerTreeFrameSink(FakeRenderWidgetCompositorDelegate* delegate,
                                 CompositorDependencies* compositor_deps)
      : RenderWidgetCompositor(delegate, compositor_deps),
        delegate_(delegate) {}

  using RenderWidgetCompositor::Initialize;

  // Force a new output surface to be created.
  void SynchronousComposite() {
    layer_tree_host()->SetVisible(false);
    layer_tree_host()->ReleaseLayerTreeFrameSink();
    layer_tree_host()->SetVisible(true);

    base::TimeTicks some_time;
    layer_tree_host()->Composite(some_time, true /* raster */);
  }

  void RequestNewLayerTreeFrameSink() override {
    delegate_->add_request();
    RenderWidgetCompositor::RequestNewLayerTreeFrameSink();
  }

  void DidInitializeLayerTreeFrameSink() override {
    RenderWidgetCompositor::DidInitializeLayerTreeFrameSink();
    delegate_->add_success();
    if (delegate_->num_successes() == expected_successes_) {
      EXPECT_EQ(delegate_->num_requests(), expected_requests_);
      EndTest();
    } else {
      // Post the synchronous composite task so that it is not called
      // reentrantly as a part of RequestNewLayerTreeFrameSink.
      blink::scheduler::GetSingleThreadTaskRunnerForTesting()->PostTask(
          FROM_HERE,
          base::BindOnce(&RenderWidgetLayerTreeFrameSink::SynchronousComposite,
                         base::Unretained(this)));
    }
  }

  void DidFailToInitializeLayerTreeFrameSink() override {
    RenderWidgetCompositor::DidFailToInitializeLayerTreeFrameSink();
    delegate_->add_failure();
    if (delegate_->num_requests() == expected_requests_) {
      EXPECT_EQ(delegate_->num_successes(), expected_successes_);
      EndTest();
      return;
    }
  }

  void SetUp(int expected_successes,
             int num_tries,
             FailureMode failure_mode,
             base::RunLoop* run_loop) {
    run_loop_ = run_loop;
    failure_mode_ = failure_mode;
    expected_successes_ = expected_successes;
    switch (failure_mode_) {
      case NO_FAILURE:
        expected_requests_ = expected_successes;
        break;
      case BIND_CONTEXT_FAILURE:
      case GPU_CHANNEL_FAILURE:
        expected_requests_ = num_tries * std::max(1, expected_successes);
        break;
    }
  }

  void EndTest() { run_loop_->Quit(); }

 private:
  FakeRenderWidgetCompositorDelegate* delegate_;
  base::RunLoop* run_loop_ = nullptr;
  int expected_successes_ = 0;
  int expected_requests_ = 0;
  FailureMode failure_mode_ = NO_FAILURE;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetLayerTreeFrameSink);
};

class RenderWidgetLayerTreeFrameSinkTest : public testing::Test {
 public:
  RenderWidgetLayerTreeFrameSinkTest()
      : render_widget_compositor_(&compositor_delegate_, &compositor_deps_) {
    auto animation_host = cc::AnimationHost::CreateMainInstance();

    ScreenInfo dummy_screen_info;
    auto layer_tree_host = RenderWidgetCompositor::CreateLayerTreeHost(
        &render_widget_compositor_, &render_widget_compositor_,
        animation_host.get(), &compositor_deps_, dummy_screen_info);
    render_widget_compositor_.Initialize(std::move(layer_tree_host),
                                         std::move(animation_host));
  }

  void RunTest(int expected_successes, FailureMode failure_mode) {
    compositor_delegate_.Reset();
    // 6 is just an artibrary "large" number to show it keeps trying.
    const int kTries = 6;
    // If it should fail, then it will fail every attempt, otherwise it fails
    // until the last attempt.
    int tries_before_success = kTries - (expected_successes ? 1 : 0);
    switch (failure_mode) {
      case NO_FAILURE:
        compositor_delegate_.set_num_failures_before_success(0);
        compositor_delegate_.set_num_requests_before_success(0);
        break;
      case BIND_CONTEXT_FAILURE:
        compositor_delegate_.set_num_failures_before_success(
            tries_before_success);
        compositor_delegate_.set_num_requests_before_success(0);
        break;
      case GPU_CHANNEL_FAILURE:
        compositor_delegate_.set_num_failures_before_success(0);
        compositor_delegate_.set_num_requests_before_success(
            tries_before_success);
        break;
    }
    base::RunLoop run_loop;
    render_widget_compositor_.SetUp(expected_successes, kTries, failure_mode,
                                    &run_loop);
    render_widget_compositor_.SetVisible(true);
    blink::scheduler::GetSingleThreadTaskRunnerForTesting()->PostTask(
        FROM_HERE,
        base::BindOnce(&RenderWidgetLayerTreeFrameSink::SynchronousComposite,
                       base::Unretained(&render_widget_compositor_)));
    run_loop.Run();
  }

 protected:
  base::MessageLoop ye_olde_message_loope_;
  MockRenderThread render_thread_;
  FakeCompositorDependencies compositor_deps_;
  FakeRenderWidgetCompositorDelegate compositor_delegate_;
  RenderWidgetLayerTreeFrameSink render_widget_compositor_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderWidgetLayerTreeFrameSinkTest);
};

TEST_F(RenderWidgetLayerTreeFrameSinkTest, SucceedOnce) {
  RunTest(1, NO_FAILURE);
}

TEST_F(RenderWidgetLayerTreeFrameSinkTest, SucceedOnce_AfterNullChannel) {
  RunTest(1, GPU_CHANNEL_FAILURE);
}

TEST_F(RenderWidgetLayerTreeFrameSinkTest, SucceedOnce_AfterLostContext) {
  RunTest(1, BIND_CONTEXT_FAILURE);
}

TEST_F(RenderWidgetLayerTreeFrameSinkTest, SucceedTwice) {
  RunTest(2, NO_FAILURE);
}

TEST_F(RenderWidgetLayerTreeFrameSinkTest, SucceedTwice_AfterNullChannel) {
  RunTest(2, GPU_CHANNEL_FAILURE);
}

TEST_F(RenderWidgetLayerTreeFrameSinkTest, SucceedTwice_AfterLostContext) {
  RunTest(2, BIND_CONTEXT_FAILURE);
}

TEST_F(RenderWidgetLayerTreeFrameSinkTest, FailWithNullChannel) {
  RunTest(0, GPU_CHANNEL_FAILURE);
}

TEST_F(RenderWidgetLayerTreeFrameSinkTest, FailWithLostContext) {
  RunTest(0, BIND_CONTEXT_FAILURE);
}

class VisibilityTestRenderWidgetCompositor : public RenderWidgetCompositor {
 public:
  VisibilityTestRenderWidgetCompositor(
      StubRenderWidgetCompositorDelegate* delegate,
      CompositorDependencies* compositor_deps)
      : RenderWidgetCompositor(delegate, compositor_deps) {}

  void RequestNewLayerTreeFrameSink() override {
    RenderWidgetCompositor::RequestNewLayerTreeFrameSink();
    num_requests_sent_++;
    if (run_loop_)
      run_loop_->Quit();
  }

  void set_run_loop(base::RunLoop* run_loop) { run_loop_ = run_loop; }
  int num_requests_sent() { return num_requests_sent_; }

 private:
  int num_requests_sent_ = 0;
  base::RunLoop* run_loop_;
};

TEST(RenderWidgetCompositorTest, VisibilityTest) {
  // Test that RenderWidgetCompositor does not retry FrameSink request while
  // invisible.

  base::MessageLoop message_loop;

  FakeCompositorDependencies compositor_deps;
  // Synchronously callback with null FrameSink.
  StubRenderWidgetCompositorDelegate compositor_delegate;
  VisibilityTestRenderWidgetCompositor render_widget_compositor(
      &compositor_delegate, &compositor_deps);

  auto animation_host = cc::AnimationHost::CreateMainInstance();
  ScreenInfo dummy_screen_info;
  auto layer_tree_host = RenderWidgetCompositor::CreateLayerTreeHost(
      &render_widget_compositor, &render_widget_compositor,
      animation_host.get(), &compositor_deps, dummy_screen_info);
  render_widget_compositor.Initialize(std::move(layer_tree_host),
                                      std::move(animation_host));

  {
    // Make one request and stop immediately while invisible.
    base::RunLoop run_loop;
    render_widget_compositor.set_run_loop(&run_loop);
    render_widget_compositor.SetVisible(false);
    render_widget_compositor.RequestNewLayerTreeFrameSink();
    run_loop.Run();
    render_widget_compositor.set_run_loop(nullptr);
    EXPECT_EQ(1, render_widget_compositor.num_requests_sent());
  }

  {
    // Make sure there are no more requests.
    base::RunLoop run_loop;
    run_loop.RunUntilIdle();
    EXPECT_EQ(1, render_widget_compositor.num_requests_sent());
  }

  {
    // Becoming visible retries request.
    base::RunLoop run_loop;
    render_widget_compositor.set_run_loop(&run_loop);
    render_widget_compositor.SetVisible(true);
    run_loop.Run();
    render_widget_compositor.set_run_loop(nullptr);
    EXPECT_EQ(2, render_widget_compositor.num_requests_sent());
  }
}

// Verify desktop memory limit calculations.
#if !defined(OS_ANDROID)
TEST(RenderWidgetCompositorTest, IgnoreGivenMemoryPolicy) {
  auto policy = RenderWidgetCompositor::GetGpuMemoryPolicy(
      cc::ManagedMemoryPolicy(256), ScreenInfo());
  EXPECT_EQ(512u * 1024u * 1024u, policy.bytes_limit_when_visible);
  EXPECT_EQ(gpu::MemoryAllocation::CUTOFF_ALLOW_NICE_TO_HAVE,
            policy.priority_cutoff_when_visible);
}

TEST(RenderWidgetCompositorTest, LargeScreensUseMoreMemory) {
  ScreenInfo screen_info;

  screen_info.rect = gfx::Rect(4096, 2160);
  screen_info.device_scale_factor = 1.f;
  auto policy = RenderWidgetCompositor::GetGpuMemoryPolicy(
      cc::ManagedMemoryPolicy(256), screen_info);
  EXPECT_EQ(2u * 512u * 1024u * 1024u, policy.bytes_limit_when_visible);
  EXPECT_EQ(gpu::MemoryAllocation::CUTOFF_ALLOW_NICE_TO_HAVE,
            policy.priority_cutoff_when_visible);

  screen_info.rect = gfx::Rect(2048, 1080);
  screen_info.device_scale_factor = 2.f;
  policy = RenderWidgetCompositor::GetGpuMemoryPolicy(
      cc::ManagedMemoryPolicy(256), screen_info);
  EXPECT_EQ(2u * 512u * 1024u * 1024u, policy.bytes_limit_when_visible);
  EXPECT_EQ(gpu::MemoryAllocation::CUTOFF_ALLOW_NICE_TO_HAVE,
            policy.priority_cutoff_when_visible);
}
#endif  // !defined(OS_ANDROID)

}  // namespace
}  // namespace content
