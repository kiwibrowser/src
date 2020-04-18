// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_LAYER_TREE_HOST_H_
#define CC_TREES_LAYER_TREE_HOST_H_

#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback_forward.h"
#include "base/cancelable_callback.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "cc/benchmarks/micro_benchmark.h"
#include "cc/benchmarks/micro_benchmark_controller.h"
#include "cc/cc_export.h"
#include "cc/input/browser_controls_state.h"
#include "cc/input/event_listener_properties.h"
#include "cc/input/input_handler.h"
#include "cc/input/layer_selection_bound.h"
#include "cc/input/scrollbar.h"
#include "cc/layers/layer_collections.h"
#include "cc/layers/layer_list_iterator.h"
#include "cc/trees/compositor_mode.h"
#include "cc/trees/layer_tree_frame_sink.h"
#include "cc/trees/layer_tree_host_client.h"
#include "cc/trees/layer_tree_settings.h"
#include "cc/trees/mutator_host.h"
#include "cc/trees/proxy.h"
#include "cc/trees/swap_promise.h"
#include "cc/trees/swap_promise_manager.h"
#include "cc/trees/target_property.h"
#include "components/viz/common/resources/resource_format.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {
class HeadsUpDisplayLayer;
class Layer;
class LayerTreeHostClient;
class LayerTreeHostImpl;
class LayerTreeHostImplClient;
class LayerTreeHostSingleThreadClient;
class LayerTreeMutator;
class MutatorEvents;
class MutatorHost;
struct PendingPageScaleAnimation;
class RenderFrameMetadataObserver;
class RenderingStatsInstrumentation;
struct OverscrollBehavior;
class TaskGraphRunner;
class UIResourceManager;
class UkmRecorderFactory;
struct RenderingStats;
struct ScrollAndScaleSet;

class CC_EXPORT LayerTreeHost : public MutatorHostClient {
 public:
  struct CC_EXPORT InitParams {
    LayerTreeHostClient* client = nullptr;
    TaskGraphRunner* task_graph_runner = nullptr;
    LayerTreeSettings const* settings = nullptr;
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner;
    MutatorHost* mutator_host = nullptr;

    // The image worker task runner is used to schedule image decodes. The
    // compositor thread may make sync calls to this thread, analogous to the
    // raster worker threads.
    scoped_refptr<base::SequencedTaskRunner> image_worker_task_runner;

    std::unique_ptr<UkmRecorderFactory> ukm_recorder_factory;

    InitParams();
    ~InitParams();
  };

  static std::unique_ptr<LayerTreeHost> CreateThreaded(
      scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner,
      InitParams* params);

  static std::unique_ptr<LayerTreeHost> CreateSingleThreaded(
      LayerTreeHostSingleThreadClient* single_thread_client,
      InitParams* params);

  virtual ~LayerTreeHost();

  // Returns the process global unique identifier for this LayerTreeHost.
  int GetId() const;

  // The current source frame number. This is incremented for each main frame
  // update(commit) pushed to the compositor thread.
  int SourceFrameNumber() const;

  // Returns the UIResourceManager used to create UIResources for
  // UIResourceLayers pushed to the LayerTree.
  UIResourceManager* GetUIResourceManager() const;

  // Returns the TaskRunnerProvider used to access the main and compositor
  // thread task runners.
  TaskRunnerProvider* GetTaskRunnerProvider() const;

  // Returns the settings used by this host.
  const LayerTreeSettings& GetSettings() const;

  // Sets the LayerTreeMutator interface used to directly mutate the compositor
  // state on the compositor thread. (Compositor-Worker)
  void SetLayerTreeMutator(std::unique_ptr<LayerTreeMutator> mutator);

  // Call this function when you expect there to be a swap buffer.
  // See swap_promise.h for how to use SwapPromise.
  void QueueSwapPromise(std::unique_ptr<SwapPromise> swap_promise);

  // Returns the SwapPromiseManager used to create SwapPromiseMonitors for this
  // host.
  SwapPromiseManager* GetSwapPromiseManager();

  // Sets whether the content is suitable to use Gpu Rasterization.
  void SetHasGpuRasterizationTrigger(bool has_trigger);

  // Visibility and LayerTreeFrameSink -------------------------------

  void SetVisible(bool visible);
  bool IsVisible() const;

  // Called in response to a LayerTreeFrameSink request made to the client
  // using LayerTreeHostClient::RequestNewLayerTreeFrameSink. The client will
  // be informed of the LayerTreeFrameSink initialization status using
  // DidInitializaLayerTreeFrameSink or DidFailToInitializeLayerTreeFrameSink.
  // The request is completed when the host successfully initializes an
  // LayerTreeFrameSink.
  void SetLayerTreeFrameSink(
      std::unique_ptr<LayerTreeFrameSink> layer_tree_frame_sink);

  // Forces the host to immediately release all references to the
  // LayerTreeFrameSink, if any. Can be safely called any time, but the
  // compositor should not be visible.
  std::unique_ptr<LayerTreeFrameSink> ReleaseLayerTreeFrameSink();

  // Frame Scheduling (main and compositor frames) requests -------

  // Requests a main frame update even if no content has changed. This is used,
  // for instance in the case of RequestAnimationFrame from blink to ensure the
  // main frame update is run on the next tick without pre-emptively forcing a
  // full commit synchronization or layer updates.
  void SetNeedsAnimate();

  // Requests a main frame update and also ensure that the host pulls layer
  // updates from the client, even if no content might have changed, without
  // forcing a full commit synchronization.
  virtual void SetNeedsUpdateLayers();

  // Requests that the next main frame update performs a full commit
  // synchronization.
  virtual void SetNeedsCommit();

  // Requests that the next frame re-chooses crisp raster scales for all layers.
  void SetNeedsRecalculateRasterScales();

  // Returns true if a main frame with commit synchronization has been
  // requested.
  bool CommitRequested() const;

  // Enables/disables the compositor from requesting main frame updates from the
  // client.
  void SetDeferCommits(bool defer_commits);

  // Synchronously performs a main frame update and layer updates. Used only in
  // single threaded mode when the compositor's internal scheduling is disabled.
  void LayoutAndUpdateLayers();

  // Synchronously performs a complete main frame update, commit and compositor
  // frame. Used only in single threaded mode when the compositor's internal
  // scheduling is disabled.
  void Composite(base::TimeTicks frame_begin_time, bool raster);

  // Requests a redraw (compositor frame) for the given rect.
  void SetNeedsRedrawRect(const gfx::Rect& damage_rect);

  // Requests a main frame (including layer updates) and ensures that this main
  // frame results in a redraw for the complete viewport when producing the
  // CompositorFrame.
  void SetNeedsCommitWithForcedRedraw();

  // Input Handling ---------------------------------------------

  // Notifies the compositor that input from the browser is being throttled till
  // the next commit. The compositor will prioritize activation of the pending
  // tree so a commit can be performed.
  void NotifyInputThrottledUntilCommit();

  // Sets the state of the browser controls. (Used for URL bar animations on
  // android).
  void UpdateBrowserControlsState(BrowserControlsState constraints,
                                  BrowserControlsState current,
                                  bool animate);

  // Returns a reference to the InputHandler used to respond to input events on
  // the compositor thread.
  const base::WeakPtr<InputHandler>& GetInputHandler() const;

  // Informs the compositor that an active fling gesture being processed on the
  // main thread has been finished.
  void DidStopFlinging();

  // Debugging and benchmarks ---------------------------------
  void SetDebugState(const LayerTreeDebugState& debug_state);
  const LayerTreeDebugState& GetDebugState() const;

  // Returns the id of the benchmark on success, 0 otherwise.
  int ScheduleMicroBenchmark(const std::string& benchmark_name,
                             std::unique_ptr<base::Value> value,
                             MicroBenchmark::DoneCallback callback);

  // Returns true if the message was successfully delivered and handled.
  bool SendMessageToMicroBenchmark(int id, std::unique_ptr<base::Value> value);

  // When the main thread informs the impl thread that it is ready to commit,
  // generally it would remain blocked till the main thread state is copied to
  // the pending tree. Calling this would ensure that the main thread remains
  // blocked till the pending tree is activated.
  void SetNextCommitWaitsForActivation();

  // The LayerTreeHost tracks whether the content is suitable for Gpu raster.
  // Calling this will reset it back to not suitable state.
  void ResetGpuRasterizationTracking();

  // Registers a callback that is run when the next frame successfully makes it
  // to the screen (it's entirely possible some frames may be dropped between
  // the time this is called and the callback is run).
  using PresentationTimeCallback =
      base::OnceCallback<void(base::TimeTicks, base::TimeDelta, uint32_t)>;
  void RequestPresentationTimeForNextFrame(PresentationTimeCallback callback);

  void SetRootLayer(scoped_refptr<Layer> root_layer);
  Layer* root_layer() { return root_layer_.get(); }
  const Layer* root_layer() const { return root_layer_.get(); }

  struct CC_EXPORT ViewportLayers {
    ViewportLayers();
    ~ViewportLayers();
    scoped_refptr<Layer> overscroll_elasticity;
    scoped_refptr<Layer> page_scale;
    scoped_refptr<Layer> inner_viewport_container;
    scoped_refptr<Layer> outer_viewport_container;
    scoped_refptr<Layer> inner_viewport_scroll;
    scoped_refptr<Layer> outer_viewport_scroll;
  };
  void RegisterViewportLayers(const ViewportLayers& viewport_layers);
  Layer* overscroll_elasticity_layer() const {
    return viewport_layers_.overscroll_elasticity.get();
  }
  Layer* page_scale_layer() const { return viewport_layers_.page_scale.get(); }
  Layer* inner_viewport_container_layer() const {
    return viewport_layers_.inner_viewport_container.get();
  }
  Layer* outer_viewport_container_layer() const {
    return viewport_layers_.outer_viewport_container.get();
  }
  Layer* inner_viewport_scroll_layer() const {
    return viewport_layers_.inner_viewport_scroll.get();
  }
  Layer* outer_viewport_scroll_layer() const {
    return viewport_layers_.outer_viewport_scroll.get();
  }

  void RegisterSelection(const LayerSelection& selection);
  const LayerSelection& selection() const { return selection_; }

  void SetHaveScrollEventHandlers(bool have_event_handlers);
  bool have_scroll_event_handlers() const {
    return have_scroll_event_handlers_;
  }

  void SetEventListenerProperties(EventListenerClass event_class,
                                  EventListenerProperties event_properties);
  EventListenerProperties event_listener_properties(
      EventListenerClass event_class) const {
    return event_listener_properties_[static_cast<size_t>(event_class)];
  }

  void SetViewportSizeAndScale(
      const gfx::Size& device_viewport_size,
      float device_scale_factor,
      const viz::LocalSurfaceId& local_surface_id_from_parent);

  void SetViewportVisibleRect(const gfx::Rect& visible_rect);

  gfx::Rect viewport_visible_rect() const { return viewport_visible_rect_; }

  gfx::Size device_viewport_size() const { return device_viewport_size_; }

  void SetBrowserControlsHeight(float top_height,
                                float bottom_height,
                                bool shrink);
  void SetBrowserControlsShownRatio(float ratio);
  void SetOverscrollBehavior(const OverscrollBehavior& overscroll_behavior);

  void SetPageScaleFactorAndLimits(float page_scale_factor,
                                   float min_page_scale_factor,
                                   float max_page_scale_factor);
  float page_scale_factor() const { return page_scale_factor_; }
  float min_page_scale_factor() const { return min_page_scale_factor_; }
  float max_page_scale_factor() const { return max_page_scale_factor_; }

  void set_background_color(SkColor color) { background_color_ = color; }
  SkColor background_color() const { return background_color_; }

  void StartPageScaleAnimation(const gfx::Vector2d& target_offset,
                               bool use_anchor,
                               float scale,
                               base::TimeDelta duration);
  bool HasPendingPageScaleAnimation() const;

  float device_scale_factor() const { return device_scale_factor_; }

  void SetRecordingScaleFactor(float recording_scale_factor);

  float painted_device_scale_factor() const {
    return painted_device_scale_factor_;
  }

  void SetContentSourceId(uint32_t);
  uint32_t content_source_id() const { return content_source_id_; }

  // If this LayerTreeHost needs a valid viz::LocalSurfaceId then commits will
  // be deferred until a valid viz::LocalSurfaceId is provided.
  void SetLocalSurfaceIdFromParent(
      const viz::LocalSurfaceId& local_surface_id_from_parent);
  const viz::LocalSurfaceId& local_surface_id_from_parent() const {
    return local_surface_id_from_parent_;
  }

  // Requests the allocation of a new LocalSurfaceId on the compositor thread.
  void RequestNewLocalSurfaceId();

  // Returns the current state of the new LocalSurfaceId request and resets
  // the state.
  bool TakeNewLocalSurfaceIdRequest();
  bool new_local_surface_id_request_for_testing() const {
    return new_local_surface_id_request_;
  }

  void SetRasterColorSpace(const gfx::ColorSpace& raster_color_space);
  const gfx::ColorSpace& raster_color_space() const {
    return raster_color_space_;
  }

  // Used externally by blink for setting the PropertyTrees when
  // UseLayerLists() is true, which also implies that Slimming Paint
  // v2 is enabled.
  PropertyTrees* property_trees() { return &property_trees_; }
  const PropertyTrees* property_trees() const { return &property_trees_; }

  void SetNeedsDisplayOnAllLayers();

  void RegisterLayer(Layer* layer);
  void UnregisterLayer(Layer* layer);
  Layer* LayerById(int id) const;

  size_t NumLayers() const;

  bool in_update_property_trees() const { return in_update_property_trees_; }
  bool PaintContent(const LayerList& update_layer_list,
                    bool* content_has_slow_paths,
                    bool* content_has_non_aa_paint);
  bool in_paint_layer_contents() const { return in_paint_layer_contents_; }

  void SetHasCopyRequest(bool has_copy_request);
  bool has_copy_request() const { return has_copy_request_; }

  void AddSurfaceLayerId(const viz::SurfaceId& surface_id);
  void RemoveSurfaceLayerId(const viz::SurfaceId& surface_id);
  base::flat_set<viz::SurfaceId> SurfaceLayerIds() const;

  void AddLayerShouldPushProperties(Layer* layer);
  void RemoveLayerShouldPushProperties(Layer* layer);
  std::unordered_set<Layer*>& LayersThatShouldPushProperties();
  bool LayerNeedsPushPropertiesForTesting(Layer* layer) const;

  void SetPageScaleFromImplSide(float page_scale);
  void SetElasticOverscrollFromImplSide(gfx::Vector2dF elastic_overscroll);
  gfx::Vector2dF elastic_overscroll() const { return elastic_overscroll_; }

  void UpdateHudLayer(bool show_hud_info);
  HeadsUpDisplayLayer* hud_layer() const { return hud_layer_.get(); }

  virtual void SetNeedsFullTreeSync();
  bool needs_full_tree_sync() const { return needs_full_tree_sync_; }

  bool needs_surface_ids_sync() const { return needs_surface_ids_sync_; }
  void set_needs_surface_ids_sync(bool needs_surface_ids_sync) {
    needs_surface_ids_sync_ = needs_surface_ids_sync;
  }

  void SetPropertyTreesNeedRebuild();

  void PushPropertyTreesTo(LayerTreeImpl* tree_impl);
  void PushLayerTreePropertiesTo(LayerTreeImpl* tree_impl);
  void PushSurfaceIdsTo(LayerTreeImpl* tree_impl);
  void PushLayerTreeHostPropertiesTo(LayerTreeHostImpl* host_impl);

  MutatorHost* mutator_host() const { return mutator_host_; }

  Layer* LayerByElementId(ElementId element_id) const;
  void RegisterElement(ElementId element_id,
                       ElementListType list_type,
                       Layer* layer);
  void UnregisterElement(ElementId element_id, ElementListType list_type);
  void SetElementIdsForTesting();

  void BuildPropertyTreesForTesting();

  // Layer iterators.
  LayerListIterator<Layer> begin() const;
  LayerListIterator<Layer> end() const;
  LayerListReverseIterator<Layer> rbegin();
  LayerListReverseIterator<Layer> rend();

  // LayerTreeHost interface to Proxy.
  void WillBeginMainFrame();
  void DidBeginMainFrame();
  void BeginMainFrame(const viz::BeginFrameArgs& args);
  void BeginMainFrameNotExpectedSoon();
  void BeginMainFrameNotExpectedUntil(base::TimeTicks time);
  void AnimateLayers(base::TimeTicks monotonic_frame_begin_time);
  using VisualStateUpdate = LayerTreeHostClient::VisualStateUpdate;
  void RequestMainFrameUpdate(
      VisualStateUpdate requested_update = VisualStateUpdate::kAll);
  void FinishCommitOnImplThread(LayerTreeHostImpl* host_impl);
  void WillCommit();
  void CommitComplete();
  void RequestNewLayerTreeFrameSink();
  void DidInitializeLayerTreeFrameSink();
  void DidFailToInitializeLayerTreeFrameSink();
  virtual std::unique_ptr<LayerTreeHostImpl> CreateLayerTreeHostImpl(
      LayerTreeHostImplClient* client);
  void DidLoseLayerTreeFrameSink();
  void DidCommitAndDrawFrame() { client_->DidCommitAndDrawFrame(); }
  void DidReceiveCompositorFrameAck() {
    client_->DidReceiveCompositorFrameAck();
  }
  bool UpdateLayers();
  void DidPresentCompositorFrame(const std::vector<int>& source_frames,
                                 base::TimeTicks time,
                                 base::TimeDelta refresh,
                                 uint32_t flags);
  // Called when the compositor completed page scale animation.
  void DidCompletePageScaleAnimation();
  void ApplyScrollAndScale(ScrollAndScaleSet* info);

  LayerTreeHostClient* client() { return client_; }

  bool gpu_rasterization_histogram_recorded() const {
    return gpu_rasterization_histogram_recorded_;
  }

  void CollectRenderingStats(RenderingStats* stats) const;

  RenderingStatsInstrumentation* rendering_stats_instrumentation() const {
    return rendering_stats_instrumentation_.get();
  }

  void SetAnimationEvents(std::unique_ptr<MutatorEvents> events);

  bool has_gpu_rasterization_trigger() const {
    return has_gpu_rasterization_trigger_;
  }

  Proxy* proxy() const { return proxy_.get(); }

  bool IsSingleThreaded() const;
  bool IsThreaded() const;

  // Indicates whether this host is configured to use layer lists
  // rather than layer trees. This also implies that property trees
  // are always already built and so cc doesn't have to build them.
  bool IsUsingLayerLists() const;

  // MutatorHostClient implementation.
  bool IsElementInList(ElementId element_id,
                       ElementListType list_type) const override;
  void SetMutatorsNeedCommit() override;
  void SetMutatorsNeedRebuildPropertyTrees() override;
  void SetElementFilterMutated(ElementId element_id,
                               ElementListType list_type,
                               const FilterOperations& filters) override;
  void SetElementOpacityMutated(ElementId element_id,
                                ElementListType list_type,
                                float opacity) override;
  void SetElementTransformMutated(ElementId element_id,
                                  ElementListType list_type,
                                  const gfx::Transform& transform) override;
  void SetElementScrollOffsetMutated(
      ElementId element_id,
      ElementListType list_type,
      const gfx::ScrollOffset& scroll_offset) override;

  void ElementIsAnimatingChanged(ElementId element_id,
                                 ElementListType list_type,
                                 const PropertyAnimationState& mask,
                                 const PropertyAnimationState& state) override;

  void ScrollOffsetAnimationFinished() override {}
  gfx::ScrollOffset GetScrollOffsetForAnimation(
      ElementId element_id) const override;

  void QueueImageDecode(const PaintImage& image,
                        base::OnceCallback<void(bool)> callback);
  void ImageDecodesFinished(const std::vector<std::pair<int, bool>>& results);

  void RequestBeginMainFrameNotExpected(bool new_state);

  float recording_scale_factor() const { return recording_scale_factor_; }

  void SetURLForUkm(const GURL& url);

  void SetRenderFrameObserver(
      std::unique_ptr<RenderFrameMetadataObserver> observer);

 protected:
  LayerTreeHost(InitParams* params, CompositorMode mode);

  void InitializeThreaded(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> impl_task_runner);
  void InitializeSingleThreaded(
      LayerTreeHostSingleThreadClient* single_thread_client,
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner);
  void InitializeForTesting(
      std::unique_ptr<TaskRunnerProvider> task_runner_provider,
      std::unique_ptr<Proxy> proxy_for_testing);
  void SetTaskRunnerProviderForTesting(
      std::unique_ptr<TaskRunnerProvider> task_runner_provider);
  void SetUIResourceManagerForTesting(
      std::unique_ptr<UIResourceManager> ui_resource_manager);

  // task_graph_runner() returns a valid value only until the LayerTreeHostImpl
  // is created in CreateLayerTreeHostImpl().
  TaskGraphRunner* task_graph_runner() const { return task_graph_runner_; }

  void OnCommitForSwapPromises();

  void RecordGpuRasterizationHistogram(const LayerTreeHostImpl* host_impl);

  MicroBenchmarkController micro_benchmark_controller_;

  base::WeakPtr<InputHandler> input_handler_weak_ptr_;

  scoped_refptr<base::SequencedTaskRunner> image_worker_task_runner_;
  std::unique_ptr<UkmRecorderFactory> ukm_recorder_factory_;

 private:
  friend class LayerTreeHostSerializationTest;

  // This is the number of consecutive frames in which we want the content to be
  // free of slow-paths before toggling the flag.
  enum { kNumFramesToConsiderBeforeRemovingSlowPathFlag = 60 };

  void ApplyViewportDeltas(ScrollAndScaleSet* info);
  void RecordWheelAndTouchScrollingCount(ScrollAndScaleSet* info);
  void ApplyPageScaleDeltaFromImplSide(float page_scale_delta);
  void InitializeProxy(std::unique_ptr<Proxy> proxy);

  bool DoUpdateLayers(Layer* root_layer);

  void UpdateDeferCommitsInternal();

  const CompositorMode compositor_mode_;

  std::unique_ptr<UIResourceManager> ui_resource_manager_;

  LayerTreeHostClient* client_;
  std::unique_ptr<Proxy> proxy_;
  std::unique_ptr<TaskRunnerProvider> task_runner_provider_;

  int source_frame_number_ = 0U;
  std::unique_ptr<RenderingStatsInstrumentation>
      rendering_stats_instrumentation_;

  SwapPromiseManager swap_promise_manager_;

  // |current_layer_tree_frame_sink_| can't be updated until we've successfully
  // initialized a new LayerTreeFrameSink. |new_layer_tree_frame_sink_|
  // contains the new LayerTreeFrameSink that is currently being initialized.
  // If initialization is successful then |new_layer_tree_frame_sink_| replaces
  // |current_layer_tree_frame_sink_|.
  std::unique_ptr<LayerTreeFrameSink> new_layer_tree_frame_sink_;
  std::unique_ptr<LayerTreeFrameSink> current_layer_tree_frame_sink_;

  const LayerTreeSettings settings_;
  LayerTreeDebugState debug_state_;

  bool visible_ = false;

  bool has_gpu_rasterization_trigger_ = false;
  bool content_has_slow_paths_ = false;
  bool content_has_non_aa_paint_ = false;
  bool gpu_rasterization_histogram_recorded_ = false;

  // If set, then page scale animation has completed, but the client hasn't been
  // notified about it yet.
  bool did_complete_scale_animation_ = false;

  int id_;
  bool next_commit_forces_redraw_ = false;
  bool next_commit_forces_recalculate_raster_scales_ = false;
  // Track when we're inside a main frame to see if compositor is being
  // destroyed midway which causes a crash. crbug.com/654672
  bool inside_main_frame_ = false;

  TaskGraphRunner* task_graph_runner_;

  uint32_t num_consecutive_frames_without_slow_paths_ = 0;

  scoped_refptr<Layer> root_layer_;

  ViewportLayers viewport_layers_;

  float top_controls_height_ = 0.f;
  float top_controls_shown_ratio_ = 0.f;
  bool browser_controls_shrink_blink_size_ = false;
  OverscrollBehavior overscroll_behavior_;

  float bottom_controls_height_ = 0.f;

  float device_scale_factor_ = 1.f;
  float painted_device_scale_factor_ = 1.f;
  float recording_scale_factor_ = 1.f;
  float page_scale_factor_ = 1.f;
  float min_page_scale_factor_ = 1.f;
  float max_page_scale_factor_ = 1.f;

  int raster_color_space_id_ = -1;
  gfx::ColorSpace raster_color_space_;

  uint32_t content_source_id_;
  viz::LocalSurfaceId local_surface_id_from_parent_;
  // Used to detect surface invariant violations.
  bool has_pushed_local_surface_id_from_parent_ = false;
  bool new_local_surface_id_request_ = false;
  bool defer_commits_ = false;

  SkColor background_color_ = SK_ColorWHITE;

  LayerSelection selection_;

  gfx::Size device_viewport_size_;

  gfx::Rect viewport_visible_rect_;

  bool have_scroll_event_handlers_ = false;
  EventListenerProperties event_listener_properties_[static_cast<size_t>(
      EventListenerClass::kNumClasses)];

  std::unique_ptr<PendingPageScaleAnimation> pending_page_scale_animation_;

  PropertyTrees property_trees_;

  bool needs_full_tree_sync_ = true;

  bool needs_surface_ids_sync_ = false;

  gfx::Vector2dF elastic_overscroll_;

  scoped_refptr<HeadsUpDisplayLayer> hud_layer_;

  // The number of SurfaceLayers that have fallback set to viz::SurfaceId.
  base::flat_map<viz::SurfaceId, int> surface_layer_ids_;

  // Set of layers that need to push properties.
  std::unordered_set<Layer*> layers_that_should_push_properties_;

  // Layer id to Layer map.
  std::unordered_map<int, Layer*> layer_id_map_;

  std::unordered_map<ElementId, Layer*, ElementIdHash> element_layers_map_;

  bool in_paint_layer_contents_ = false;
  bool in_update_property_trees_ = false;

  // This is true if atleast one layer in the layer tree has a copy request. We
  // use this bool to decide whether we need to compute subtree has copy request
  // for every layer during property tree building.
  bool has_copy_request_ = false;

  MutatorHost* mutator_host_;

  std::vector<std::pair<PaintImage, base::OnceCallback<void(bool)>>>
      queued_image_decodes_;
  std::unordered_map<int, base::OnceCallback<void(bool)>>
      pending_image_decodes_;

  // Presentation time callbacks requested for the next frame are initially
  // added here.
  std::vector<PresentationTimeCallback> pending_presentation_time_callbacks_;

  // Maps from the source frame presentation callbacks are requested for to
  // the callbacks.
  std::map<int, std::vector<PresentationTimeCallback>>
      frame_to_presentation_time_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(LayerTreeHost);
};

}  // namespace cc

#endif  // CC_TREES_LAYER_TREE_HOST_H_
