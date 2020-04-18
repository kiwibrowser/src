// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/gpu/render_widget_compositor.h"

#include <stddef.h>

#include <cmath>
#include <limits>
#include <string>
#include <utility>

#include "base/auto_reset.h"
#include "base/base_switches.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/lock.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "base/values.h"
#include "build/build_config.h"
#include "cc/animation/animation_host.h"
#include "cc/animation/animation_timeline.h"
#include "cc/base/region.h"
#include "cc/base/switches.h"
#include "cc/benchmarks/micro_benchmark.h"
#include "cc/debug/layer_tree_debug_state.h"
#include "cc/input/layer_selection_bound.h"
#include "cc/layers/layer.h"
#include "cc/trees/latency_info_swap_promise.h"
#include "cc/trees/latency_info_swap_promise_monitor.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/layer_tree_mutator.h"
#include "cc/trees/render_frame_metadata_observer.h"
#include "cc/trees/swap_promise.h"
#include "cc/trees/ukm_manager.h"
#include "components/viz/common/features.h"
#include "components/viz/common/frame_sinks/begin_frame_args.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/frame_sinks/copy_output_request.h"
#include "components/viz/common/frame_sinks/copy_output_result.h"
#include "components/viz/common/resources/single_release_callback.h"
#include "components/viz/common/switches.h"
#include "content/common/content_switches_internal.h"
#include "content/common/layer_tree_settings_factory.h"
#include "content/common/render_frame_metadata.mojom.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/screen_info.h"
#include "content/public/common/use_zoom_for_dsf_policy.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/renderer/gpu/render_widget_compositor_delegate.h"
#include "content/renderer/render_frame_metadata_observer_impl.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/service/gpu_switches.h"
#include "media/base/media_switches.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "third_party/blink/public/platform/scheduler/web_main_thread_scheduler.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_selection.h"
#include "third_party/skia/include/core/SkImage.h"
#include "ui/base/ui_base_switches.h"
#include "ui/gfx/switches.h"
#include "ui/gl/gl_switches.h"
#include "ui/native_theme/native_theme_features.h"
#include "ui/native_theme/overlay_scrollbar_constants_aura.h"

namespace base {
class Value;
}

namespace cc {
class Layer;
}

namespace content {
namespace {

const base::Feature kUnpremultiplyAndDitherLowBitDepthTiles = {
    "UnpremultiplyAndDitherLowBitDepthTiles", base::FEATURE_ENABLED_BY_DEFAULT};

using ReportTimeCallback = blink::WebLayerTreeView::ReportTimeCallback;

class ReportTimeSwapPromise : public cc::SwapPromise {
 public:
  ReportTimeSwapPromise(
      ReportTimeCallback callback,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~ReportTimeSwapPromise() override;

  void DidActivate() override {}
  void WillSwap(viz::CompositorFrameMetadata* metadata,
                cc::FrameTokenAllocator* frame_token_allocator) override {}
  void DidSwap() override;
  DidNotSwapAction DidNotSwap(DidNotSwapReason reason) override;

  int64_t TraceId() const override;

 private:
  ReportTimeCallback callback_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ReportTimeSwapPromise);
};

ReportTimeSwapPromise::ReportTimeSwapPromise(
    ReportTimeCallback callback,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : callback_(std::move(callback)), task_runner_(std::move(task_runner)) {}

ReportTimeSwapPromise::~ReportTimeSwapPromise() {}

void ReportTimeSwapPromise::DidSwap() {
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback_),
                                blink::WebLayerTreeView::SwapResult::kDidSwap,
                                base::TimeTicks::Now()));
}

cc::SwapPromise::DidNotSwapAction ReportTimeSwapPromise::DidNotSwap(
    cc::SwapPromise::DidNotSwapReason reason) {
  blink::WebLayerTreeView::SwapResult result;
  switch (reason) {
    case cc::SwapPromise::DidNotSwapReason::SWAP_FAILS:
      result = blink::WebLayerTreeView::SwapResult::kDidNotSwapSwapFails;
      break;
    case cc::SwapPromise::DidNotSwapReason::COMMIT_FAILS:
      result = blink::WebLayerTreeView::SwapResult::kDidNotSwapCommitFails;
      break;
    case cc::SwapPromise::DidNotSwapReason::COMMIT_NO_UPDATE:
      result = blink::WebLayerTreeView::SwapResult::kDidNotSwapCommitNoUpdate;
      break;
    case cc::SwapPromise::DidNotSwapReason::ACTIVATION_FAILS:
      result = blink::WebLayerTreeView::SwapResult::kDidNotSwapActivationFails;
      break;
  }
  task_runner_->PostTask(FROM_HERE, base::BindOnce(std::move(callback_), result,
                                                   base::TimeTicks::Now()));
  return cc::SwapPromise::DidNotSwapAction::BREAK_PROMISE;
}

int64_t ReportTimeSwapPromise::TraceId() const {
  return 0;
}

bool GetSwitchValueAsInt(const base::CommandLine& command_line,
                         const std::string& switch_string,
                         int min_value,
                         int max_value,
                         int* result) {
  std::string string_value = command_line.GetSwitchValueASCII(switch_string);
  int int_value;
  if (base::StringToInt(string_value, &int_value) && int_value >= min_value &&
      int_value <= max_value) {
    *result = int_value;
    return true;
  } else {
    LOG(WARNING) << "Failed to parse switch " << switch_string << ": "
                 << string_value;
    return false;
  }
}

gfx::SelectionBound::Type ConvertFromWebSelectionBoundType(
    blink::WebSelectionBound::Type type) {
  if (type == blink::WebSelectionBound::Type::kSelectionLeft)
    return gfx::SelectionBound::Type::LEFT;
  if (type == blink::WebSelectionBound::Type::kSelectionRight)
    return gfx::SelectionBound::Type::RIGHT;
  // if WebSelection is not a range (caret or none),
  // The type of gfx::SelectionBound should be CENTER.
  DCHECK_EQ(type, blink::WebSelectionBound::Type::kCaret);
  return gfx::SelectionBound::Type::CENTER;
}

cc::LayerSelectionBound ConvertFromWebSelectionBound(
    const blink::WebSelectionBound& bound) {
  cc::LayerSelectionBound cc_bound;
  DCHECK(bound.layer_id);

  cc_bound.type = ConvertFromWebSelectionBoundType(bound.type);
  cc_bound.layer_id = bound.layer_id;
  cc_bound.edge_top = gfx::Point(bound.edge_top_in_layer);
  cc_bound.edge_bottom = gfx::Point(bound.edge_bottom_in_layer);
  cc_bound.hidden = bound.hidden;
  return cc_bound;
}

cc::LayerSelection ConvertFromWebSelection(
    const blink::WebSelection& web_selection) {
  if (web_selection.IsNone())
    return cc::LayerSelection();
  cc::LayerSelection cc_selection;
  cc_selection.start = ConvertFromWebSelectionBound(web_selection.Start());
  cc_selection.end = ConvertFromWebSelectionBound(web_selection.end());
  return cc_selection;
}

gfx::Size CalculateDefaultTileSize(const ScreenInfo& screen_info) {
  int default_tile_size = 256;
#if defined(OS_ANDROID)
  const gfx::Size screen_size = gfx::ScaleToFlooredSize(
      screen_info.rect.size(), screen_info.device_scale_factor);
  int display_width = screen_size.width();
  int display_height = screen_size.height();
  int numTiles = (display_width * display_height) / (256 * 256);
  if (numTiles > 16)
    default_tile_size = 384;
  if (numTiles >= 40)
    default_tile_size = 512;

  // Adjust for some resolutions that barely straddle an extra
  // tile when in portrait mode. This helps worst case scroll/raster
  // by not needing a full extra tile for each row.
  constexpr int tolerance = 10;  // To avoid rounding errors.
  int portrait_width = std::min(display_width, display_height);
  if (default_tile_size == 256 && std::abs(portrait_width - 768) < tolerance)
    default_tile_size += 32;
  if (default_tile_size == 384 && std::abs(portrait_width - 1200) < tolerance)
    default_tile_size += 32;
#elif defined(OS_CHROMEOS) || defined(OS_MACOSX)
  // Use 512 for high DPI (dsf=2.0f) devices.
  if (screen_info.device_scale_factor >= 2.0f)
    default_tile_size = 512;
#endif

  return gfx::Size(default_tile_size, default_tile_size);
}

// Check cc::BrowserControlsState, and blink::WebBrowserControlsState
// are kept in sync.
static_assert(int(blink::kWebBrowserControlsBoth) == int(cc::BOTH),
              "mismatching enums: BOTH");
static_assert(int(blink::kWebBrowserControlsHidden) == int(cc::HIDDEN),
              "mismatching enums: HIDDEN");
static_assert(int(blink::kWebBrowserControlsShown) == int(cc::SHOWN),
              "mismatching enums: SHOWN");

static cc::BrowserControlsState ConvertBrowserControlsState(
    blink::WebBrowserControlsState state) {
  return static_cast<cc::BrowserControlsState>(state);
}

}  // namespace

// static
std::unique_ptr<RenderWidgetCompositor> RenderWidgetCompositor::Create(
    RenderWidgetCompositorDelegate* delegate,
    CompositorDependencies* compositor_deps) {
  std::unique_ptr<RenderWidgetCompositor> compositor(
      new RenderWidgetCompositor(delegate, compositor_deps));
  return compositor;
}

RenderWidgetCompositor::RenderWidgetCompositor(
    RenderWidgetCompositorDelegate* delegate,
    CompositorDependencies* compositor_deps)
    : delegate_(delegate),
      compositor_deps_(compositor_deps),
      threaded_(!!compositor_deps_->GetCompositorImplThreadTaskRunner()),
      never_visible_(false),
      is_for_oopif_(false),
      weak_factory_(this) {}

void RenderWidgetCompositor::Initialize(
    std::unique_ptr<cc::LayerTreeHost> layer_tree_host,
    std::unique_ptr<cc::AnimationHost> animation_host) {
  DCHECK(layer_tree_host);
  DCHECK(animation_host);
  animation_host_ = std::move(animation_host);
  layer_tree_host_ = std::move(layer_tree_host);
}

RenderWidgetCompositor::~RenderWidgetCompositor() = default;

// static
std::unique_ptr<cc::LayerTreeHost> RenderWidgetCompositor::CreateLayerTreeHost(
    LayerTreeHostClient* client,
    cc::LayerTreeHostSingleThreadClient* single_thread_client,
    cc::MutatorHost* mutator_host,
    CompositorDependencies* deps,
    const ScreenInfo& screen_info) {
  base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
  const bool is_threaded = !!deps->GetCompositorImplThreadTaskRunner();
  cc::LayerTreeSettings settings = GenerateLayerTreeSettings(
      *cmd, deps, client->IsForSubframe(), screen_info, is_threaded);

  std::unique_ptr<cc::LayerTreeHost> layer_tree_host;

  cc::LayerTreeHost::InitParams params;
  params.client = client;
  params.settings = &settings;
  params.task_graph_runner = deps->GetTaskGraphRunner();
  params.main_task_runner = deps->GetCompositorMainThreadTaskRunner();
  params.mutator_host = mutator_host;
  params.ukm_recorder_factory = deps->CreateUkmRecorderFactory();
  if (base::TaskScheduler::GetInstance()) {
    // The image worker thread needs to allow waiting since it makes discardable
    // shared memory allocations which need to make synchronous calls to the
    // IO thread.
    params.image_worker_task_runner = base::CreateSequencedTaskRunnerWithTraits(
        {base::WithBaseSyncPrimitives(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN});
  }
  if (!is_threaded) {
    // Single-threaded layout tests.
    layer_tree_host =
        cc::LayerTreeHost::CreateSingleThreaded(single_thread_client, &params);
  } else {
    layer_tree_host = cc::LayerTreeHost::CreateThreaded(
        deps->GetCompositorImplThreadTaskRunner(), &params);
  }

  return layer_tree_host;
}

// static
cc::LayerTreeSettings RenderWidgetCompositor::GenerateLayerTreeSettings(
    const base::CommandLine& cmd,
    CompositorDependencies* compositor_deps,
    bool is_for_subframe,
    const ScreenInfo& screen_info,
    bool is_threaded) {
  cc::LayerTreeSettings settings;

  settings.resource_settings.use_r16_texture =
      base::FeatureList::IsEnabled(media::kUseR16Texture);

  settings.commit_to_active_tree = !is_threaded;
  settings.is_layer_tree_for_subframe = is_for_subframe;

  // For web contents, layer transforms should scale up the contents of layers
  // to keep content always crisp when possible.
  settings.layer_transforms_should_scale_layer_contents = true;

  settings.main_frame_before_activation_enabled =
      cmd.HasSwitch(cc::switches::kEnableMainFrameBeforeActivation);

  // Checkerimaging is not supported for synchronous single-threaded mode, which
  // is what the renderer uses if its not threaded.
  settings.enable_checker_imaging =
      !cmd.HasSwitch(cc::switches::kDisableCheckerImaging) && is_threaded;

#if defined(OS_ANDROID)
  // We can use a more aggressive limit on Android since decodes tend to take
  // longer on these devices.
  settings.min_image_bytes_to_checker = 512 * 1024;  // 512kB

  // Re-rasterization of checker-imaged content with software raster can be too
  // costly on Android.
  settings.only_checker_images_with_gpu_raster = true;
#endif

  // TODO(danakj): This should not be a setting O_O; it should change when the
  // device scale factor on LayerTreeHost changes.
  settings.default_tile_size = CalculateDefaultTileSize(screen_info);
  if (cmd.HasSwitch(switches::kDefaultTileWidth)) {
    int tile_width = 0;
    GetSwitchValueAsInt(cmd, switches::kDefaultTileWidth, 1,
                        std::numeric_limits<int>::max(), &tile_width);
    settings.default_tile_size.set_width(tile_width);
  }
  if (cmd.HasSwitch(switches::kDefaultTileHeight)) {
    int tile_height = 0;
    GetSwitchValueAsInt(cmd, switches::kDefaultTileHeight, 1,
                        std::numeric_limits<int>::max(), &tile_height);
    settings.default_tile_size.set_height(tile_height);
  }

  int max_untiled_layer_width = settings.max_untiled_layer_size.width();
  if (cmd.HasSwitch(switches::kMaxUntiledLayerWidth)) {
    GetSwitchValueAsInt(cmd, switches::kMaxUntiledLayerWidth, 1,
                        std::numeric_limits<int>::max(),
                        &max_untiled_layer_width);
  }
  int max_untiled_layer_height = settings.max_untiled_layer_size.height();
  if (cmd.HasSwitch(switches::kMaxUntiledLayerHeight)) {
    GetSwitchValueAsInt(cmd, switches::kMaxUntiledLayerHeight, 1,
                        std::numeric_limits<int>::max(),
                        &max_untiled_layer_height);
  }

  settings.max_untiled_layer_size =
      gfx::Size(max_untiled_layer_width, max_untiled_layer_height);

  settings.gpu_rasterization_msaa_sample_count =
      compositor_deps->GetGpuRasterizationMSAASampleCount();
  settings.gpu_rasterization_forced =
      compositor_deps->IsGpuRasterizationForced();

  settings.can_use_lcd_text = compositor_deps->IsLcdTextEnabled();
  settings.use_zero_copy = compositor_deps->IsZeroCopyEnabled();
  settings.use_partial_raster = compositor_deps->IsPartialRasterEnabled();
  settings.enable_elastic_overscroll =
      compositor_deps->IsElasticOverscrollEnabled();
  settings.resource_settings.use_gpu_memory_buffer_resources =
      compositor_deps->IsGpuMemoryBufferCompositorResourcesEnabled();
  settings.enable_oop_rasterization =
      cmd.HasSwitch(switches::kEnableOOPRasterization);

  // Build LayerTreeSettings from command line args.
  LayerTreeSettingsFactory::SetBrowserControlsSettings(settings, cmd);

  settings.use_layer_lists = cmd.HasSwitch(cc::switches::kEnableLayerLists);

  // The means the renderer compositor has 2 possible modes:
  // - Threaded compositing with a scheduler.
  // - Single threaded compositing without a scheduler (for layout tests only).
  // Using the scheduler in layout tests introduces additional composite steps
  // that create flakiness.
  settings.single_thread_proxy_scheduler = false;

  // These flags should be mirrored by UI versions in ui/compositor/.
  if (cmd.HasSwitch(cc::switches::kShowCompositedLayerBorders))
    settings.initial_debug_state.show_debug_borders.set();
  settings.initial_debug_state.show_layer_animation_bounds_rects =
      cmd.HasSwitch(cc::switches::kShowLayerAnimationBounds);
  settings.initial_debug_state.show_paint_rects =
      cmd.HasSwitch(switches::kShowPaintRects);
  settings.initial_debug_state.show_property_changed_rects =
      cmd.HasSwitch(cc::switches::kShowPropertyChangedRects);
  settings.initial_debug_state.show_surface_damage_rects =
      cmd.HasSwitch(cc::switches::kShowSurfaceDamageRects);
  settings.initial_debug_state.show_screen_space_rects =
      cmd.HasSwitch(cc::switches::kShowScreenSpaceRects);

  settings.initial_debug_state.SetRecordRenderingStats(
      cmd.HasSwitch(cc::switches::kEnableGpuBenchmarking));
  settings.enable_surface_synchronization =
      features::IsSurfaceSynchronizationEnabled();
  settings.build_hit_test_data = features::IsVizHitTestingSurfaceLayerEnabled();

  if (cmd.HasSwitch(cc::switches::kSlowDownRasterScaleFactor)) {
    const int kMinSlowDownScaleFactor = 0;
    const int kMaxSlowDownScaleFactor = INT_MAX;
    GetSwitchValueAsInt(
        cmd, cc::switches::kSlowDownRasterScaleFactor, kMinSlowDownScaleFactor,
        kMaxSlowDownScaleFactor,
        &settings.initial_debug_state.slow_down_raster_scale_factor);
  }

  // This is default overlay scrollbar settings for Android and DevTools mobile
  // emulator. Aura Overlay Scrollbar will override below.
  settings.scrollbar_animator = cc::LayerTreeSettings::ANDROID_OVERLAY;
  settings.solid_color_scrollbar_color = SkColorSetARGB(128, 128, 128, 128);
  settings.scrollbar_fade_delay = base::TimeDelta::FromMilliseconds(300);
  settings.scrollbar_fade_duration = base::TimeDelta::FromMilliseconds(300);


#if defined(OS_ANDROID)
  bool using_synchronous_compositor =
      GetContentClient()->UsingSynchronousCompositing();
  bool using_low_memory_policy = base::SysInfo::IsLowEndDevice();

  settings.use_stream_video_draw_quad = true;
  settings.using_synchronous_renderer_compositor = using_synchronous_compositor;
  if (using_synchronous_compositor) {
    // Android WebView uses system scrollbars, so make ours invisible.
    // http://crbug.com/677348: This can't be done using hide_scrollbars
    // setting because supporting -webkit custom scrollbars is still desired
    // on sublayers.
    settings.scrollbar_animator = cc::LayerTreeSettings::NO_ANIMATOR;
    settings.solid_color_scrollbar_color = SK_ColorTRANSPARENT;

    settings.enable_early_damage_check =
        cmd.HasSwitch(cc::switches::kCheckDamageEarly);
  }
  // Android WebView handles root layer flings itself.
  settings.ignore_root_layer_flings = using_synchronous_compositor;
  // Memory policy on Android WebView does not depend on whether device is
  // low end, so always use default policy.
  if (using_low_memory_policy && !using_synchronous_compositor) {
    // On low-end we want to be very carefull about killing other
    // apps. So initially we use 50% more memory to avoid flickering
    // or raster-on-demand.
    settings.max_memory_for_prepaint_percentage = 67;
  } else {
    // On other devices we have increased memory excessively to avoid
    // raster-on-demand already, so now we reserve 50% _only_ to avoid
    // raster-on-demand, and use 50% of the memory otherwise.
    settings.max_memory_for_prepaint_percentage = 50;
  }

  // TODO(danakj): Only do this on low end devices.
  settings.create_low_res_tiling = true;

#else  // defined(OS_ANDROID)
  bool using_synchronous_compositor = false;  // Only for Android WebView.
  // On desktop, we never use the low memory policy unless we are simulating
  // low-end mode via a switch.
  bool using_low_memory_policy =
      cmd.HasSwitch(switches::kEnableLowEndDeviceMode);

  if (ui::IsOverlayScrollbarEnabled()) {
    settings.scrollbar_animator = cc::LayerTreeSettings::AURA_OVERLAY;
    settings.scrollbar_fade_delay = ui::kOverlayScrollbarFadeDelay;
    settings.scrollbar_fade_duration = ui::kOverlayScrollbarFadeDuration;
    settings.scrollbar_thinning_duration =
        ui::kOverlayScrollbarThinningDuration;
    settings.scrollbar_flash_after_any_scroll_update =
        ui::OverlayScrollbarFlashAfterAnyScrollUpdate();
    settings.scrollbar_flash_when_mouse_enter =
        ui::OverlayScrollbarFlashWhenMouseEnter();
  }

  // On desktop, if there's over 4GB of memory on the machine, increase the
  // working set size to 256MB for both gpu and software.
  const int kImageDecodeMemoryThresholdMB = 4 * 1024;
  if (base::SysInfo::AmountOfPhysicalMemoryMB() >=
      kImageDecodeMemoryThresholdMB) {
    settings.decoded_image_working_set_budget_bytes = 256 * 1024 * 1024;
  } else {
    // This is the default, but recorded here as well.
    settings.decoded_image_working_set_budget_bytes = 128 * 1024 * 1024;
  }
#endif  // defined(OS_ANDROID)

  if (using_low_memory_policy) {
    // RGBA_4444 textures are only enabled:
    //  - If the user hasn't explicitly disabled them
    //  - If system ram is <= 512MB (1GB devices are sometimes low-end).
    //  - If we are not running in a WebView, where 4444 isn't supported.
    if (!cmd.HasSwitch(switches::kDisableRGBA4444Textures) &&
        base::SysInfo::AmountOfPhysicalMemoryMB() <= 512 &&
        !using_synchronous_compositor) {
      settings.use_rgba_4444 = viz::RGBA_4444;

      // If we are going to unpremultiply and dither these tiles, we need to
      // allocate an additional RGBA_8888 intermediate for each tile
      // rasterization when rastering to RGBA_4444 to allow for dithering.
      // Setting a reasonable sized max tile size allows this intermediate to
      // be consistently reused.
      if (base::FeatureList::IsEnabled(
              kUnpremultiplyAndDitherLowBitDepthTiles)) {
        settings.max_gpu_raster_tile_size = gfx::Size(512, 256);
        settings.unpremultiply_and_dither_low_bit_depth_tiles = true;
      }
    }
  }

  if (cmd.HasSwitch(switches::kEnableLowResTiling))
    settings.create_low_res_tiling = true;
  if (cmd.HasSwitch(switches::kDisableLowResTiling))
    settings.create_low_res_tiling = false;

  if (cmd.HasSwitch(switches::kEnableRGBA4444Textures) &&
      !cmd.HasSwitch(switches::kDisableRGBA4444Textures)) {
    settings.use_rgba_4444 = true;
  }

  settings.max_staging_buffer_usage_in_bytes = 32 * 1024 * 1024;  // 32MB
  // Use 1/4th of staging buffers on low-end devices.
  if (base::SysInfo::IsLowEndDevice())
    settings.max_staging_buffer_usage_in_bytes /= 4;

  cc::ManagedMemoryPolicy defaults = settings.memory_policy;
  settings.memory_policy = GetGpuMemoryPolicy(defaults, screen_info);

  settings.disallow_non_exact_resource_reuse =
      cmd.HasSwitch(switches::kDisallowNonExactResourceReuse);
#if defined(OS_ANDROID)
  // TODO(crbug.com/746931): This feature appears to be causing visual
  // corruption on certain android devices. Will investigate and re-enable.
  settings.disallow_non_exact_resource_reuse = true;
#endif

  if (cmd.HasSwitch(switches::kRunAllCompositorStagesBeforeDraw)) {
    settings.wait_for_all_pipeline_stages_before_draw = true;
    settings.enable_latency_recovery = false;
  }

  settings.enable_image_animation_resync =
      !cmd.HasSwitch(switches::kDisableImageAnimationResync);

  settings.always_request_presentation_time =
      cmd.HasSwitch(cc::switches::kAlwaysRequestPresentationTime);

  settings.use_painted_device_scale_factor = IsUseZoomForDSFEnabled();
  return settings;
}

// static
cc::ManagedMemoryPolicy RenderWidgetCompositor::GetGpuMemoryPolicy(
    const cc::ManagedMemoryPolicy& default_policy,
    const ScreenInfo& screen_info) {
  cc::ManagedMemoryPolicy actual = default_policy;
  actual.bytes_limit_when_visible = 0;

  // If the value was overridden on the command line, use the specified value.
  static bool client_hard_limit_bytes_overridden =
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kForceGpuMemAvailableMb);
  if (client_hard_limit_bytes_overridden) {
    if (base::StringToSizeT(
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                switches::kForceGpuMemAvailableMb),
            &actual.bytes_limit_when_visible))
      actual.bytes_limit_when_visible *= 1024 * 1024;
    return actual;
  }

#if defined(OS_ANDROID)
  // We can't query available GPU memory from the system on Android.
  // Physical memory is also mis-reported sometimes (eg. Nexus 10 reports
  // 1262MB when it actually has 2GB, while Razr M has 1GB but only reports
  // 128MB java heap size). First we estimate physical memory using both.
  size_t dalvik_mb = base::SysInfo::DalvikHeapSizeMB();
  size_t physical_mb = base::SysInfo::AmountOfPhysicalMemoryMB();
  size_t physical_memory_mb = 0;
  if (base::SysInfo::IsLowEndDevice()) {
    // TODO(crbug.com/742534): The code below appears to no longer work.
    // |dalvik_mb| no longer follows the expected heuristic pattern, causing us
    // to over-estimate memory on low-end devices. This entire section probably
    // needs to be re-written, but for now we can address the low-end Android
    // issues by ignoring |dalvik_mb|.
    physical_memory_mb = physical_mb;
  } else if (dalvik_mb >= 256) {
    physical_memory_mb = dalvik_mb * 4;
  } else {
    physical_memory_mb = std::max(dalvik_mb * 4, (physical_mb * 4) / 3);
  }

  // Now we take a default of 1/8th of memory on high-memory devices,
  // and gradually scale that back for low-memory devices (to be nicer
  // to other apps so they don't get killed). Examples:
  // Nexus 4/10(2GB)    256MB (normally 128MB)
  // Droid Razr M(1GB)  114MB (normally 57MB)
  // Galaxy Nexus(1GB)  100MB (normally 50MB)
  // Xoom(1GB)          100MB (normally 50MB)
  // Nexus S(low-end)   8MB (normally 8MB)
  // Note that the compositor now uses only some of this memory for
  // pre-painting and uses the rest only for 'emergencies'.
  if (actual.bytes_limit_when_visible == 0) {
    // NOTE: Non-low-end devices use only 50% of these limits,
    // except during 'emergencies' where 100% can be used.
    if (physical_memory_mb >= 1536)
      actual.bytes_limit_when_visible = physical_memory_mb / 8;  // >192MB
    else if (physical_memory_mb >= 1152)
      actual.bytes_limit_when_visible = physical_memory_mb / 8;  // >144MB
    else if (physical_memory_mb >= 768)
      actual.bytes_limit_when_visible = physical_memory_mb / 10;  // >76MB
    else if (physical_memory_mb >= 513)
      actual.bytes_limit_when_visible = physical_memory_mb / 12;  // <64MB
    else
      // Devices with this little RAM have very little headroom so we hardcode
      // the limit rather than relying on the heuristics above.  (They also use
      // 4444 textures so we can use a lower limit.)
      actual.bytes_limit_when_visible = 8;

    actual.bytes_limit_when_visible =
        actual.bytes_limit_when_visible * 1024 * 1024;
    // Clamp the observed value to a specific range on Android.
    actual.bytes_limit_when_visible = std::max(
        actual.bytes_limit_when_visible, static_cast<size_t>(8 * 1024 * 1024));
    actual.bytes_limit_when_visible =
        std::min(actual.bytes_limit_when_visible,
                 static_cast<size_t>(256 * 1024 * 1024));
  }
  actual.priority_cutoff_when_visible =
      gpu::MemoryAllocation::CUTOFF_ALLOW_EVERYTHING;
#else
  // Ignore what the system said and give all clients the same maximum
  // allocation on desktop platforms.
  actual.bytes_limit_when_visible = 512 * 1024 * 1024;
  actual.priority_cutoff_when_visible =
      gpu::MemoryAllocation::CUTOFF_ALLOW_NICE_TO_HAVE;

  // For large monitors (4k), double the tile memory to avoid frequent out of
  // memory problems. 4k could mean a screen width of anywhere from 3840 to 4096
  // (see https://en.wikipedia.org/wiki/4K_resolution). We use 3500 as a proxy
  // for "large enough".
  static const int kLargeDisplayThreshold = 3500;
  int display_width =
      std::round(screen_info.rect.width() * screen_info.device_scale_factor);
  if (display_width >= kLargeDisplayThreshold)
    actual.bytes_limit_when_visible *= 2;
#endif
  return actual;
}

void RenderWidgetCompositor::SetNeverVisible() {
  DCHECK(!layer_tree_host_->IsVisible());
  never_visible_ = true;
}

const base::WeakPtr<cc::InputHandler>&
RenderWidgetCompositor::GetInputHandler() {
  return layer_tree_host_->GetInputHandler();
}

void RenderWidgetCompositor::SetNeedsDisplayOnAllLayers() {
  layer_tree_host_->SetNeedsDisplayOnAllLayers();
}

void RenderWidgetCompositor::SetRasterizeOnlyVisibleContent() {
  cc::LayerTreeDebugState current = layer_tree_host_->GetDebugState();
  current.rasterize_only_visible_content = true;
  layer_tree_host_->SetDebugState(current);
}

void RenderWidgetCompositor::SetNeedsRedrawRect(gfx::Rect damage_rect) {
  layer_tree_host_->SetNeedsRedrawRect(damage_rect);
}

bool RenderWidgetCompositor::IsSurfaceSynchronizationEnabled() const {
  return layer_tree_host_->GetSettings().enable_surface_synchronization;
}

void RenderWidgetCompositor::SetNeedsForcedRedraw() {
  layer_tree_host_->SetNeedsCommitWithForcedRedraw();
}

std::unique_ptr<cc::SwapPromiseMonitor>
RenderWidgetCompositor::CreateLatencyInfoSwapPromiseMonitor(
    ui::LatencyInfo* latency) {
  return std::make_unique<cc::LatencyInfoSwapPromiseMonitor>(
      latency, layer_tree_host_->GetSwapPromiseManager(), nullptr);
}

void RenderWidgetCompositor::QueueSwapPromise(
    std::unique_ptr<cc::SwapPromise> swap_promise) {
  layer_tree_host_->QueueSwapPromise(std::move(swap_promise));
}

int RenderWidgetCompositor::GetSourceFrameNumber() const {
  return layer_tree_host_->SourceFrameNumber();
}

void RenderWidgetCompositor::NotifyInputThrottledUntilCommit() {
  layer_tree_host_->NotifyInputThrottledUntilCommit();
}

const cc::Layer* RenderWidgetCompositor::GetRootLayer() const {
  return layer_tree_host_->root_layer();
}

int RenderWidgetCompositor::ScheduleMicroBenchmark(
    const std::string& name,
    std::unique_ptr<base::Value> value,
    const base::Callback<void(std::unique_ptr<base::Value>)>& callback) {
  return layer_tree_host_->ScheduleMicroBenchmark(name, std::move(value),
                                                  callback);
}

bool RenderWidgetCompositor::SendMessageToMicroBenchmark(
    int id,
    std::unique_ptr<base::Value> value) {
  return layer_tree_host_->SendMessageToMicroBenchmark(id, std::move(value));
}

void RenderWidgetCompositor::SetViewportSizeAndScale(
    const gfx::Size& device_viewport_size,
    float device_scale_factor,
    const viz::LocalSurfaceId& local_surface_id) {
  layer_tree_host_->SetViewportSizeAndScale(
      device_viewport_size, device_scale_factor, local_surface_id);
}

void RenderWidgetCompositor::RequestNewLocalSurfaceId() {
  layer_tree_host_->RequestNewLocalSurfaceId();
}

bool RenderWidgetCompositor::HasNewLocalSurfaceIdRequest() const {
  return layer_tree_host_->new_local_surface_id_request_for_testing();
}

void RenderWidgetCompositor::SetViewportVisibleRect(
    const gfx::Rect& visible_rect) {
  layer_tree_host_->SetViewportVisibleRect(visible_rect);
}

viz::FrameSinkId RenderWidgetCompositor::GetFrameSinkId() {
  return frame_sink_id_;
}

void RenderWidgetCompositor::SetRootLayer(scoped_refptr<cc::Layer> layer) {
  layer_tree_host_->SetRootLayer(std::move(layer));
}

void RenderWidgetCompositor::ClearRootLayer() {
  layer_tree_host_->SetRootLayer(nullptr);
}

cc::AnimationHost* RenderWidgetCompositor::CompositorAnimationHost() {
  return animation_host_.get();
}

blink::WebSize RenderWidgetCompositor::GetViewportSize() const {
  return layer_tree_host_->device_viewport_size();
}

blink::WebFloatPoint RenderWidgetCompositor::adjustEventPointForPinchZoom(
    const blink::WebFloatPoint& point) const {
  return point;
}

void RenderWidgetCompositor::SetBackgroundColor(SkColor color) {
  layer_tree_host_->set_background_color(color);
}

void RenderWidgetCompositor::SetVisible(bool visible) {
  if (never_visible_)
    return;

  layer_tree_host_->SetVisible(visible);

  if (visible && layer_tree_frame_sink_request_failed_while_invisible_)
    DidFailToInitializeLayerTreeFrameSink();
}

void RenderWidgetCompositor::SetPageScaleFactorAndLimits(
    float page_scale_factor,
    float minimum,
    float maximum) {
  layer_tree_host_->SetPageScaleFactorAndLimits(page_scale_factor, minimum,
                                                maximum);
}

void RenderWidgetCompositor::StartPageScaleAnimation(
    const blink::WebPoint& destination,
    bool use_anchor,
    float new_page_scale,
    double duration_sec) {
  base::TimeDelta duration = base::TimeDelta::FromMicroseconds(
      duration_sec * base::Time::kMicrosecondsPerSecond);
  layer_tree_host_->StartPageScaleAnimation(
      gfx::Vector2d(destination.x, destination.y), use_anchor, new_page_scale,
      duration);
}

bool RenderWidgetCompositor::HasPendingPageScaleAnimation() const {
  return layer_tree_host_->HasPendingPageScaleAnimation();
}

void RenderWidgetCompositor::HeuristicsForGpuRasterizationUpdated(
    bool matches_heuristics) {
  layer_tree_host_->SetHasGpuRasterizationTrigger(matches_heuristics);
}

void RenderWidgetCompositor::SetNeedsBeginFrame() {
  layer_tree_host_->SetNeedsAnimate();
}

void RenderWidgetCompositor::DidStopFlinging() {
  layer_tree_host_->DidStopFlinging();
}

void RenderWidgetCompositor::RegisterViewportLayers(
    const blink::WebLayerTreeView::ViewportLayers& layers) {
  cc::LayerTreeHost::ViewportLayers viewport_layers;
  // TODO(bokan): This check can probably be removed now, but it looks
  // like overscroll elasticity may still be nullptr until VisualViewport
  // registers its layers.
  if (layers.overscroll_elasticity) {
    viewport_layers.overscroll_elasticity = layers.overscroll_elasticity;
  }
  viewport_layers.page_scale = layers.page_scale;
  if (layers.inner_viewport_container) {
    viewport_layers.inner_viewport_container = layers.inner_viewport_container;
  }
  if (layers.outer_viewport_container) {
    viewport_layers.outer_viewport_container = layers.outer_viewport_container;
  }
  viewport_layers.inner_viewport_scroll = layers.inner_viewport_scroll;
  // TODO(bokan): This check can probably be removed now, but it looks
  // like overscroll elasticity may still be nullptr until VisualViewport
  // registers its layers.
  if (layers.outer_viewport_scroll) {
    viewport_layers.outer_viewport_scroll = layers.outer_viewport_scroll;
  }
  layer_tree_host_->RegisterViewportLayers(viewport_layers);
}

void RenderWidgetCompositor::ClearViewportLayers() {
  layer_tree_host_->RegisterViewportLayers(cc::LayerTreeHost::ViewportLayers());
}

void RenderWidgetCompositor::RegisterSelection(
    const blink::WebSelection& selection) {
  layer_tree_host_->RegisterSelection(ConvertFromWebSelection(selection));
}

void RenderWidgetCompositor::ClearSelection() {
  cc::LayerSelection empty_selection;
  layer_tree_host_->RegisterSelection(empty_selection);
}

void RenderWidgetCompositor::SetMutatorClient(
    std::unique_ptr<cc::LayerTreeMutator> client) {
  TRACE_EVENT0("cc", "RenderWidgetCompositor::setMutatorClient");
  layer_tree_host_->SetLayerTreeMutator(std::move(client));
}

void RenderWidgetCompositor::ForceRecalculateRasterScales() {
  layer_tree_host_->SetNeedsRecalculateRasterScales();
}

static_assert(static_cast<cc::EventListenerClass>(
                  blink::WebEventListenerClass::kTouchStartOrMove) ==
                  cc::EventListenerClass::kTouchStartOrMove,
              "EventListenerClass and WebEventListenerClass enums must match");
static_assert(static_cast<cc::EventListenerClass>(
                  blink::WebEventListenerClass::kMouseWheel) ==
                  cc::EventListenerClass::kMouseWheel,
              "EventListenerClass and WebEventListenerClass enums must match");

static_assert(static_cast<cc::EventListenerProperties>(
                  blink::WebEventListenerProperties::kNothing) ==
                  cc::EventListenerProperties::kNone,
              "EventListener and WebEventListener enums must match");
static_assert(static_cast<cc::EventListenerProperties>(
                  blink::WebEventListenerProperties::kPassive) ==
                  cc::EventListenerProperties::kPassive,
              "EventListener and WebEventListener enums must match");
static_assert(static_cast<cc::EventListenerProperties>(
                  blink::WebEventListenerProperties::kBlocking) ==
                  cc::EventListenerProperties::kBlocking,
              "EventListener and WebEventListener enums must match");
static_assert(static_cast<cc::EventListenerProperties>(
                  blink::WebEventListenerProperties::kBlockingAndPassive) ==
                  cc::EventListenerProperties::kBlockingAndPassive,
              "EventListener and WebEventListener enums must match");

void RenderWidgetCompositor::SetEventListenerProperties(
    blink::WebEventListenerClass eventClass,
    blink::WebEventListenerProperties properties) {
  layer_tree_host_->SetEventListenerProperties(
      static_cast<cc::EventListenerClass>(eventClass),
      static_cast<cc::EventListenerProperties>(properties));
}

blink::WebEventListenerProperties
RenderWidgetCompositor::EventListenerProperties(
    blink::WebEventListenerClass event_class) const {
  return static_cast<blink::WebEventListenerProperties>(
      layer_tree_host_->event_listener_properties(
          static_cast<cc::EventListenerClass>(event_class)));
}

void RenderWidgetCompositor::SetHaveScrollEventHandlers(bool has_handlers) {
  layer_tree_host_->SetHaveScrollEventHandlers(has_handlers);
}

bool RenderWidgetCompositor::HaveScrollEventHandlers() const {
  return layer_tree_host_->have_scroll_event_handlers();
}

bool RenderWidgetCompositor::CompositeIsSynchronous() const {
  if (!threaded_) {
    DCHECK(!layer_tree_host_->GetSettings().single_thread_proxy_scheduler);
    return true;
  }
  return false;
}

void RenderWidgetCompositor::LayoutAndPaintAsync(base::OnceClosure callback) {
  DCHECK(layout_and_paint_async_callback_.is_null());
  layout_and_paint_async_callback_ = std::move(callback);

  if (CompositeIsSynchronous()) {
    // The LayoutAndPaintAsyncCallback is invoked in WillCommit, which is
    // dispatched after layout and paint for all compositing modes.
    const bool raster = false;
    layer_tree_host_->GetTaskRunnerProvider()->MainThreadTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(&RenderWidgetCompositor::SynchronouslyComposite,
                       weak_factory_.GetWeakPtr(), raster, nullptr));
  } else {
    layer_tree_host_->SetNeedsCommit();
  }
}

void RenderWidgetCompositor::SetLayerTreeFrameSink(
    std::unique_ptr<cc::LayerTreeFrameSink> layer_tree_frame_sink) {
  if (!layer_tree_frame_sink) {
    DidFailToInitializeLayerTreeFrameSink();
    return;
  }
  layer_tree_host_->SetLayerTreeFrameSink(std::move(layer_tree_frame_sink));
}

void RenderWidgetCompositor::InvokeLayoutAndPaintCallback() {
  if (!layout_and_paint_async_callback_.is_null())
    std::move(layout_and_paint_async_callback_).Run();
}

void RenderWidgetCompositor::CompositeAndReadbackAsync(
    base::OnceCallback<void(const SkBitmap&)> callback) {
  DCHECK(layout_and_paint_async_callback_.is_null());
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner =
      layer_tree_host_->GetTaskRunnerProvider()->MainThreadTaskRunner();
  std::unique_ptr<viz::CopyOutputRequest> request =
      std::make_unique<viz::CopyOutputRequest>(
          viz::CopyOutputRequest::ResultFormat::RGBA_BITMAP,
          base::BindOnce(
              [](base::OnceCallback<void(const SkBitmap&)> callback,
                 scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                 std::unique_ptr<viz::CopyOutputResult> result) {
                task_runner->PostTask(
                    FROM_HERE,
                    base::BindOnce(std::move(callback), result->AsSkBitmap()));
              },
              std::move(callback), std::move(main_thread_task_runner)));
  auto swap_promise =
      delegate_->RequestCopyOfOutputForLayoutTest(std::move(request));

  // Force a commit to happen. The temporary copy output request will
  // be installed after layout which will happen as a part of the commit, for
  // widgets that delay the creation of their output surface.
  if (CompositeIsSynchronous()) {
    // Since the composite is required for a pixel dump, we need to raster.
    // Note that we defer queuing the SwapPromise until the requested Composite
    // with rasterization is done.
    const bool raster = true;
    layer_tree_host_->GetTaskRunnerProvider()->MainThreadTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(&RenderWidgetCompositor::SynchronouslyComposite,
                       weak_factory_.GetWeakPtr(), raster,
                       std::move(swap_promise)));
  } else {
    // Force a redraw to ensure that the copy swap promise isn't cancelled due
    // to no damage.
    SetNeedsForcedRedraw();
    layer_tree_host_->QueueSwapPromise(std::move(swap_promise));
    layer_tree_host_->SetNeedsCommit();
  }
}

void RenderWidgetCompositor::SynchronouslyCompositeNoRasterForTesting() {
  SynchronouslyComposite(false /* raster */, nullptr /* swap_promise */);
}

void RenderWidgetCompositor::CompositeWithRasterForTesting() {
  SynchronouslyComposite(true /* raster */, nullptr /* swap_promise */);
}

void RenderWidgetCompositor::SynchronouslyComposite(
    bool raster,
    std::unique_ptr<cc::SwapPromise> swap_promise) {
  DCHECK(CompositeIsSynchronous());
  if (!layer_tree_host_->IsVisible())
    return;

  if (in_synchronous_compositor_update_) {
    // LayoutTests can use a nested message loop to pump frames while inside a
    // frame, but the compositor does not support this. In this case, we only
    // run blink's lifecycle updates.
    delegate_->BeginMainFrame(base::TimeTicks::Now());
    delegate_->UpdateVisualState(
        cc::LayerTreeHostClient::VisualStateUpdate::kAll);
    return;
  }

  if (swap_promise) {
    // Force a redraw to ensure that the copy swap promise isn't cancelled due
    // to no damage.
    SetNeedsForcedRedraw();
    layer_tree_host_->QueueSwapPromise(std::move(swap_promise));
  }

  DCHECK(!in_synchronous_compositor_update_);
  base::AutoReset<bool> inside_composite(&in_synchronous_compositor_update_,
                                         true);
  layer_tree_host_->Composite(base::TimeTicks::Now(), raster);
}

void RenderWidgetCompositor::SetDeferCommits(bool defer_commits) {
  layer_tree_host_->SetDeferCommits(defer_commits);
}

int RenderWidgetCompositor::LayerTreeId() const {
  return layer_tree_host_->GetId();
}

void RenderWidgetCompositor::SetShowFPSCounter(bool show) {
  cc::LayerTreeDebugState debug_state = layer_tree_host_->GetDebugState();
  debug_state.show_fps_counter = show;
  layer_tree_host_->SetDebugState(debug_state);
}

void RenderWidgetCompositor::SetShowPaintRects(bool show) {
  cc::LayerTreeDebugState debug_state = layer_tree_host_->GetDebugState();
  debug_state.show_paint_rects = show;
  layer_tree_host_->SetDebugState(debug_state);
}

void RenderWidgetCompositor::SetShowDebugBorders(bool show) {
  cc::LayerTreeDebugState debug_state = layer_tree_host_->GetDebugState();
  if (show)
    debug_state.show_debug_borders.set();
  else
    debug_state.show_debug_borders.reset();
  layer_tree_host_->SetDebugState(debug_state);
}

void RenderWidgetCompositor::SetShowScrollBottleneckRects(bool show) {
  cc::LayerTreeDebugState debug_state = layer_tree_host_->GetDebugState();
  debug_state.show_touch_event_handler_rects = show;
  debug_state.show_wheel_event_handler_rects = show;
  debug_state.show_non_fast_scrollable_rects = show;
  layer_tree_host_->SetDebugState(debug_state);
}

void RenderWidgetCompositor::UpdateBrowserControlsState(
    blink::WebBrowserControlsState constraints,
    blink::WebBrowserControlsState current,
    bool animate) {
  layer_tree_host_->UpdateBrowserControlsState(
      ConvertBrowserControlsState(constraints),
      ConvertBrowserControlsState(current), animate);
}

void RenderWidgetCompositor::SetBrowserControlsHeight(float top_height,
                                                      float bottom_height,
                                                      bool shrink) {
  layer_tree_host_->SetBrowserControlsHeight(top_height, bottom_height, shrink);
}

void RenderWidgetCompositor::SetBrowserControlsShownRatio(float ratio) {
  layer_tree_host_->SetBrowserControlsShownRatio(ratio);
}

void RenderWidgetCompositor::RequestDecode(
    const cc::PaintImage& image,
    base::OnceCallback<void(bool)> callback) {
  layer_tree_host_->QueueImageDecode(image, std::move(callback));

  // If we're compositing synchronously, the SetNeedsCommit call which will be
  // issued by |layer_tree_host_| is not going to cause a commit, due to the
  // fact that this would make layout tests slow and cause flakiness. However,
  // in this case we actually need a commit to transfer the decode requests to
  // the impl side. So, force a commit to happen.
  if (CompositeIsSynchronous()) {
    const bool raster = true;
    layer_tree_host_->GetTaskRunnerProvider()->MainThreadTaskRunner()->PostTask(
        FROM_HERE,
        base::BindOnce(&RenderWidgetCompositor::SynchronouslyComposite,
                       weak_factory_.GetWeakPtr(), raster, nullptr));
  }
}

void RenderWidgetCompositor::SetOverscrollBehavior(
    const cc::OverscrollBehavior& behavior) {
  layer_tree_host_->SetOverscrollBehavior(behavior);
}

void RenderWidgetCompositor::WillBeginMainFrame() {
  delegate_->WillBeginCompositorFrame();
}

void RenderWidgetCompositor::DidBeginMainFrame() {}

void RenderWidgetCompositor::BeginMainFrame(const viz::BeginFrameArgs& args) {
  compositor_deps_->GetWebMainThreadScheduler()->WillBeginFrame(args);
  delegate_->BeginMainFrame(args.frame_time);
}

void RenderWidgetCompositor::BeginMainFrameNotExpectedSoon() {
  compositor_deps_->GetWebMainThreadScheduler()->BeginFrameNotExpectedSoon();
}

void RenderWidgetCompositor::BeginMainFrameNotExpectedUntil(
    base::TimeTicks time) {
  compositor_deps_->GetWebMainThreadScheduler()->BeginMainFrameNotExpectedUntil(
      time);
}

void RenderWidgetCompositor::UpdateLayerTreeHost(
    VisualStateUpdate requested_update) {
  delegate_->UpdateVisualState(requested_update);
}

void RenderWidgetCompositor::ApplyViewportDeltas(
    const gfx::Vector2dF& inner_delta,
    const gfx::Vector2dF& outer_delta,
    const gfx::Vector2dF& elastic_overscroll_delta,
    float page_scale,
    float top_controls_delta) {
  delegate_->ApplyViewportDeltas(inner_delta, outer_delta,
                                 elastic_overscroll_delta, page_scale,
                                 top_controls_delta);
}

void RenderWidgetCompositor::RecordWheelAndTouchScrollingCount(
    bool has_scrolled_by_wheel,
    bool has_scrolled_by_touch) {
  delegate_->RecordWheelAndTouchScrollingCount(has_scrolled_by_wheel,
                                               has_scrolled_by_touch);
}

void RenderWidgetCompositor::RequestNewLayerTreeFrameSink() {
  // If the host is closing, then no more compositing is possible.  This
  // prevents shutdown races between handling the close message and
  // the CreateLayerTreeFrameSink task.
  if (delegate_->IsClosing())
    return;
  delegate_->RequestNewLayerTreeFrameSink(
      base::Bind(&RenderWidgetCompositor::SetLayerTreeFrameSink,
                 weak_factory_.GetWeakPtr()));
}

void RenderWidgetCompositor::DidInitializeLayerTreeFrameSink() {
}

void RenderWidgetCompositor::DidFailToInitializeLayerTreeFrameSink() {
  if (!layer_tree_host_->IsVisible()) {
    layer_tree_frame_sink_request_failed_while_invisible_ = true;
    return;
  }
  layer_tree_frame_sink_request_failed_while_invisible_ = false;
  layer_tree_host_->GetTaskRunnerProvider()->MainThreadTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&RenderWidgetCompositor::RequestNewLayerTreeFrameSink,
                     weak_factory_.GetWeakPtr()));
}

void RenderWidgetCompositor::WillCommit() {
  InvokeLayoutAndPaintCallback();
}

void RenderWidgetCompositor::DidCommit() {
  delegate_->DidCommitCompositorFrame();
  compositor_deps_->GetWebMainThreadScheduler()->DidCommitFrameToCompositor();
}

void RenderWidgetCompositor::DidCommitAndDrawFrame() {
  delegate_->DidCommitAndDrawCompositorFrame();
}

void RenderWidgetCompositor::DidReceiveCompositorFrameAck() {
  delegate_->DidReceiveCompositorFrameAck();
}

void RenderWidgetCompositor::DidCompletePageScaleAnimation() {
  delegate_->DidCompletePageScaleAnimation();
}

bool RenderWidgetCompositor::IsForSubframe() {
  return is_for_oopif_;
}

void RenderWidgetCompositor::RequestScheduleAnimation() {
  delegate_->RequestScheduleAnimation();
}

void RenderWidgetCompositor::DidSubmitCompositorFrame() {}

void RenderWidgetCompositor::DidLoseLayerTreeFrameSink() {}

void RenderWidgetCompositor::SetFrameSinkId(
    const viz::FrameSinkId& frame_sink_id) {
  frame_sink_id_ = frame_sink_id;
}

void RenderWidgetCompositor::SetRasterColorSpace(
    const gfx::ColorSpace& color_space) {
  layer_tree_host_->SetRasterColorSpace(color_space);
}

void RenderWidgetCompositor::SetIsForOopif(bool is_for_oopif) {
  is_for_oopif_ = is_for_oopif;
}

void RenderWidgetCompositor::SetContentSourceId(uint32_t id) {
  layer_tree_host_->SetContentSourceId(id);
}

void RenderWidgetCompositor::NotifySwapTime(ReportTimeCallback callback) {
  QueueSwapPromise(std::make_unique<ReportTimeSwapPromise>(
      std::move(callback),
      layer_tree_host_->GetTaskRunnerProvider()->MainThreadTaskRunner()));
}

void RenderWidgetCompositor::RequestBeginMainFrameNotExpected(bool new_state) {
  layer_tree_host_->RequestBeginMainFrameNotExpected(new_state);
}

void RenderWidgetCompositor::CreateRenderFrameObserver(
    mojom::RenderFrameMetadataObserverRequest request,
    mojom::RenderFrameMetadataObserverClientPtrInfo client_info) {
  auto render_frame_metadata_observer =
      std::make_unique<RenderFrameMetadataObserverImpl>(std::move(request),
                                                        std::move(client_info));
  layer_tree_host_->SetRenderFrameObserver(
      std::move(render_frame_metadata_observer));
}

void RenderWidgetCompositor::SetURLForUkm(const GURL& url) {
  layer_tree_host_->SetURLForUkm(url);
}

}  // namespace content
