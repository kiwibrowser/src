// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_LAYER_TREE_TEST_H_
#define CC_TEST_LAYER_TREE_TEST_H_

#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "cc/animation/animation_delegate.h"
#include "cc/test/test_hooks.h"
#include "cc/test/test_task_graph_runner.h"
#include "cc/trees/compositor_mode.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {
class BeginFrameSource;
class TestContextProvider;
class TestLayerTreeFrameSink;
}

namespace cc {

class AnimationHost;
class LayerImpl;
class LayerTreeHost;
class LayerTreeHostForTesting;
class LayerTreeTestLayerTreeFrameSinkClient;
class Proxy;
class SingleKeyframeEffectAnimation;
class TestTaskGraphRunner;

// Creates the virtual viewport layer hierarchy under the given root_layer.
// Convenient overload of the method below that creates a scrolling layer as
// the outer viewport scroll layer.
void CreateVirtualViewportLayers(Layer* root_layer,
                                 const gfx::Size& inner_bounds,
                                 const gfx::Size& outer_bounds,
                                 const gfx::Size& scroll_bounds,
                                 LayerTreeHost* host);

// Creates the virtual viewport layer hierarchy under the given root_layer.
// Uses the given scroll layer as the content "outer viewport scroll layer".
void CreateVirtualViewportLayers(Layer* root_layer,
                                 scoped_refptr<Layer> outer_scroll_layer,
                                 const gfx::Size& outer_bounds,
                                 const gfx::Size& scroll_bounds,
                                 LayerTreeHost* host);

class LayerTreeHostClientForTesting;

// The LayerTreeTests runs with the main loop running. It instantiates a single
// LayerTreeHostForTesting and associated LayerTreeHostImplForTesting and
// LayerTreeHostClientForTesting.
//
// BeginTest() is called once the main message loop is running and the layer
// tree host is initialized.
//
// Key stages of the drawing loop, e.g. drawing or commiting, redirect to
// LayerTreeTest methods of similar names. To track the commit process, override
// these functions.
//
// The test continues until someone calls EndTest. EndTest can be called on any
// thread, but be aware that ending the test is an asynchronous process.
class LayerTreeTest : public testing::Test, public TestHooks {
 public:
  ~LayerTreeTest() override;

  virtual void EndTest();
  void EndTestAfterDelayMs(int delay_milliseconds);

  void PostAddNoDamageAnimationToMainThread(
      SingleKeyframeEffectAnimation* animation_to_receive_animation);
  void PostAddOpacityAnimationToMainThread(
      SingleKeyframeEffectAnimation* animation_to_receive_animation);
  void PostAddOpacityAnimationToMainThreadInstantly(
      SingleKeyframeEffectAnimation* animation_to_receive_animation);
  void PostAddOpacityAnimationToMainThreadDelayed(
      SingleKeyframeEffectAnimation* animation_to_receive_animation);
  void PostSetLocalSurfaceIdToMainThread(
      const viz::LocalSurfaceId& local_surface_id);
  void PostRequestNewLocalSurfaceIdToMainThread();
  void PostSetDeferCommitsToMainThread(bool defer_commits);
  void PostSetNeedsCommitToMainThread();
  void PostSetNeedsUpdateLayersToMainThread();
  void PostSetNeedsRedrawToMainThread();
  void PostSetNeedsRedrawRectToMainThread(const gfx::Rect& damage_rect);
  void PostSetVisibleToMainThread(bool visible);
  void PostSetNeedsCommitWithForcedRedrawToMainThread();
  void PostCompositeImmediatelyToMainThread();
  void PostNextCommitWaitsForActivationToMainThread();

  void DoBeginTest();
  void Timeout();

  AnimationHost* animation_host() const { return animation_host_.get(); }

 protected:
  LayerTreeTest();

  virtual void InitializeSettings(LayerTreeSettings* settings) {}

  void RealEndTest();

  std::unique_ptr<LayerTreeFrameSink>
  ReleaseLayerTreeFrameSinkOnLayerTreeHost();
  void SetVisibleOnLayerTreeHost(bool visible);
  void SetInitialDeviceScaleFactor(float initial_device_scale_factor) {
    initial_device_scale_factor_ = initial_device_scale_factor;
  }

  virtual void AfterTest() = 0;
  virtual void WillBeginTest();
  virtual void BeginTest() = 0;
  virtual void SetupTree();

  virtual void RunTest(CompositorMode mode);

  bool HasImplThread() const { return !!impl_thread_; }
  base::SingleThreadTaskRunner* ImplThreadTaskRunner() {
    return impl_task_runner_.get();
  }
  base::SingleThreadTaskRunner* MainThreadTaskRunner() {
    return main_task_runner_.get();
  }
  Proxy* proxy();
  TaskRunnerProvider* task_runner_provider() const;
  TaskGraphRunner* task_graph_runner() const {
    return task_graph_runner_.get();
  }
  bool TestEnded() const {
    base::AutoLock hold(test_ended_lock_);
    return ended_;
  }

  LayerTreeHost* layer_tree_host();
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager() {
    return gpu_memory_buffer_manager_.get();
  }

  void DestroyLayerTreeHost();

  // By default, output surface recreation is synchronous.
  void RequestNewLayerTreeFrameSink() override;
  // Override this and call the base class to change what viz::ContextProviders
  // will be used (such as for pixel tests). Or override it and create your own
  // TestLayerTreeFrameSink to control how it is created.
  virtual std::unique_ptr<viz::TestLayerTreeFrameSink> CreateLayerTreeFrameSink(
      const viz::RendererSettings& renderer_settings,
      double refresh_rate,
      scoped_refptr<viz::ContextProvider> compositor_context_provider,
      scoped_refptr<viz::RasterContextProvider> worker_context_provider);
  // Override this and call the base class to change what viz::ContextProvider
  // will be used, such as to prevent sharing the context with the
  // LayerTreeFrameSink. Or override it and create your own OutputSurface to
  // change what type of OutputSurface is used, such as a real OutputSurface for
  // pixel tests or a software-compositing OutputSurface.
  std::unique_ptr<viz::OutputSurface> CreateDisplayOutputSurfaceOnThread(
      scoped_refptr<viz::ContextProvider> compositor_context_provider) override;

  gfx::Vector2dF ScrollDelta(LayerImpl* layer_impl);

  base::SingleThreadTaskRunner* image_worker_task_runner() const {
    return image_worker_->task_runner().get();
  }

  void UseBeginFrameSource(viz::BeginFrameSource* begin_frame_source) {
    begin_frame_source_ = begin_frame_source;
  }

  bool use_skia_renderer_ = false;

 private:
  virtual void DispatchAddNoDamageAnimation(
      SingleKeyframeEffectAnimation* animation_to_receive_animation,
      double animation_duration);
  virtual void DispatchAddOpacityAnimation(
      SingleKeyframeEffectAnimation* animation_to_receive_animation,
      double animation_duration);
  void DispatchSetLocalSurfaceId(const viz::LocalSurfaceId& local_surface_id);
  void DispatchRequestNewLocalSurfaceId();
  void DispatchSetDeferCommits(bool defer_commits);
  void DispatchSetNeedsCommit();
  void DispatchSetNeedsUpdateLayers();
  void DispatchSetNeedsRedraw();
  void DispatchSetNeedsRedrawRect(const gfx::Rect& damage_rect);
  void DispatchSetVisible(bool visible);
  void DispatchSetNeedsCommitWithForcedRedraw();
  void DispatchDidAddAnimation();
  void DispatchCompositeImmediately();
  void DispatchNextCommitWaitsForActivation();

  LayerTreeSettings settings_;
  float initial_device_scale_factor_ = 1.f;

  CompositorMode mode_;

  std::unique_ptr<LayerTreeHostClientForTesting> client_;
  std::unique_ptr<LayerTreeHost> layer_tree_host_;
  std::unique_ptr<AnimationHost> animation_host_;

  bool beginning_ = false;
  bool end_when_begin_returns_ = false;
  bool timed_out_ = false;
  bool scheduled_ = false;
  bool started_ = false;

  mutable base::Lock test_ended_lock_;
  bool ended_ = false;

  int timeout_seconds_ = false;

  viz::BeginFrameSource* begin_frame_source_ = nullptr;  // NOT OWNED.

  std::unique_ptr<LayerTreeTestLayerTreeFrameSinkClient>
      layer_tree_frame_sink_client_;
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner_;
  std::unique_ptr<base::Thread> impl_thread_;
  std::unique_ptr<base::Thread> image_worker_;
  std::unique_ptr<viz::TestGpuMemoryBufferManager> gpu_memory_buffer_manager_;
  std::unique_ptr<TestTaskGraphRunner> task_graph_runner_;
  base::CancelableClosure timeout_;
  scoped_refptr<viz::TestContextProvider> compositor_contexts_;
  base::WeakPtr<LayerTreeTest> main_thread_weak_ptr_;
  base::WeakPtrFactory<LayerTreeTest> weak_factory_;
};

}  // namespace cc

// Do not change this macro to disable a test, it will disable half of
// the unit test suite. Instead, comment out the usage of this macro for
// a specific test name. eg.
// // TODO(crbug.com/abcd): Disabled for some reasons stated here.
// // SINGLE_AND_MULTI_THREAD_TEST_F(SomeRandomTest)
#define SINGLE_THREAD_TEST_F(TEST_FIXTURE_NAME)                   \
  TEST_F(TEST_FIXTURE_NAME, RunSingleThread_DelegatingRenderer) { \
    RunTest(CompositorMode::SINGLE_THREADED);                     \
  }                                                               \
  class SingleThreadDelegatingImplNeedsSemicolon##TEST_FIXTURE_NAME {}

// Do not change this macro to disable a test, it will disable half of
// the unit test suite. Instead, comment out the usage of this macro for
// a specific test name. eg.
// // TODO(crbug.com/abcd): Disabled for some reasons stated here.
// // SINGLE_AND_MULTI_THREAD_TEST_F(SomeRandomTest)
#define MULTI_THREAD_TEST_F(TEST_FIXTURE_NAME)                   \
  TEST_F(TEST_FIXTURE_NAME, RunMultiThread_DelegatingRenderer) { \
    RunTest(CompositorMode::THREADED);                           \
  }                                                              \
  class MultiThreadDelegatingImplNeedsSemicolon##TEST_FIXTURE_NAME {}

// Do not change this macro to disable a test, it will disable half of
// the unit test suite. Instead, comment out the usage of this macro for
// a specific test name. eg.
// // TODO(crbug.com/abcd): Disabled for some reasons stated here.
// // SINGLE_AND_MULTI_THREAD_TEST_F(SomeRandomTest)
#define SINGLE_AND_MULTI_THREAD_TEST_F(TEST_FIXTURE_NAME) \
  SINGLE_THREAD_TEST_F(TEST_FIXTURE_NAME);                \
  MULTI_THREAD_TEST_F(TEST_FIXTURE_NAME)

// Some tests want to control when notify ready for activation occurs,
// but this is not supported in the single-threaded case.
//
// Do not change this macro to disable a test, it will disable half of
// the unit test suite. Instead, comment out the usage of this macro for
// a specific test name. eg.
// // TODO(crbug.com/abcd): Disabled for some reasons stated here.
// // MULTI_THREAD_BLOCKNOTIFY_TEST_F(SomeRandomTest)
#define MULTI_THREAD_BLOCKNOTIFY_TEST_F(TEST_FIXTURE_NAME) \
  MULTI_THREAD_TEST_F(TEST_FIXTURE_NAME)

#endif  // CC_TEST_LAYER_TREE_TEST_H_
