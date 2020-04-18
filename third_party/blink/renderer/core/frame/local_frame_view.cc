/*
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *           (C) 2006 Graham Dennis (graham.dennis@gmail.com)
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/frame/local_frame_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom-blink.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_scroll_into_view_params.h"
#include "third_party/blink/renderer/core/animation/document_animations.h"
#include "third_party/blink/renderer/core/css/font_face_set_document.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/dom/ax_object_cache.h"
#include "third_party/blink/renderer/core/dom/element_visibility_observer.h"
#include "third_party/blink/renderer/core/editing/drag_caret.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/markers/document_marker_controller.h"
#include "third_party/blink/renderer/core/editing/rendered_position.h"
#include "third_party/blink/renderer/core/events/error_event.h"
#include "third_party/blink/renderer/core/exported/web_plugin_container_impl.h"
#include "third_party/blink/renderer/core/frame/browser_controls.h"
#include "third_party/blink/renderer/core/frame/event_handler_registry.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/location.h"
#include "third_party/blink/renderer/core/frame/page_scale_constraints_set.h"
#include "third_party/blink/renderer/core/frame/remote_frame.h"
#include "third_party/blink/renderer/core/frame/remote_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/fullscreen/fullscreen.h"
#include "third_party/blink/renderer/core/html/forms/text_control_element.h"
#include "third_party/blink/renderer/core/html/html_frame_element.h"
#include "third_party/blink/renderer/core/html/html_plugin_element.h"
#include "third_party/blink/renderer/core/html/parser/text_resource_decoder.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_controller.h"
#include "third_party/blink/renderer/core/intersection_observer/intersection_observer_init.h"
#include "third_party/blink/renderer/core/layout/adjust_for_absolute_zoom.h"
#include "third_party/blink/renderer/core/layout/layout_analyzer.h"
#include "third_party/blink/renderer/core/layout/layout_counter.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_content.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_object.h"
#include "third_party/blink/renderer/core/layout/layout_scrollbar.h"
#include "third_party/blink/renderer/core/layout/layout_scrollbar_part.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_root.h"
#include "third_party/blink/renderer/core/layout/text_autosizer.h"
#include "third_party/blink/renderer/core/layout/traced_layout_object.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/media_type_names.h"
#include "third_party/blink/renderer/core/page/autoscroll_controller.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/page/frame_tree.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/print_context.h"
#include "third_party/blink/renderer/core/page/scrolling/root_scroller_util.h"
#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator.h"
#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator_context.h"
#include "third_party/blink/renderer/core/page/scrolling/snap_coordinator.h"
#include "third_party/blink/renderer/core/page/scrolling/top_document_root_scroller_controller.h"
#include "third_party/blink/renderer/core/paint/block_paint_invalidator.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_layer_mapping.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_selection.h"
#include "third_party/blink/renderer/core/paint/compositing/compositing_inputs_updater.h"
#include "third_party/blink/renderer/core/paint/compositing/paint_layer_compositor.h"
#include "third_party/blink/renderer/core/paint/frame_painter.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_timing.h"
#include "third_party/blink/renderer/core/paint/pre_paint_tree_walk.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/resize_observer/resize_observer_controller.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/svg/svg_svg_element.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/geometry/double_rect.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/graphics/compositing/paint_artifact_compositor.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/cull_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/foreign_layer_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/graphics/paint/scoped_paint_chunk_properties.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/traced_value.h"
#include "third_party/blink/renderer/platform/json/json_values.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/scroll/scroll_alignment.h"
#include "third_party/blink/renderer/platform/scroll/scroll_animator_base.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_theme.h"
#include "third_party/blink/renderer/platform/transforms/transform_state.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

// Used to check for dirty layouts violating document lifecycle rules.
// If arg evaluates to true, the program will continue. If arg evaluates to
// false, program will crash if DCHECK_IS_ON() or return false from the current
// function.
#define CHECK_FOR_DIRTY_LAYOUT(arg) \
  do {                              \
    if (!(arg)) {                   \
      NOTREACHED();                 \
      return false;                 \
    }                               \
  } while (false)

namespace blink {
namespace {

// Page dimensions in pixels at 72 DPI.
constexpr int kA4PortraitPageWidth = 595;
constexpr int kA4PortraitPageHeight = 842;
constexpr int kLetterPortraitPageWidth = 612;
constexpr int kLetterPortraitPageHeight = 792;

// Changing these values requires changing the names generated in
// EnsureUkmTimeAggregator().
enum class UkmMetricNames {
  kCompositing,
  kIntersectionObservation,
  kPaint,
  kPrePaint,
  kStyleAndLayout,
  kCount
};

}  // namespace

// Defines an UMA and a UKM, recorded in microseconds equal to the duration of
// the current lexical scope after declaration of the macro. Example usage:
//
// void LocalFrameView::DoExpensiveThing() {
//   SCOPED_UMA_AND_UKM_TIMER(UmaName, kUkmEnumName);
//   // Do computation of expensive thing
//
// }
//
// |uma_name| should be the full name of an UMA defined
// in histograms.xml. |ukm_enum| should be an entry in UkmMetricNames
// (which in turn come from ukm.xml).
#define SCOPED_UMA_AND_UKM_TIMER(uma_name, ukm_enum)                    \
  DEFINE_STATIC_LOCAL_IMPL(CustomCountHistogram, scoped_uma_counter,    \
                           (uma_name, 0, 10000000, 50), false);         \
  auto scoped_ukm_uma_timer = EnsureUkmTimeAggregator().GetScopedTimer( \
      static_cast<size_t>(ukm_enum), &scoped_uma_counter);

using namespace HTMLNames;

// The maximum number of updatePlugins iterations that should be done before
// returning.
static const unsigned kMaxUpdatePluginsIterations = 2;
static const double kResourcePriorityUpdateDelayAfterScroll = 0.250;

static bool g_initial_track_all_paint_invalidations = false;

LocalFrameView::LocalFrameView(LocalFrame& frame, IntRect frame_rect)
    : frame_(frame),
      frame_rect_(frame_rect),
      is_attached_(false),
      display_mode_(kWebDisplayModeBrowser),
      can_have_scrollbars_(true),
      has_pending_layout_(false),
      in_synchronous_post_layout_(false),
      post_layout_tasks_timer_(frame.GetTaskRunner(TaskType::kInternalDefault),
                               this,
                               &LocalFrameView::PostLayoutTimerFired),
      update_plugins_timer_(frame.GetTaskRunner(TaskType::kInternalDefault),
                            this,
                            &LocalFrameView::UpdatePluginsTimerFired),
      base_background_color_(Color::kWhite),
      media_type_(MediaTypeNames::screen),
      safe_to_propagate_scroll_to_parent_(true),
      scroll_corner_(nullptr),
      sticky_position_object_count_(0),
      input_events_scale_factor_for_emulation_(1),
      layout_size_fixed_to_frame_size_(true),
      did_scroll_timer_(frame.GetTaskRunner(TaskType::kInternalDefault),
                        this,
                        &LocalFrameView::DidScrollTimerFired),
      needs_update_geometries_(false),
      horizontal_scrollbar_mode_(kScrollbarAuto),
      vertical_scrollbar_mode_(kScrollbarAuto),
      scrollbars_suppressed_(false),
      root_layer_did_scroll_(false),
      in_update_scrollbars_(false),
      frame_timing_requests_dirty_(true),
      hidden_for_throttling_(false),
      subtree_throttled_(false),
      lifecycle_updates_throttled_(false),
      needs_paint_property_update_(true),
      current_update_lifecycle_phases_target_state_(
          DocumentLifecycle::kUninitialized),
      past_layout_lifecycle_update_(false),
      scroll_anchor_(this),
      scrollbar_manager_(*this),
      needs_scrollbars_update_(false),
      suppress_adjust_view_size_(false),
      allows_layout_invalidation_after_layout_clean_(true),
      needs_intersection_observation_(false),
      needs_forced_compositing_update_(false),
      needs_focus_on_fragment_(false),
      main_thread_scrolling_reasons_(0),
      paint_frame_count_(0),
      unique_id_(NewUniqueObjectId()),
      jank_tracker_(this) {
  Init();
}

LocalFrameView* LocalFrameView::Create(LocalFrame& frame) {
  LocalFrameView* view = new LocalFrameView(frame, IntRect());
  view->Show();
  return view;
}

LocalFrameView* LocalFrameView::Create(LocalFrame& frame,
                                       const IntSize& initial_size) {
  LocalFrameView* view =
      new LocalFrameView(frame, IntRect(IntPoint(), initial_size));
  view->SetLayoutSizeInternal(initial_size);

  view->Show();
  return view;
}

LocalFrameView::~LocalFrameView() {
#if DCHECK_IS_ON()
  DCHECK(has_been_disposed_);
#endif
}

void LocalFrameView::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  visitor->Trace(parent_);
  visitor->Trace(fragment_anchor_);
  visitor->Trace(scrollable_areas_);
  visitor->Trace(animating_scrollable_areas_);
  visitor->Trace(auto_size_info_);
  visitor->Trace(plugins_);
  visitor->Trace(scrollbars_);
  visitor->Trace(viewport_scrollable_area_);
  visitor->Trace(visibility_observer_);
  visitor->Trace(scroll_anchor_);
  visitor->Trace(anchoring_adjustment_queue_);
  visitor->Trace(scrollbar_manager_);
  visitor->Trace(print_context_);
  ScrollableArea::Trace(visitor);
}

void LocalFrameView::Reset() {
  // The compositor throttles the main frame using deferred commits, we can't
  // throttle it here or it seems the root compositor doesn't get setup
  // properly.
  if (RuntimeEnabledFeatures::
          RenderingPipelineThrottlingLoadingIframesEnabled())
    lifecycle_updates_throttled_ = !GetFrame().IsMainFrame();
  has_pending_layout_ = false;
  layout_scheduling_enabled_ = true;
  in_synchronous_post_layout_ = false;
  layout_count_ = 0;
  nested_layout_count_ = 0;
  post_layout_tasks_timer_.Stop();
  update_plugins_timer_.Stop();
  first_layout_ = true;
  safe_to_propagate_scroll_to_parent_ = true;
  last_viewport_size_ = IntSize();
  last_zoom_factor_ = 1.0f;
  tracked_object_paint_invalidations_ =
      base::WrapUnique(g_initial_track_all_paint_invalidations
                           ? new Vector<ObjectPaintInvalidation>
                           : nullptr);
  visually_non_empty_character_count_ = 0;
  visually_non_empty_pixel_count_ = 0;
  is_visually_non_empty_ = false;
  main_thread_scrolling_reasons_ = 0;
  layout_object_counter_.Reset();
  ClearFragmentAnchor();
  viewport_constrained_objects_.reset();
  layout_subtree_root_list_.Clear();
  orthogonal_writing_mode_root_list_.Clear();
  ukm_time_aggregator_.reset();
}

template <typename Function>
void LocalFrameView::ForAllChildViewsAndPlugins(const Function& function) {
  for (Frame* child = frame_->Tree().FirstChild(); child;
       child = child->Tree().NextSibling()) {
    if (child->View())
      function(*child->View());
  }

  for (const auto& plugin : plugins_) {
    function(*plugin);
  }
}

template <typename Function>
void LocalFrameView::ForAllChildLocalFrameViews(const Function& function) {
  for (Frame* child = frame_->Tree().FirstChild(); child;
       child = child->Tree().NextSibling()) {
    if (!child->IsLocalFrame())
      continue;
    if (LocalFrameView* child_view = ToLocalFrame(child)->View())
      function(*child_view);
  }
}

// Call function for each non-throttled frame view in pre tree order.
template <typename Function>
void LocalFrameView::ForAllNonThrottledLocalFrameViews(
    const Function& function) {
  if (ShouldThrottleRendering())
    return;

  function(*this);

  for (Frame* child = frame_->Tree().FirstChild(); child;
       child = child->Tree().NextSibling()) {
    if (!child->IsLocalFrame())
      continue;
    if (LocalFrameView* child_view = ToLocalFrame(child)->View())
      child_view->ForAllNonThrottledLocalFrameViews(function);
  }
}

void LocalFrameView::Init() {
  Reset();

  size_ = LayoutSize();

  // Propagate the marginwidth/height and scrolling modes to the view.
  if (frame_->Owner() &&
      frame_->Owner()->ScrollingMode() == kScrollbarAlwaysOff)
    SetCanHaveScrollbars(false);
}

void LocalFrameView::SetupRenderThrottling() {
  if (visibility_observer_)
    return;

  // We observe the frame owner element instead of the document element, because
  // if the document has no content we can falsely think the frame is invisible.
  // Note that this means we cannot throttle top-level frames or (currently)
  // frames whose owner element is remote.
  Element* target_element = GetFrame().DeprecatedLocalOwner();
  if (!target_element)
    return;

  visibility_observer_ = new ElementVisibilityObserver(
      target_element, WTF::BindRepeating(
                          [](LocalFrameView* frame_view, bool is_visible) {
                            if (!frame_view)
                              return;
                            frame_view->UpdateRenderThrottlingStatus(
                                !is_visible, frame_view->subtree_throttled_);
                          },
                          WrapWeakPersistent(this)));
  visibility_observer_->Start();
}

void LocalFrameView::Dispose() {
  CHECK(!IsInPerformLayout());

  if (ScrollAnimatorBase* scroll_animator = ExistingScrollAnimator())
    scroll_animator->CancelAnimation();
  CancelProgrammaticScrollAnimation();

  DetachScrollbars();

  if (ScrollingCoordinator* scrolling_coordinator =
          this->GetScrollingCoordinator())
    scrolling_coordinator->WillDestroyScrollableArea(this);

  Page* page = frame_->GetPage();
  // TODO(dcheng): It's wrong that the frame can be detached before the
  // LocalFrameView. Figure out what's going on and fix LocalFrameView to be
  // disposed with the correct timing.
  if (page)
    page->GlobalRootScrollerController().DidDisposeScrollableArea(*this);

  // We need to clear the RootFrameViewport's animator since it gets called
  // from non-GC'd objects and RootFrameViewport will still have a pointer to
  // this class.
  if (viewport_scrollable_area_)
    viewport_scrollable_area_->ClearScrollableArea();

  ClearScrollableArea();

  // Destroy |m_autoSizeInfo| as early as possible, to avoid dereferencing
  // partially destroyed |this| via |m_autoSizeInfo->m_frameView|.
  auto_size_info_.Clear();

  post_layout_tasks_timer_.Stop();
  did_scroll_timer_.Stop();

  // FIXME: Do we need to do something here for OOPI?
  HTMLFrameOwnerElement* owner_element = frame_->DeprecatedLocalOwner();
  // TODO(dcheng): It seems buggy that we can have an owner element that points
  // to another EmbeddedContentView. This can happen when a plugin element loads
  // a frame (EmbeddedContentView A of type LocalFrameView) and then loads a
  // plugin (EmbeddedContentView B of type WebPluginContainerImpl). In this
  // case, the frame's view is A and the frame element's
  // OwnedEmbeddedContentView is B. See https://crbug.com/673170 for an example.
  if (owner_element && owner_element->OwnedEmbeddedContentView() == this)
    owner_element->SetEmbeddedContentView(nullptr);

  ClearPrintContext();

  ukm_time_aggregator_.reset();

#if DCHECK_IS_ON()
  has_been_disposed_ = true;
#endif
}

void LocalFrameView::DetachScrollbars() {
  // Previously, we detached custom scrollbars as early as possible to prevent
  // Document::detachLayoutTree() from messing with the view such that its
  // scroll bars won't be torn down. However, scripting in
  // Document::detachLayoutTree() is forbidden
  // now, so it's not clear if these edge cases can still happen.
  // However, for Oilpan, we still need to remove the native scrollbars before
  // we lose the connection to the ChromeClient, so we just unconditionally
  // detach any scrollbars now.
  scrollbar_manager_.Dispose();

  if (scroll_corner_) {
    scroll_corner_->Destroy();
    scroll_corner_ = nullptr;
  }
}

void LocalFrameView::ScrollbarManager::SetHasHorizontalScrollbar(
    bool has_scrollbar) {
  if (has_scrollbar == HasHorizontalScrollbar())
    return;

  if (has_scrollbar) {
    h_bar_ = CreateScrollbar(kHorizontalScrollbar);
    h_bar_is_attached_ = 1;
    scrollable_area_->DidAddScrollbar(*h_bar_, kHorizontalScrollbar);
    h_bar_->StyleChanged();
  } else {
    h_bar_is_attached_ = 0;
    DestroyScrollbar(kHorizontalScrollbar);
  }

  scrollable_area_->SetScrollCornerNeedsPaintInvalidation();
}

void LocalFrameView::ScrollbarManager::SetHasVerticalScrollbar(
    bool has_scrollbar) {
  if (has_scrollbar == HasVerticalScrollbar())
    return;

  if (has_scrollbar) {
    v_bar_ = CreateScrollbar(kVerticalScrollbar);
    v_bar_is_attached_ = 1;
    scrollable_area_->DidAddScrollbar(*v_bar_, kVerticalScrollbar);
    v_bar_->StyleChanged();
  } else {
    v_bar_is_attached_ = 0;
    DestroyScrollbar(kVerticalScrollbar);
  }

  scrollable_area_->SetScrollCornerNeedsPaintInvalidation();
}

Scrollbar* LocalFrameView::ScrollbarManager::CreateScrollbar(
    ScrollbarOrientation orientation) {
  Element* custom_scrollbar_element = nullptr;
  LayoutBox* box = scrollable_area_->GetLayoutBox();
  if (box->GetDocument().View()->ShouldUseCustomScrollbars(
          custom_scrollbar_element)) {
    return LayoutScrollbar::CreateCustomScrollbar(
        scrollable_area_.Get(), orientation, custom_scrollbar_element);
  }

  // Nobody set a custom style, so we just use a native scrollbar.
  return Scrollbar::Create(scrollable_area_.Get(), orientation,
                           kRegularScrollbar,
                           &box->GetFrame()->GetPage()->GetChromeClient());
}

void LocalFrameView::SnapAfterScrollbarDragging(
    ScrollbarOrientation orientation) {
  SnapCoordinator* snap_coordinator =
      frame_->GetDocument()->GetSnapCoordinator();
  if (!snap_coordinator)
    return;
  snap_coordinator->PerformSnapping(*GetLayoutBox(),
                                    orientation == kHorizontalScrollbar,
                                    orientation == kVerticalScrollbar);
}

void LocalFrameView::ScrollbarManager::DestroyScrollbar(
    ScrollbarOrientation orientation) {
  Member<Scrollbar>& scrollbar =
      orientation == kHorizontalScrollbar ? h_bar_ : v_bar_;
  DCHECK(orientation == kHorizontalScrollbar ? !h_bar_is_attached_
                                             : !v_bar_is_attached_);
  if (!scrollbar)
    return;

  scrollable_area_->WillRemoveScrollbar(*scrollbar, orientation);
  scrollbar->DisconnectFromScrollableArea();
  scrollbar = nullptr;
}

void LocalFrameView::RecalculateCustomScrollbarStyle() {
  bool did_style_change = false;
  if (HorizontalScrollbar() && HorizontalScrollbar()->IsCustomScrollbar()) {
    HorizontalScrollbar()->StyleChanged();
    did_style_change = true;
  }
  if (VerticalScrollbar() && VerticalScrollbar()->IsCustomScrollbar()) {
    VerticalScrollbar()->StyleChanged();
    did_style_change = true;
  }
  if (did_style_change) {
    UpdateScrollbarGeometry();
    UpdateScrollCorner();
    PositionScrollbarLayers();
  }
}

void LocalFrameView::InvalidateAllCustomScrollbarsOnActiveChanged() {
  bool uses_window_inactive_selector =
      frame_->GetDocument()->GetStyleEngine().UsesWindowInactiveSelector();

  ForAllChildLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.InvalidateAllCustomScrollbarsOnActiveChanged();
  });

  for (const auto& scrollbar : scrollbars_) {
    if (uses_window_inactive_selector && scrollbar->IsCustomScrollbar())
      scrollbar->StyleChanged();
  }

  if (uses_window_inactive_selector)
    RecalculateCustomScrollbarStyle();
}

bool LocalFrameView::DidFirstLayout() const {
  return !first_layout_;
}

bool LocalFrameView::LifecycleUpdatesActive() const {
  return !lifecycle_updates_throttled_;
}

void LocalFrameView::InvalidateRect(const IntRect& rect) {
  auto* layout_object = frame_->OwnerLayoutObject();
  if (!layout_object)
    return;

  IntRect paint_invalidation_rect = rect;
  paint_invalidation_rect.Move(
      (layout_object->BorderLeft() + layout_object->PaddingLeft()).ToInt(),
      (layout_object->BorderTop() + layout_object->PaddingTop()).ToInt());
  layout_object->InvalidatePaintRectangle(LayoutRect(paint_invalidation_rect));
}

void LocalFrameView::SetFrameRect(const IntRect& unclamped_frame_rect) {
  IntRect frame_rect(SaturatedRect(unclamped_frame_rect));
  if (frame_rect == frame_rect_)
    return;
  const bool width_changed = frame_rect_.Width() != frame_rect.Width();
  const bool height_changed = frame_rect_.Height() != frame_rect.Height();
  frame_rect_ = frame_rect;

  needs_scrollbars_update_ |= width_changed || height_changed;

  FrameRectsChanged();

  UpdateParentScrollableAreaSet();

  if (auto* layout_view = GetLayoutView())
    layout_view->SetMayNeedPaintInvalidation();

  if (width_changed || height_changed) {
    ViewportSizeChanged(width_changed, height_changed);

    if (frame_->IsMainFrame())
      frame_->GetPage()->GetVisualViewport().MainFrameDidChangeSize();

    GetFrame().Loader().RestoreScrollPositionAndViewState();
  }
}

IntPoint LocalFrameView::Location() const {
  IntPoint location(frame_rect_.Location());

  // As an optimization, we don't include the root layer's scroll offset in the
  // frame rect.  As a result, we don't need to recalculate the frame rect every
  // time the root layer scrolls, but we need to add it in here.
  LayoutEmbeddedContent* owner = frame_->OwnerLayoutObject();
  if (owner) {
    LayoutView* owner_layout_view = owner->View();
    DCHECK(owner_layout_view);
    if (owner_layout_view->HasOverflowClip()) {
      IntSize scroll_offset(owner_layout_view->ScrolledContentOffset());
      location.SaturatedMove(-scroll_offset.Width(), -scroll_offset.Height());
    }
  }
  return location;
}

Page* LocalFrameView::GetPage() const {
  return GetFrame().GetPage();
}

LayoutView* LocalFrameView::GetLayoutView() const {
  return GetFrame().ContentLayoutObject();
}

ScrollingCoordinator* LocalFrameView::GetScrollingCoordinator() const {
  Page* p = GetPage();
  return p ? p->GetScrollingCoordinator() : nullptr;
}

ScrollingCoordinatorContext* LocalFrameView::GetScrollingContext() const {
  LocalFrame* root = &GetFrame().LocalFrameRoot();
  if (GetFrame() != root)
    return root->View()->GetScrollingContext();

  if (!scrolling_context_)
    scrolling_context_.reset(new ScrollingCoordinatorContext());
  return scrolling_context_.get();
}

CompositorAnimationHost* LocalFrameView::GetCompositorAnimationHost() const {
  if (GetScrollingContext()->GetCompositorAnimationHost())
    return GetScrollingContext()->GetCompositorAnimationHost();

  if (!GetFrame().LocalFrameRoot().IsMainFrame())
    return nullptr;

  // TODO(kenrb): Compositor animation host and timeline for the main frame
  // still live on ScrollingCoordinator. https://crbug.com/680606.
  ScrollingCoordinator* c = GetScrollingCoordinator();
  return c ? c->GetCompositorAnimationHost() : nullptr;
}

CompositorAnimationTimeline* LocalFrameView::GetCompositorAnimationTimeline()
    const {
  if (GetScrollingContext()->GetCompositorAnimationTimeline())
    return GetScrollingContext()->GetCompositorAnimationTimeline();

  if (!GetFrame().LocalFrameRoot().IsMainFrame())
    return nullptr;

  // TODO(kenrb): Compositor animation host and timeline for the main frame
  // still live on ScrollingCoordinator. https://crbug.com/680606.
  ScrollingCoordinator* c = GetScrollingCoordinator();
  return c ? c->GetCompositorAnimationTimeline() : nullptr;
}

LayoutBox* LocalFrameView::GetLayoutBox() const {
  return GetLayoutView();
}

FloatQuad LocalFrameView::LocalToVisibleContentQuad(
    const FloatQuad& quad,
    const LayoutObject* local_object,
    MapCoordinatesFlags flags) const {
  LayoutBox* box = GetLayoutBox();
  if (!box)
    return quad;
  DCHECK(local_object);
  FloatQuad result = local_object->LocalToAncestorQuad(quad, box, flags);
  result.Move(-GetScrollOffset());
  return result;
}

scoped_refptr<base::SingleThreadTaskRunner> LocalFrameView::GetTimerTaskRunner()
    const {
  return frame_->GetTaskRunner(TaskType::kInternalDefault);
}

void LocalFrameView::SetCanHaveScrollbars(bool can_have_scrollbars) {
  can_have_scrollbars_ = can_have_scrollbars;

  ScrollbarMode new_vertical_mode = EffectiveVerticalScrollbarMode();
  if (can_have_scrollbars && new_vertical_mode == kScrollbarAlwaysOff)
    new_vertical_mode = kScrollbarAuto;
  else if (!can_have_scrollbars)
    new_vertical_mode = kScrollbarAlwaysOff;

  ScrollbarMode new_horizontal_mode = EffectiveHorizontalScrollbarMode();
  if (can_have_scrollbars && new_horizontal_mode == kScrollbarAlwaysOff)
    new_horizontal_mode = kScrollbarAuto;
  else if (!can_have_scrollbars)
    new_horizontal_mode = kScrollbarAlwaysOff;

  SetScrollbarModes(new_horizontal_mode, new_vertical_mode);
}

bool LocalFrameView::ShouldUseCustomScrollbars(
    Element*& custom_scrollbar_element) const {
  custom_scrollbar_element = nullptr;

  if (Settings* settings = frame_->GetSettings()) {
    if (!settings->GetAllowCustomScrollbarInMainFrame() &&
        frame_->IsMainFrame())
      return false;
  }
  Document* doc = frame_->GetDocument();

  // Try the <body> element first as a scrollbar source.
  Element* body = doc ? doc->body() : nullptr;
  if (body && body->GetLayoutObject() &&
      body->GetLayoutObject()->Style()->HasPseudoStyle(kPseudoIdScrollbar)) {
    custom_scrollbar_element = body;
    return true;
  }

  // If the <body> didn't have a custom style, then the root element might.
  Element* doc_element = doc ? doc->documentElement() : nullptr;
  if (doc_element && doc_element->GetLayoutObject() &&
      doc_element->GetLayoutObject()->Style()->HasPseudoStyle(
          kPseudoIdScrollbar)) {
    custom_scrollbar_element = doc_element;
    return true;
  }

  return false;
}

Scrollbar* LocalFrameView::CreateScrollbar(ScrollbarOrientation orientation) {
  return scrollbar_manager_.CreateScrollbar(orientation);
}

void LocalFrameView::SetLayoutOverflowSize(const IntSize& size) {
  if (size == layout_overflow_size_)
    return;

  layout_overflow_size_ = size;
  needs_scrollbars_update_ = true;
  ScrollableArea::ContentsResized();

  Page* page = GetFrame().GetPage();
  if (!page)
    return;

  page->GetChromeClient().ContentsSizeChanged(frame_.Get(), size);
}

void LocalFrameView::AdjustViewSize() {
  if (suppress_adjust_view_size_)
    return;

  LayoutView* layout_view = GetLayoutView();
  if (!layout_view)
    return;

  DCHECK_EQ(frame_->View(), this);

  const IntRect rect = layout_view->DocumentRect();
  const IntSize& size = rect.Size();

  if (!RuntimeEnabledFeatures::RootLayerScrollingEnabled()) {
    const IntPoint origin(-rect.X(), -rect.Y());
    if (ScrollOrigin() != origin)
      SetScrollOrigin(origin);
  }

  SetLayoutOverflowSize(size);
}

void LocalFrameView::AdjustViewSizeAndLayout() {
  AdjustViewSize();
  if (NeedsLayout()) {
    base::AutoReset<bool> suppress_adjust_view_size(&suppress_adjust_view_size_,
                                                    true);
    UpdateLayout();
  }
}

void LocalFrameView::UpdateAcceleratedCompositingSettings() {
  if (auto* layout_view = GetLayoutView())
    layout_view->Compositor()->UpdateAcceleratedCompositingSettings();
}

void LocalFrameView::RecalcOverflowAfterStyleChange() {
  auto* layout_view = this->GetLayoutView();
  CHECK(layout_view);
  if (!layout_view->NeedsOverflowRecalcAfterStyleChange())
    return;

  layout_view->RecalcOverflowAfterStyleChange();

  // Changing overflow should notify scrolling coordinator to ensures that it
  // updates non-fast scroll rects even if there is no layout.
  if (ScrollingCoordinator* scrolling_coordinator =
          this->GetScrollingCoordinator()) {
    GetScrollingContext()->SetScrollGestureRegionIsDirty(true);
  }

  IntRect document_rect = layout_view->DocumentRect();
  if (ScrollOrigin() == -document_rect.Location() &&
      ContentsSize() == document_rect.Size())
    return;

  if (NeedsLayout())
    return;

  // If the visualViewport supplies scrollbars, we won't get a paint
  // invalidation from computeScrollbarExistence so we need to force one.
  if (VisualViewportSuppliesScrollbars())
    layout_view->SetMayNeedPaintInvalidation();

  // TODO(pdr): This should be refactored to just block scrollbar updates as
  // we are not in a scrollbar update here and m_inUpdateScrollbars has other
  // side effects. This scope is only for preventing a synchronous layout from
  // scroll origin changes which would not be allowed during style recalc.
  InUpdateScrollbarsScope in_update_scrollbars_scope(this);

  bool should_have_horizontal_scrollbar = false;
  bool should_have_vertical_scrollbar = false;
  ComputeScrollbarExistence(should_have_horizontal_scrollbar,
                            should_have_vertical_scrollbar,
                            document_rect.Size());

  bool has_horizontal_scrollbar = HorizontalScrollbar();
  bool has_vertical_scrollbar = VerticalScrollbar();
  if (has_horizontal_scrollbar != should_have_horizontal_scrollbar ||
      has_vertical_scrollbar != should_have_vertical_scrollbar) {
    SetNeedsLayout();
    return;
  }

  AdjustViewSize();
  UpdateScrollbarGeometry();
  SetNeedsPaintPropertyUpdate();

  if (ScrollOriginChanged())
    SetNeedsLayout();
}

void LocalFrameView::UpdateCountersAfterStyleChange() {
  auto* layout_view = GetLayoutView();
  DCHECK(layout_view);
  layout_view->UpdateCounters();
}

bool LocalFrameView::ShouldScrollOnMainThread() const {
  if (GetMainThreadScrollingReasons())
    return true;
  return ScrollableArea::ShouldScrollOnMainThread();
}

bool LocalFrameView::IsEnclosedInCompositingLayer() const {
  // FIXME: It's a bug that compositing state isn't always up to date when this
  // is called. crbug.com/366314
  DisableCompositingQueryAsserts disabler;

  auto* frame_owner_layout_object = frame_->OwnerLayoutObject();
  return frame_owner_layout_object &&
         frame_owner_layout_object->EnclosingLayer()
             ->EnclosingLayerForPaintInvalidationCrossingFrameBoundaries();
}

void LocalFrameView::CountObjectsNeedingLayout(unsigned& needs_layout_objects,
                                               unsigned& total_objects,
                                               bool& is_subtree) {
  needs_layout_objects = 0;
  total_objects = 0;
  is_subtree = IsSubtreeLayout();
  if (is_subtree) {
    layout_subtree_root_list_.CountObjectsNeedingLayout(needs_layout_objects,
                                                        total_objects);
  } else {
    LayoutSubtreeRootList::CountObjectsNeedingLayoutInRoot(
        GetLayoutView(), needs_layout_objects, total_objects);
  }
}

void LocalFrameView::PerformPreLayoutTasks() {
  TRACE_EVENT0("blink,benchmark", "LocalFrameView::performPreLayoutTasks");
  Lifecycle().AdvanceTo(DocumentLifecycle::kInPreLayout);

  // Don't schedule more layouts, we're in one.
  base::AutoReset<bool> change_scheduling_enabled(&layout_scheduling_enabled_,
                                                  false);

  if (!nested_layout_count_ && !in_synchronous_post_layout_ &&
      post_layout_tasks_timer_.IsActive()) {
    // This is a new top-level layout. If there are any remaining tasks from the
    // previous layout, finish them now.
    in_synchronous_post_layout_ = true;
    PerformPostLayoutTasks();
    in_synchronous_post_layout_ = false;
  }

  bool was_resized = WasViewportResized();
  Document* document = frame_->GetDocument();
  if (was_resized)
    document->SetResizedForViewportUnits();

  // Viewport-dependent or device-dependent media queries may cause us to need
  // completely different style information.
  bool main_frame_rotation =
      frame_->IsMainFrame() && frame_->GetSettings() &&
      frame_->GetSettings()->GetMainFrameResizesAreOrientationChanges();
  if ((was_resized &&
       document->GetStyleEngine().MediaQueryAffectedByViewportChange()) ||
      (was_resized && main_frame_rotation &&
       document->GetStyleEngine().MediaQueryAffectedByDeviceChange())) {
    document->MediaQueryAffectingValueChanged();
  } else if (was_resized) {
    document->EvaluateMediaQueryList();
  }

  document->UpdateStyleAndLayoutTree();
  Lifecycle().AdvanceTo(DocumentLifecycle::kStyleClean);

  if (was_resized)
    document->ClearResizedForViewportUnits();

  if (ShouldPerformScrollAnchoring())
    scroll_anchor_.NotifyBeforeLayout();
}

bool LocalFrameView::ShouldPerformScrollAnchoring() const {
  return !RuntimeEnabledFeatures::RootLayerScrollingEnabled() &&
         scroll_anchor_.HasScroller() && GetLayoutBox() &&
         GetLayoutBox()->Style()->OverflowAnchor() != EOverflowAnchor::kNone &&
         !frame_->GetDocument()->FinishingOrIsPrinting();
}

void LocalFrameView::LayoutFromRootObject(LayoutObject& root) {
  LayoutState layout_state(root);
  if (!root.IsBoxModelObject()) {
    root.UpdateLayout();
  } else {
    // Laying out the root may change its visual overflow. If so, that
    // visual overflow needs to propagate to its containing block.
    LayoutBoxModelObject& box_object = ToLayoutBoxModelObject(root);
    LayoutRect previous_visual_overflow_rect = box_object.VisualOverflowRect();
    box_object.UpdateLayout();
    if (box_object.VisualOverflowRect() != previous_visual_overflow_rect) {
      box_object.SetNeedsOverflowRecalcAfterStyleChange();
      RecalcOverflowAfterStyleChange();
    }
  }
}

void LocalFrameView::PrepareLayoutAnalyzer() {
  bool is_tracing = false;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(
      TRACE_DISABLED_BY_DEFAULT("blink.debug.layout"), &is_tracing);
  if (!is_tracing) {
    analyzer_.reset();
    return;
  }
  if (!analyzer_)
    analyzer_ = std::make_unique<LayoutAnalyzer>();
  analyzer_->Reset();
}

std::unique_ptr<TracedValue> LocalFrameView::AnalyzerCounters() {
  if (!analyzer_)
    return TracedValue::Create();
  std::unique_ptr<TracedValue> value = analyzer_->ToTracedValue();
  value->SetString("host", GetLayoutView()->GetDocument().location()->host());
  value->SetString(
      "frame",
      String::Format("0x%" PRIxPTR, reinterpret_cast<uintptr_t>(frame_.Get())));
  value->SetInteger("contentsHeightAfterLayout",
                    GetLayoutView()->DocumentRect().Height());
  value->SetInteger("visibleHeight", VisibleHeight());
  value->SetInteger("approximateBlankCharacterCount",
                    FontFaceSetDocument::ApproximateBlankCharacterCount(
                        *frame_->GetDocument()));
  return value;
}

#define PERFORM_LAYOUT_TRACE_CATEGORIES \
  "blink,benchmark,rail," TRACE_DISABLED_BY_DEFAULT("blink.debug.layout")

void LocalFrameView::PerformLayout(bool in_subtree_layout) {
  DCHECK(in_subtree_layout || layout_subtree_root_list_.IsEmpty());

  int contents_height_before_layout = GetLayoutView()->DocumentRect().Height();
  TRACE_EVENT_BEGIN1(
      PERFORM_LAYOUT_TRACE_CATEGORIES, "LocalFrameView::performLayout",
      "contentsHeightBeforeLayout", contents_height_before_layout);
  PrepareLayoutAnalyzer();

  ScriptForbiddenScope forbid_script;

  if (in_subtree_layout && HasOrthogonalWritingModeRoots()) {
    // If we're going to lay out from each subtree root, rather than once from
    // LayoutView, we need to merge the depth-ordered orthogonal writing mode
    // root list into the depth-ordered list of subtrees scheduled for
    // layout. Otherwise, during layout of one such subtree, we'd risk skipping
    // over a subtree of objects needing layout.
    DCHECK(!layout_subtree_root_list_.IsEmpty());
    ScheduleOrthogonalWritingModeRootsForLayout();
  }

  DCHECK(!IsInPerformLayout());
  Lifecycle().AdvanceTo(DocumentLifecycle::kInPerformLayout);

  // performLayout is the actual guts of layout().
  // FIXME: The 300 other lines in layout() probably belong in other helper
  // functions so that a single human could understand what layout() is actually
  // doing.

  {
    // TODO(szager): Remove this after diagnosing crash.
    DocumentLifecycle::CheckNoTransitionScope check_no_transition(Lifecycle());
    if (in_subtree_layout) {
      if (analyzer_) {
        analyzer_->Increment(LayoutAnalyzer::kPerformLayoutRootLayoutObjects,
                             layout_subtree_root_list_.size());
      }
      for (auto& root : layout_subtree_root_list_.Ordered()) {
        if (!root->NeedsLayout())
          continue;
        LayoutFromRootObject(*root);

        // We need to ensure that we mark up all layoutObjects up to the
        // LayoutView for paint invalidation. This simplifies our code as we
        // just always do a full tree walk.
        if (LayoutObject* container = root->Container())
          container->SetMayNeedPaintInvalidation();
      }
      layout_subtree_root_list_.Clear();
    } else {
      if (HasOrthogonalWritingModeRoots() &&
          !RuntimeEnabledFeatures::LayoutNGEnabled())
        LayoutOrthogonalWritingModeRoots();
      GetLayoutView()->UpdateLayout();
    }
  }

  frame_->GetDocument()->Fetcher()->UpdateAllImageResourcePriorities();

  Lifecycle().AdvanceTo(DocumentLifecycle::kAfterPerformLayout);

  TRACE_EVENT_END1(PERFORM_LAYOUT_TRACE_CATEGORIES,
                   "LocalFrameView::performLayout", "counters",
                   AnalyzerCounters());
  FirstMeaningfulPaintDetector::From(*frame_->GetDocument())
      .MarkNextPaintAsMeaningfulIfNeeded(
          layout_object_counter_, contents_height_before_layout,
          GetLayoutView()->DocumentRect().Height(), VisibleHeight());
}

void LocalFrameView::ScheduleOrPerformPostLayoutTasks() {
  if (post_layout_tasks_timer_.IsActive())
    return;

  if (!in_synchronous_post_layout_) {
    in_synchronous_post_layout_ = true;
    // Calls resumeScheduledEvents()
    PerformPostLayoutTasks();
    in_synchronous_post_layout_ = false;
  }

  if (!post_layout_tasks_timer_.IsActive() &&
      (NeedsLayout() || in_synchronous_post_layout_)) {
    // If we need layout or are already in a synchronous call to
    // postLayoutTasks(), defer LocalFrameView updates and event dispatch until
    // after we return.  postLayoutTasks() can make us need to update again, and
    // we can get stuck in a nasty cycle unless we call it through the timer
    // here.
    post_layout_tasks_timer_.StartOneShot(TimeDelta(), FROM_HERE);
    if (NeedsLayout())
      UpdateLayout();
  }
}

void LocalFrameView::UpdateLayout() {
  // We should never layout a Document which is not in a LocalFrame.
  DCHECK(frame_);
  DCHECK_EQ(frame_->View(), this);
  DCHECK(frame_->GetPage());

  {
    ScriptForbiddenScope forbid_script;

    if (IsInPerformLayout() || ShouldThrottleRendering() ||
        !frame_->GetDocument()->IsActive())
      return;

    TRACE_EVENT0("blink,benchmark", "LocalFrameView::layout");

    RUNTIME_CALL_TIMER_SCOPE(V8PerIsolateData::MainThreadIsolate(),
                             RuntimeCallStats::CounterId::kUpdateLayout);

    // The actual call to UpdateGeometries is in PerformPostLayoutTasks.
    SetNeedsUpdateGeometries();

    if (auto_size_info_)
      auto_size_info_->AutoSizeIfNeeded();

    has_pending_layout_ = false;

    Document* document = frame_->GetDocument();
    TRACE_EVENT_BEGIN1("devtools.timeline", "Layout", "beginData",
                       InspectorLayoutEvent::BeginData(this));
    probe::UpdateLayout probe(document);

    PerformPreLayoutTasks();

    VisualViewport& visual_viewport = frame_->GetPage()->GetVisualViewport();
    DoubleSize viewport_size(visual_viewport.VisibleWidthCSSPx(),
                             visual_viewport.VisibleHeightCSSPx());

    // TODO(crbug.com/460956): The notion of a single root for layout is no
    // longer applicable. Remove or update this code.
    LayoutObject* root_for_this_layout = GetLayoutView();

    FontCachePurgePreventer font_cache_purge_preventer;
    {
      base::AutoReset<bool> change_scheduling_enabled(
          &layout_scheduling_enabled_, false);
      nested_layout_count_++;

      // If the layout view was marked as needing layout after we added items in
      // the subtree roots we need to clear the roots and do the layout from the
      // layoutView.
      if (GetLayoutView()->NeedsLayout())
        ClearLayoutSubtreeRootsAndMarkContainingBlocks();
      GetLayoutView()->ClearHitTestCache();

      bool in_subtree_layout = IsSubtreeLayout();

      // TODO(crbug.com/460956): The notion of a single root for layout is no
      // longer applicable. Remove or update this code.
      if (in_subtree_layout)
        root_for_this_layout = layout_subtree_root_list_.RandomRoot();

      if (!root_for_this_layout) {
        // FIXME: Do we need to set m_size here?
        NOTREACHED();
        return;
      }

      if (!in_subtree_layout) {
        ClearLayoutSubtreeRootsAndMarkContainingBlocks();
        Node* body = document->body();
        if (body && body->GetLayoutObject()) {
          if (IsHTMLFrameSetElement(*body)) {
            body->GetLayoutObject()->SetChildNeedsLayout();
          } else if (IsHTMLBodyElement(*body)) {
            if (!first_layout_ && size_.Height() != GetLayoutSize().Height() &&
                body->GetLayoutObject()->EnclosingBox()->StretchesToViewport())
              body->GetLayoutObject()->SetChildNeedsLayout();
          }
        }

        ScrollbarMode h_mode;
        ScrollbarMode v_mode;
        GetLayoutView()->CalculateScrollbarModes(h_mode, v_mode);

        // Now set our scrollbar state for the layout.
        ScrollbarMode current_h_mode = EffectiveHorizontalScrollbarMode();
        ScrollbarMode current_v_mode = EffectiveVerticalScrollbarMode();

        if (first_layout_) {
          SetScrollbarsSuppressed(true);

          first_layout_ = false;
          last_viewport_size_ = GetLayoutSize(kIncludeScrollbars);
          last_zoom_factor_ = GetLayoutView()->Style()->Zoom();

          // Set the initial vMode to AlwaysOn if we're auto.
          if (v_mode == kScrollbarAuto) {
            // This causes a vertical scrollbar to appear.
            SetVerticalScrollbarMode(kScrollbarAlwaysOn);
            if (RuntimeEnabledFeatures::RootLayerScrollingEnabled()) {
              GetLayoutView()
                  ->GetScrollableArea()
                  ->ForceVerticalScrollbarForFirstLayout();
            }
          }
          // Set the initial hMode to AlwaysOff if we're auto.
          if (h_mode == kScrollbarAuto) {
            // This causes a horizontal scrollbar to disappear.
            SetHorizontalScrollbarMode(kScrollbarAlwaysOff);
          }

          SetScrollbarModes(h_mode, v_mode);
          SetScrollbarsSuppressed(false);
        } else if (h_mode != current_h_mode || v_mode != current_v_mode) {
          SetScrollbarModes(h_mode, v_mode);
        }

        UpdateScrollbarsIfNeeded();

        LayoutSize old_size = size_;

        size_ = LayoutSize(GetLayoutSize());

        if (old_size != size_ && !first_layout_) {
          LayoutBox* root_layout_object =
              document->documentElement()
                  ? document->documentElement()->GetLayoutBox()
                  : nullptr;
          LayoutBox* body_layout_object = root_layout_object && document->body()
                                              ? document->body()->GetLayoutBox()
                                              : nullptr;
          if (body_layout_object && body_layout_object->StretchesToViewport())
            body_layout_object->SetChildNeedsLayout();
          else if (root_layout_object &&
                   root_layout_object->StretchesToViewport())
            root_layout_object->SetChildNeedsLayout();
        }
      }

      TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
          TRACE_DISABLED_BY_DEFAULT("blink.debug.layout.trees"), "LayoutTree",
          this, TracedLayoutObject::Create(*GetLayoutView(), false));

      IntSize old_size(Size());

      PerformLayout(in_subtree_layout);

      UpdateScrollbars();
      UpdateParentScrollableAreaSet();

      IntSize new_size(Size());
      if (old_size != new_size) {
        needs_scrollbars_update_ = true;
        SetNeedsLayout();
        MarkViewportConstrainedObjectsForLayout(
            old_size.Width() != new_size.Width(),
            old_size.Height() != new_size.Height());
      }

      if (NeedsLayout()) {
        base::AutoReset<bool> suppress(&suppress_adjust_view_size_, true);
        UpdateLayout();
      }

      DCHECK(layout_subtree_root_list_.IsEmpty());
    }  // Reset m_layoutSchedulingEnabled to its previous value.
    CheckDoesNotNeedLayout();

    DocumentLifecycle::Scope lifecycle_scope(Lifecycle(),
                                             DocumentLifecycle::kLayoutClean);

    frame_timing_requests_dirty_ = true;

    // FIXME: Could find the common ancestor layer of all dirty subtrees and
    // mark from there. crbug.com/462719
    GetLayoutView()->EnclosingLayer()->UpdateLayerPositionsAfterLayout();

    TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID(
        TRACE_DISABLED_BY_DEFAULT("blink.debug.layout.trees"), "LayoutTree",
        this, TracedLayoutObject::Create(*GetLayoutView(), true));

    GetLayoutView()->Compositor()->DidLayout();

    layout_count_++;

    if (AXObjectCache* cache = document->GetOrCreateAXObjectCache()) {
      const KURL& url = document->Url();
      if (url.IsValid() && !url.IsAboutBlankURL())
        cache->HandleLayoutComplete(document);
    }
    UpdateDocumentAnnotatedRegions();
    CheckDoesNotNeedLayout();

    ScheduleOrPerformPostLayoutTasks();
    CheckDoesNotNeedLayout();

    // FIXME: The notion of a single root for layout is no longer applicable.
    // Remove or update this code. crbug.com/460596
    TRACE_EVENT_END1("devtools.timeline", "Layout", "endData",
                     InspectorLayoutEvent::EndData(root_for_this_layout));
    probe::didChangeViewport(frame_.Get());

    nested_layout_count_--;
    if (nested_layout_count_)
      return;

#if DCHECK_IS_ON()
    // Post-layout assert that nobody was re-marked as needing layout during
    // layout.
    GetLayoutView()->AssertSubtreeIsLaidOut();
#endif

    if (frame_->IsMainFrame() &&
        RuntimeEnabledFeatures::VisualViewportAPIEnabled()) {
      // Scrollbars changing state can cause a visual viewport size change.
      DoubleSize new_viewport_size(visual_viewport.VisibleWidthCSSPx(),
                                   visual_viewport.VisibleHeightCSSPx());
      if (new_viewport_size != viewport_size)
        frame_->GetDocument()->EnqueueVisualViewportResizeEvent();
    }
  }  // ScriptForbiddenScope

  GetFrame().GetDocument()->LayoutUpdated();
  CheckDoesNotNeedLayout();
}

void LocalFrameView::SetNeedsPaintPropertyUpdate() {
  needs_paint_property_update_ = true;
  if (auto* layout_view = this->GetLayoutView()) {
    layout_view->SetNeedsPaintPropertyUpdate();
    return;
  }
  if (LayoutObject* owner = GetFrame().OwnerLayoutObject())
    owner->SetNeedsPaintPropertyUpdate();
}

void LocalFrameView::SetSubtreeNeedsPaintPropertyUpdate() {
  SetNeedsPaintPropertyUpdate();
  if (auto* layout_view = GetLayoutView())
    layout_view->SetSubtreeNeedsPaintPropertyUpdate();
}

FloatSize LocalFrameView::ViewportSizeForViewportUnits() const {
  float zoom = 1;
  if (!frame_->GetDocument() || !frame_->GetDocument()->Printing())
    zoom = GetFrame().PageZoomFactor();

  auto* layout_view = GetLayoutView();
  if (!layout_view)
    return FloatSize();

  FloatSize layout_size;
  layout_size.SetWidth(layout_view->ViewWidth(kIncludeScrollbars) / zoom);
  layout_size.SetHeight(layout_view->ViewHeight(kIncludeScrollbars) / zoom);

  BrowserControls& browser_controls = frame_->GetPage()->GetBrowserControls();
  if (browser_controls.PermittedState() != kWebBrowserControlsHidden) {
    // We use the layoutSize rather than frameRect to calculate viewport units
    // so that we get correct results on mobile where the page is laid out into
    // a rect that may be larger than the viewport (e.g. the 980px fallback
    // width for desktop pages). Since the layout height is statically set to
    // be the viewport with browser controls showing, we add the browser
    // controls height, compensating for page scale as well, since we want to
    // use the viewport with browser controls hidden for vh (to match Safari).
    int viewport_width = frame_->GetPage()->GetVisualViewport().Size().Width();
    if (frame_->IsMainFrame() && layout_size.Width() && viewport_width) {
      // TODO(bokan/eirage): BrowserControl height may need to account for the
      // zoom factor when use-zoom-for-dsf is enabled on Android. Confirm this
      // works correctly when that's turned on. https://crbug.com/737777.
      float page_scale_at_layout_width = viewport_width / layout_size.Width();
      layout_size.Expand(
          0, browser_controls.TotalHeight() / page_scale_at_layout_width);
    }
  }

  return layout_size;
}

FloatSize LocalFrameView::ViewportSizeForMediaQueries() const {
  FloatSize viewport_size(GetLayoutSize(kIncludeScrollbars));
  if (!frame_->GetDocument() || !frame_->GetDocument()->Printing())
    viewport_size.Scale(1 / GetFrame().PageZoomFactor());
  return viewport_size;
}

DocumentLifecycle& LocalFrameView::Lifecycle() const {
  DCHECK(frame_);
  DCHECK(frame_->GetDocument());
  return frame_->GetDocument()->Lifecycle();
}

LayoutSVGRoot* LocalFrameView::EmbeddedReplacedContent() const {
  auto* layout_view = this->GetLayoutView();
  if (!layout_view)
    return nullptr;

  LayoutObject* first_child = layout_view->FirstChild();
  if (!first_child || !first_child->IsBox())
    return nullptr;

  // Currently only embedded SVG documents participate in the size-negotiation
  // logic.
  return ToLayoutSVGRootOrNull(first_child);
}

bool LocalFrameView::GetIntrinsicSizingInfo(
    IntrinsicSizingInfo& intrinsic_sizing_info) const {
  if (LayoutSVGRoot* content_layout_object = EmbeddedReplacedContent()) {
    content_layout_object->UnscaledIntrinsicSizingInfo(intrinsic_sizing_info);
    return true;
  }
  return false;
}

bool LocalFrameView::HasIntrinsicSizingInfo() const {
  return EmbeddedReplacedContent();
}

void LocalFrameView::UpdateGeometry() {
  LayoutEmbeddedContent* layout = frame_->OwnerLayoutObject();
  if (!layout)
    return;

  bool did_need_layout = NeedsLayout();

  LayoutRect new_frame = layout->ReplacedContentRect();
  DCHECK(new_frame.Size() == RoundedIntSize(new_frame.Size()));
  bool bounds_will_change = LayoutSize(Size()) != new_frame.Size();

  // If frame bounds are changing mark the view for layout. Also check the
  // frame's page to make sure that the frame isn't in the process of being
  // destroyed. If iframe scrollbars needs reconstruction from native to custom
  // scrollbar, then also we need to layout the frameview.
  if (bounds_will_change || NeedsScrollbarReconstruction())
    SetNeedsLayout();

  layout->UpdateGeometry(*this);
  // If view needs layout, either because bounds have changed or possibly
  // indicating content size is wrong, we have to do a layout to set the right
  // LocalFrameView size.
  if (NeedsLayout())
    UpdateLayout();

  if (!did_need_layout && !ShouldThrottleRendering())
    CheckDoesNotNeedLayout();
}

void LocalFrameView::AddPartToUpdate(LayoutEmbeddedObject& object) {
  DCHECK(IsInPerformLayout());
  // Tell the DOM element that it needs a Plugin update.
  Node* node = object.GetNode();
  DCHECK(node);
  if (IsHTMLObjectElement(*node) || IsHTMLEmbedElement(*node))
    ToHTMLPlugInElement(node)->SetNeedsPluginUpdate(true);

  part_update_set_.insert(&object);
}

void LocalFrameView::SetDisplayMode(WebDisplayMode mode) {
  if (mode == display_mode_)
    return;

  display_mode_ = mode;

  if (frame_->GetDocument())
    frame_->GetDocument()->MediaQueryAffectingValueChanged();
}

void LocalFrameView::SetDisplayShape(DisplayShape display_shape) {
  if (display_shape == display_shape_)
    return;

  display_shape_ = display_shape;

  if (frame_->GetDocument())
    frame_->GetDocument()->MediaQueryAffectingValueChanged();
}

void LocalFrameView::SetMediaType(const AtomicString& media_type) {
  DCHECK(frame_->GetDocument());
  media_type_ = media_type;
  frame_->GetDocument()->MediaQueryAffectingValueChanged();
}

AtomicString LocalFrameView::MediaType() const {
  // See if we have an override type.
  if (frame_->GetSettings() &&
      !frame_->GetSettings()->GetMediaTypeOverride().IsEmpty())
    return AtomicString(frame_->GetSettings()->GetMediaTypeOverride());
  return media_type_;
}

void LocalFrameView::AdjustMediaTypeForPrinting(bool printing) {
  if (printing) {
    if (media_type_when_not_printing_.IsNull())
      media_type_when_not_printing_ = MediaType();
    SetMediaType(MediaTypeNames::print);
  } else {
    if (!media_type_when_not_printing_.IsNull())
      SetMediaType(media_type_when_not_printing_);
    media_type_when_not_printing_ = g_null_atom;
  }

  frame_->GetDocument()->SetNeedsStyleRecalc(
      kSubtreeStyleChange, StyleChangeReasonForTracing::Create(
                               StyleChangeReason::kStyleSheetChange));
}

bool LocalFrameView::ContentsInCompositedLayer() const {
  auto* layout_view = this->GetLayoutView();
  return layout_view &&
         layout_view->GetCompositingState() == kPaintsIntoOwnBacking;
}

void LocalFrameView::AddBackgroundAttachmentFixedObject(LayoutObject* object) {
  DCHECK(!background_attachment_fixed_objects_.Contains(object));

  background_attachment_fixed_objects_.insert(object);
  if (ScrollingCoordinator* scrolling_coordinator =
          this->GetScrollingCoordinator()) {
    scrolling_coordinator
        ->FrameViewHasBackgroundAttachmentFixedObjectsDidChange(this);
  }

  // Ensure main thread scrolling reasons are recomputed.
  SetNeedsPaintPropertyUpdate();
  // The object's scroll properties are not affected by its own background.
  object->SetAncestorsNeedPaintPropertyUpdateForMainThreadScrolling();
}

void LocalFrameView::RemoveBackgroundAttachmentFixedObject(
    LayoutObject* object) {
  DCHECK(background_attachment_fixed_objects_.Contains(object));

  background_attachment_fixed_objects_.erase(object);
  if (ScrollingCoordinator* scrolling_coordinator =
          this->GetScrollingCoordinator()) {
    scrolling_coordinator
        ->FrameViewHasBackgroundAttachmentFixedObjectsDidChange(this);
  }

  // Ensure main thread scrolling reasons are recomputed.
  SetNeedsPaintPropertyUpdate();
  // The object's scroll properties are not affected by its own background.
  object->SetAncestorsNeedPaintPropertyUpdateForMainThreadScrolling();
}

void LocalFrameView::AddViewportConstrainedObject(LayoutObject& object) {
  if (!viewport_constrained_objects_) {
    viewport_constrained_objects_ =
        std::make_unique<ViewportConstrainedObjectSet>();
  }

  if (!viewport_constrained_objects_->Contains(&object)) {
    viewport_constrained_objects_->insert(&object);

    if (ScrollingCoordinator* scrolling_coordinator =
            this->GetScrollingCoordinator())
      scrolling_coordinator->FrameViewFixedObjectsDidChange(this);
  }
}

void LocalFrameView::RemoveViewportConstrainedObject(LayoutObject& object) {
  if (viewport_constrained_objects_ &&
      viewport_constrained_objects_->Contains(&object)) {
    viewport_constrained_objects_->erase(&object);

    if (ScrollingCoordinator* scrolling_coordinator =
            this->GetScrollingCoordinator())
      scrolling_coordinator->FrameViewFixedObjectsDidChange(this);
  }
}

void LocalFrameView::ViewportSizeChanged(bool width_changed,
                                         bool height_changed) {
  DCHECK(width_changed || height_changed);
  DCHECK(frame_->GetPage());
  if (frame_->GetDocument() &&
      frame_->GetDocument()->Lifecycle().LifecyclePostponed())
    return;

  bool root_layer_scrolling_enabled =
      RuntimeEnabledFeatures::RootLayerScrollingEnabled();

  if (LayoutView* layout_view = this->GetLayoutView()) {
    // If this is the main frame, we might have got here by hiding/showing the
    // top controls.  In that case, layout won't be triggered, so we need to
    // clamp the scroll offset here.
    if (GetFrame().IsMainFrame()) {
      if (root_layer_scrolling_enabled) {
        layout_view->Layer()->UpdateSize();
        layout_view->GetScrollableArea()
            ->ClampScrollOffsetAfterOverflowChange();
      } else {
        AdjustScrollOffsetFromUpdateScrollbars();
      }
    }

    if (layout_view->UsesCompositing()) {
      if (root_layer_scrolling_enabled) {
        layout_view->Layer()->SetNeedsCompositingInputsUpdate();
        SetNeedsPaintPropertyUpdate();
      }
    }
  }

  if (GetFrame().GetDocument())
    GetFrame().GetDocument()->GetRootScrollerController().DidResizeFrameView();

  ShowOverlayScrollbars();

  if (GetLayoutView() && frame_->IsMainFrame() &&
      frame_->GetPage()->GetBrowserControls().TotalHeight()) {
    if (GetLayoutView()->Style()->HasFixedBackgroundImage()) {
      // We've already issued a full invalidation above.
      GetLayoutView()->SetShouldDoFullPaintInvalidationOnResizeIfNeeded(
          width_changed, height_changed);
    } else if (height_changed) {
      // If the document rect doesn't fill the full view height, hiding the
      // URL bar will expose area outside the current LayoutView so we need to
      // paint additional background. If RLS is on, we've already invalidated
      // above.
      auto* layout_view = GetLayoutView();
      DCHECK(layout_view);
      if (layout_view->DocumentRect().Height() <
          layout_view->ViewRect().Height()) {
        layout_view->SetShouldDoFullPaintInvalidation(
            PaintInvalidationReason::kGeometry);
      }
    }
  }

  if (GetFrame().GetDocument() && !IsInPerformLayout())
    MarkViewportConstrainedObjectsForLayout(width_changed, height_changed);
}

void LocalFrameView::MarkViewportConstrainedObjectsForLayout(
    bool width_changed,
    bool height_changed) {
  if (!HasViewportConstrainedObjects() || !(width_changed || height_changed))
    return;

  for (auto* const viewport_constrained_object :
       *viewport_constrained_objects_) {
    LayoutObject* layout_object = viewport_constrained_object;
    const ComputedStyle& style = layout_object->StyleRef();
    if (width_changed) {
      if (style.Width().IsFixed() &&
          (style.Left().IsAuto() || style.Right().IsAuto())) {
        layout_object->SetNeedsPositionedMovementLayout();
      } else {
        layout_object->SetNeedsLayoutAndFullPaintInvalidation(
            LayoutInvalidationReason::kSizeChanged);
      }
    }
    if (height_changed) {
      if (style.Height().IsFixed() &&
          (style.Top().IsAuto() || style.Bottom().IsAuto())) {
        layout_object->SetNeedsPositionedMovementLayout();
      } else {
        layout_object->SetNeedsLayoutAndFullPaintInvalidation(
            LayoutInvalidationReason::kSizeChanged);
      }
    }
  }
}

IntPoint LocalFrameView::LastKnownMousePosition() const {
  return frame_->GetEventHandler().LastKnownMousePositionInRootFrame();
}

bool LocalFrameView::ShouldSetCursor() const {
  Page* page = GetFrame().GetPage();
  return page &&
         page->VisibilityState() != mojom::PageVisibilityState::kHidden &&
         !frame_->GetEventHandler().IsMousePositionUnknown() &&
         page->GetFocusController().IsActive();
}

void LocalFrameView::NotifyFrameRectsChangedIfNeededRecursive() {
  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.NotifyFrameRectsChangedIfNeeded();
  });
}

void LocalFrameView::ScrollContentsIfNeededRecursive() {
  ForAllNonThrottledLocalFrameViews(
      [](LocalFrameView& frame_view) { frame_view.ScrollContentsIfNeeded(); });
}

void LocalFrameView::InvalidateBackgroundAttachmentFixedDescendants(
    const LayoutObject& object) {
  for (auto* const layout_object : background_attachment_fixed_objects_) {
    if (object != GetLayoutView() && !layout_object->IsDescendantOf(&object))
      continue;

    bool needs_scrolling_contents_layer_invalidation = false;
    if (layout_object->HasLayer()) {
      PaintLayer* layer = ToLayoutBoxModelObject(layout_object)->Layer();
      if (layer->GetBackgroundPaintLocation() ==
          kBackgroundPaintInScrollingContents) {
        needs_scrolling_contents_layer_invalidation = true;
      }
    }
    if (needs_scrolling_contents_layer_invalidation) {
      // BoxPaintInvalidator doesn't want to invalidate scrolling contents layer
      // whenever the LayoutObject is marked ShouldDoFullPaintInvalidation() -
      // see crrev.com/433093.  (LayoutObject doesn't track full-invalidation
      // reasons independently, so it's not safe for BoxPaintInvalidator to have
      // special handling of kBackground.)
      layout_object->SetBackgroundChangedSinceLastPaintInvalidation();
    } else {
      layout_object->SetShouldDoFullPaintInvalidation(
          PaintInvalidationReason::kBackground);
    }
  }
}

bool LocalFrameView::HasBackgroundAttachmentFixedDescendants(
    const LayoutObject& object) const {
  if (object == GetLayoutView())
    return !background_attachment_fixed_objects_.IsEmpty();

  for (const auto* potential_descendant :
       background_attachment_fixed_objects_) {
    if (potential_descendant == &object)
      continue;
    if (potential_descendant->IsDescendantOf(&object))
      return true;
  }
  return false;
}

bool LocalFrameView::InvalidateViewportConstrainedObjects() {
  bool fast_path_allowed = true;
  for (auto* const viewport_constrained_object :
       *viewport_constrained_objects_) {
    LayoutObject* layout_object = viewport_constrained_object;
    DCHECK(layout_object->Style()->HasViewportConstrainedPosition() ||
           layout_object->Style()->HasStickyConstrainedPosition());
    DCHECK(layout_object->HasLayer());
    PaintLayer* layer = ToLayoutBoxModelObject(layout_object)->Layer();

    if (layer->IsPaintInvalidationContainer())
      continue;

    // If the layer has no visible content, then we shouldn't invalidate; but
    // if we're not compositing-inputs-clean, then we can't query
    // layer->SubtreeIsInvisible() here.
    layout_object->SetMayNeedPaintInvalidationSubtree();
    if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled() &&
        RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
        !layer->NeedsRepaint()) {
      // Paint properties of the layer relative to its containing graphics
      // layer may change if the paint properties escape the graphics layer's
      // property state. Need to check raster invalidation for relative paint
      // property changes.
      if (auto* paint_invalidation_layer =
              layer->EnclosingLayerForPaintInvalidation()) {
        auto* mapping = paint_invalidation_layer->GetCompositedLayerMapping();
        if (!mapping)
          mapping = paint_invalidation_layer->GroupedMapping();
        if (mapping)
          mapping->SetNeedsCheckRasterInvalidation();
      }
    }

    TRACE_EVENT_INSTANT1(
        TRACE_DISABLED_BY_DEFAULT("devtools.timeline.invalidationTracking"),
        "ScrollInvalidationTracking", TRACE_EVENT_SCOPE_THREAD, "data",
        InspectorScrollInvalidationTrackingEvent::Data(*layout_object));

    // If the fixed layer has a blur/drop-shadow filter applied on at least one
    // of its parents, we cannot scroll using the fast path, otherwise the
    // outsets of the filter will be moved around the page.
    if (layer->HasAncestorWithFilterThatMovesPixels())
      fast_path_allowed = false;
  }
  return fast_path_allowed;
}

bool LocalFrameView::ScrollContentsFastPath(const IntSize& scroll_delta) {
  if (!ContentsInCompositedLayer())
    return false;

  InvalidateBackgroundAttachmentFixedDescendants(*GetLayoutView());

  if (!viewport_constrained_objects_ ||
      viewport_constrained_objects_->IsEmpty()) {
    probe::didChangeViewport(frame_.Get());
    return true;
  }

  if (!InvalidateViewportConstrainedObjects())
    return false;

  probe::didChangeViewport(frame_.Get());
  return true;
}

void LocalFrameView::ScrollContentsSlowPath() {
  TRACE_EVENT0("blink", "LocalFrameView::scrollContentsSlowPath");
  // We need full invalidation during slow scrolling. For slimming paint, full
  // invalidation of the LayoutView is not enough. We also need to invalidate
  // all of the objects.
  // FIXME: Find out what are enough to invalidate in slow path scrolling.
  // crbug.com/451090#9.
  auto* layout_view = GetLayoutView();
  DCHECK(layout_view);
  if (ContentsInCompositedLayer()) {
    layout_view->Layer()->GetCompositedLayerMapping()->SetContentsNeedDisplay();
  } else {
    layout_view
        ->SetShouldDoFullPaintInvalidationIncludingNonCompositingDescendants();
  }

  if (ContentsInCompositedLayer()) {
    IntRect update_rect = VisibleContentRect();
    layout_view->InvalidatePaintRectangle(LayoutRect(update_rect));
  }
}

void LocalFrameView::RestoreScrollbar() {
  SetScrollbarsSuppressed(false);
}

bool LocalFrameView::RestoreScrollAnchor(
    const SerializedAnchor& serialized_anchor) {
  return false;
}

void LocalFrameView::ProcessUrlFragment(const KURL& url,
                                        UrlFragmentBehavior behavior) {
  // If our URL has no ref, then we have no place we need to jump to.
  // OTOH If CSS target was set previously, we want to set it to 0, recalc
  // and possibly paint invalidation because :target pseudo class may have been
  // set (see bug 11321).
  // Similarly for svg, if we had a previous svgView() then we need to reset
  // the initial view if we don't have a fragment.
  if (!url.HasFragmentIdentifier() && !frame_->GetDocument()->CssTarget() &&
      !frame_->GetDocument()->IsSVGDocument())
    return;

  UseCounter::Count(&GetFrame(), WebFeature::kScrollToFragmentRequested);
  // Try the raw fragment for HTML documents, but skip it for `svgView()`:
  String fragment_identifier = url.FragmentIdentifier();
  if (!frame_->GetDocument()->IsSVGDocument() &&
      ProcessUrlFragmentHelper(fragment_identifier, behavior)) {
    UseCounter::Count(&GetFrame(), WebFeature::kScrollToFragmentSucceedWithRaw);
    return;
  }

  // Try again after decoding the fragment.
  if (frame_->GetDocument()->Encoding().IsValid()) {
    DecodeURLResult decode_result;
    if (ProcessUrlFragmentHelper(
            DecodeURLEscapeSequences(fragment_identifier, &decode_result),
            behavior)) {
      switch (decode_result) {
        case DecodeURLResult::kAsciiOnly:
          UseCounter::Count(&GetFrame(),
                            WebFeature::kScrollToFragmentSucceedWithASCII);
          break;
        case DecodeURLResult::kUTF8:
          UseCounter::Count(&GetFrame(),
                            WebFeature::kScrollToFragmentSucceedWithUTF8);
          break;
        case DecodeURLResult::kIsomorphic:
          UseCounter::Count(&GetFrame(),
                            WebFeature::kScrollToFragmentSucceedWithIsomorphic);
          break;
        case DecodeURLResult::kMixed:
          UseCounter::Count(&GetFrame(),
                            WebFeature::kScrollToFragmentSucceedWithMixed);
          break;
      }
    } else {
      switch (decode_result) {
        case DecodeURLResult::kAsciiOnly:
          UseCounter::Count(&GetFrame(),
                            WebFeature::kScrollToFragmentFailWithASCII);
          break;
        case DecodeURLResult::kUTF8:
          UseCounter::Count(&GetFrame(),
                            WebFeature::kScrollToFragmentFailWithUTF8);
          break;
        case DecodeURLResult::kIsomorphic:
          UseCounter::Count(&GetFrame(),
                            WebFeature::kScrollToFragmentFailWithIsomorphic);
          break;
        case DecodeURLResult::kMixed:
          UseCounter::Count(&GetFrame(),
                            WebFeature::kScrollToFragmentFailWithMixed);
          break;
      }
    }
  } else {
    UseCounter::Count(&GetFrame(),
                      WebFeature::kScrollToFragmentFailWithInvalidEncoding);
  }
}

bool LocalFrameView::ProcessUrlFragmentHelper(const String& name,
                                              UrlFragmentBehavior behavior) {
  DCHECK(frame_->GetDocument());

  Element* anchor_node = frame_->GetDocument()->FindAnchor(name);

  // Setting to null will clear the current target.
  frame_->GetDocument()->SetCSSTarget(anchor_node);

  if (frame_->GetDocument()->IsSVGDocument()) {
    if (SVGSVGElement* svg =
            ToSVGSVGElementOrNull(frame_->GetDocument()->documentElement())) {
      svg->SetupInitialView(name, anchor_node);
      if (!anchor_node)
        return false;
    }
    // If this is not the top-level frame, then don't scroll to the
    // anchor position.
    if (!frame_->IsMainFrame())
      return false;
  }

  // Implement the rule that "" and "top" both mean top of page as in other
  // browsers.
  if (!anchor_node &&
      !(name.IsEmpty() || DeprecatedEqualIgnoringCase(name, "top")))
    return false;

  if (behavior == kUrlFragmentDontScroll)
    return true;

  if (!anchor_node) {
    fragment_anchor_ = frame_->GetDocument();
    needs_focus_on_fragment_ = false;
  } else {
    fragment_anchor_ = anchor_node;
    needs_focus_on_fragment_ = true;
  }

  // If rendering is blocked, we'll necessarily have a layout to kick off the
  // scroll and focus.
  if (frame_->GetDocument()->IsRenderingReady()) {
    frame_->GetDocument()->UpdateStyleAndLayoutTree();

    // If layout is needed, we will scroll in performPostLayoutTasks. Otherwise,
    // scroll and focus immediately.
    if (NeedsLayout())
      UpdateLayout();
    else
      ScrollAndFocusFragmentAnchor();
  }

  return true;
}

void LocalFrameView::ClearFragmentAnchor() {
  fragment_anchor_ = nullptr;
}

void LocalFrameView::DidUpdateElasticOverscroll() {
  Page* page = GetFrame().GetPage();
  if (!page)
    return;
  FloatSize elastic_overscroll = page->GetChromeClient().ElasticOverscroll();
  if (HorizontalScrollbar()) {
    float delta =
        elastic_overscroll.Width() - HorizontalScrollbar()->ElasticOverscroll();
    if (delta != 0) {
      HorizontalScrollbar()->SetElasticOverscroll(elastic_overscroll.Width());
      GetScrollAnimator().NotifyContentAreaScrolled(FloatSize(delta, 0),
                                                    kCompositorScroll);
      SetScrollbarNeedsPaintInvalidation(kHorizontalScrollbar);
    }
  }
  if (VerticalScrollbar()) {
    float delta =
        elastic_overscroll.Height() - VerticalScrollbar()->ElasticOverscroll();
    if (delta != 0) {
      VerticalScrollbar()->SetElasticOverscroll(elastic_overscroll.Height());
      GetScrollAnimator().NotifyContentAreaScrolled(FloatSize(0, delta),
                                                    kCompositorScroll);
      SetScrollbarNeedsPaintInvalidation(kVerticalScrollbar);
    }
  }
}

IntSize LocalFrameView::GetLayoutSize(
    IncludeScrollbarsInRect scrollbar_inclusion) const {
  return scrollbar_inclusion == kExcludeScrollbars
             ? ExcludeScrollbars(layout_size_)
             : layout_size_;
}

void LocalFrameView::SetLayoutSize(const IntSize& size) {
  DCHECK(!LayoutSizeFixedToFrameSize());
  if (frame_->GetDocument() &&
      frame_->GetDocument()->Lifecycle().LifecyclePostponed())
    return;

  SetLayoutSizeInternal(size);
}

void LocalFrameView::SetLayoutSizeFixedToFrameSize(bool is_fixed) {
  if (layout_size_fixed_to_frame_size_ == is_fixed)
    return;

  layout_size_fixed_to_frame_size_ = is_fixed;
  if (is_fixed)
    SetLayoutSizeInternal(Size());
}

void LocalFrameView::DidScrollTimerFired(TimerBase*) {
  if (frame_->GetDocument() && frame_->GetDocument()->GetLayoutView())
    frame_->GetDocument()->Fetcher()->UpdateAllImageResourcePriorities();

  GetFrame().Loader().SaveScrollAnchor();
}

void LocalFrameView::UpdateLayersAndCompositingAfterScrollIfNeeded() {
  // Nothing to do after scrolling if there are no fixed position elements.
  if (!HasViewportConstrainedObjects())
    return;

  // Update sticky position objects which are stuck to the viewport. In order to
  // correctly compute the sticky position offsets the layers must be visited
  // top-down, so start at the 'root' sticky elements and recurse downwards.
  for (auto* const viewport_constrained_object :
       *viewport_constrained_objects_) {
    LayoutObject* layout_object = viewport_constrained_object;
    if (layout_object->Style()->GetPosition() != EPosition::kSticky)
      continue;

    PaintLayer* layer = ToLayoutBoxModelObject(layout_object)->Layer();

    // This method can be called during layout at which point the ancestor
    // overflow layer may not be set yet. We can safely skip such cases as we
    // will revisit this method during compositing inputs update.
    if (!layer->AncestorOverflowLayer())
      continue;

    const StickyConstraintsMap& constraints_map =
        layer->AncestorOverflowLayer()
            ->GetScrollableArea()
            ->GetStickyConstraintsMap();
    if (constraints_map.Contains(layer) &&
        !constraints_map.at(layer).HasAncestorStickyElement()) {
      // TODO(skobes): Resolve circular dependency between scroll offset and
      // compositing state, and remove this disabler. https://crbug.com/420741
      DisableCompositingQueryAsserts disabler;
      layer->UpdateLayerPositionsAfterOverflowScroll();
      layout_object->SetMayNeedPaintInvalidationSubtree();
      SetNeedsUpdateGeometries();
    }
  }

  // If there fixed position elements, scrolling may cause compositing layers to
  // change.  Update LocalFrameView and layer positions after scrolling, but
  // only if we're not inside of layout.
  if (!nested_layout_count_) {
    UpdateGeometriesIfNeeded();
    if (auto* layout_view = this->GetLayoutView())
      layout_view->Layer()->SetNeedsCompositingInputsUpdate();
  }
}

static CompositedSelection ComputeCompositedSelection(LocalFrame& frame) {
  if (!frame.View() || frame.View()->ShouldThrottleRendering())
    return {};

  return ComputeCompositedSelection(frame.Selection());
}

void LocalFrameView::UpdateCompositedSelectionIfNeeded() {
  if (!RuntimeEnabledFeatures::CompositedSelectionUpdateEnabled())
    return;

  TRACE_EVENT0("blink", "LocalFrameView::updateCompositedSelectionIfNeeded");

  Page* page = GetFrame().GetPage();
  DCHECK(page);

  LocalFrame* focused_frame = page->GetFocusController().FocusedFrame();
  LocalFrame* local_frame =
      (focused_frame &&
       (focused_frame->LocalFrameRoot() == frame_->LocalFrameRoot()))
          ? focused_frame
          : nullptr;

  if (local_frame) {
    const CompositedSelection& selection =
        ComputeCompositedSelection(*local_frame);
    if (selection.type != kNoSelection) {
      page->GetChromeClient().UpdateCompositedSelection(local_frame, selection);
      return;
    }
  }

  if (!local_frame) {
    // Clearing the mainframe when there is no focused frame (and hence
    // no localFrame) is legacy behaviour, and implemented here to
    // satisfy WebFrameTest.CompositedSelectionBoundsCleared's
    // first check that the composited selection has been cleared even
    // though no frame has focus yet. If this is not desired, then the
    // expectation needs to be removed from the test.
    local_frame = &frame_->LocalFrameRoot();
  }
  DCHECK(local_frame);
  page->GetChromeClient().ClearCompositedSelection(local_frame);
}

void LocalFrameView::SetNeedsCompositingUpdate(
    CompositingUpdateType update_type) {
  if (auto* layout_view = GetLayoutView()) {
    if (frame_->GetDocument()->IsActive())
      layout_view->Compositor()->SetNeedsCompositingUpdate(update_type);
  }
}

ChromeClient* LocalFrameView::GetChromeClient() const {
  Page* page = GetFrame().GetPage();
  if (!page)
    return nullptr;
  return &page->GetChromeClient();
}

SmoothScrollSequencer* LocalFrameView::GetSmoothScrollSequencer() const {
  Page* page = GetFrame().GetPage();
  if (!page)
    return nullptr;
  return page->GetSmoothScrollSequencer();
}

void LocalFrameView::ContentsResized() {
  if (frame_->IsMainFrame() && frame_->GetDocument()) {
    if (TextAutosizer* text_autosizer =
            frame_->GetDocument()->GetTextAutosizer())
      text_autosizer->UpdatePageInfoInAllFrames();
  }

  ScrollableArea::ContentsResized();
  SetNeedsLayout();
}

void LocalFrameView::ScrollbarExistenceMaybeChanged() {
  // We check to make sure the view is attached to a frame() as this method can
  // be triggered before the view is attached by LocalFrame::createView(...)
  // setting various values such as setScrollBarModes(...) for example.  An
  // ASSERT is triggered when a view is layout before being attached to a
  // frame().
  if (!GetFrame().View())
    return;

  Element* custom_scrollbar_element = nullptr;

  bool uses_overlay_scrollbars =
      GetPageScrollbarTheme().UsesOverlayScrollbars() &&
      !ShouldUseCustomScrollbars(custom_scrollbar_element);

  if (!uses_overlay_scrollbars && NeedsLayout())
    UpdateLayout();
}

void LocalFrameView::HandleLoadCompleted() {
  // Once loading has completed, allow autoSize one last opportunity to
  // reduce the size of the frame.
  if (auto_size_info_)
    auto_size_info_->AutoSizeIfNeeded();

  // If there is a pending layout, the fragment anchor will be cleared when it
  // finishes.
  if (!NeedsLayout())
    ClearFragmentAnchor();
}

void LocalFrameView::ClearLayoutSubtreeRoot(const LayoutObject& root) {
  layout_subtree_root_list_.Remove(const_cast<LayoutObject&>(root));
}

void LocalFrameView::ClearLayoutSubtreeRootsAndMarkContainingBlocks() {
  layout_subtree_root_list_.ClearAndMarkContainingBlocksForLayout();
}

void LocalFrameView::AddOrthogonalWritingModeRoot(LayoutBox& root) {
  DCHECK(!root.IsLayoutScrollbarPart());
  orthogonal_writing_mode_root_list_.Add(root);
}

void LocalFrameView::RemoveOrthogonalWritingModeRoot(LayoutBox& root) {
  orthogonal_writing_mode_root_list_.Remove(root);
}

bool LocalFrameView::HasOrthogonalWritingModeRoots() const {
  return !orthogonal_writing_mode_root_list_.IsEmpty();
}

static inline void RemoveFloatingObjectsForSubtreeRoot(LayoutObject& root) {
  // TODO(kojii): Under certain conditions, moveChildTo() defers
  // removeFloatingObjects() until the containing block layouts. For
  // instance, when descendants of the moving child is floating,
  // removeChildNode() does not clear them. In such cases, at this
  // point, FloatingObjects may contain old or even deleted objects.
  // Dealing this in markAllDescendantsWithFloatsForLayout() could
  // solve, but since that is likely to suffer the performance and
  // since the containing block of orthogonal writing mode roots
  // having floats is very rare, prefer to re-create
  // FloatingObjects.
  if (LayoutBlock* cb = root.ContainingBlock()) {
    if ((cb->NormalChildNeedsLayout() || cb->SelfNeedsLayout()) &&
        cb->IsLayoutBlockFlow()) {
      ToLayoutBlockFlow(cb)->RemoveFloatingObjectsFromDescendants();
    }
  }
}

static bool PrepareOrthogonalWritingModeRootForLayout(LayoutObject& root) {
  DCHECK(root.IsBox() && ToLayoutBox(root).IsOrthogonalWritingModeRoot());
  if (!root.NeedsLayout() || root.IsOutOfFlowPositioned() ||
      root.IsColumnSpanAll() ||
      !root.StyleRef().LogicalHeight().IsIntrinsicOrAuto() ||
      ToLayoutBox(root).IsGridItem() || root.IsTablePart())
    return false;

  RemoveFloatingObjectsForSubtreeRoot(root);
  return true;
}

void LocalFrameView::LayoutOrthogonalWritingModeRoots() {
  for (auto& root : orthogonal_writing_mode_root_list_.Ordered()) {
    if (PrepareOrthogonalWritingModeRootForLayout(*root))
      LayoutFromRootObject(*root);
  }
}

void LocalFrameView::ScheduleOrthogonalWritingModeRootsForLayout() {
  for (auto& root : orthogonal_writing_mode_root_list_.Ordered()) {
    if (PrepareOrthogonalWritingModeRootForLayout(*root))
      layout_subtree_root_list_.Add(*root);
  }
}

bool LocalFrameView::CheckLayoutInvalidationIsAllowed() const {
  if (allows_layout_invalidation_after_layout_clean_)
    return true;

  // If we are updating all lifecycle phases beyond LayoutClean, we don't expect
  // dirty layout after LayoutClean.
  CHECK_FOR_DIRTY_LAYOUT(Lifecycle().GetState() <
                         DocumentLifecycle::kLayoutClean);

  return true;
}

void LocalFrameView::ScheduleRelayout() {
  DCHECK(frame_->View() == this);

  if (!layout_scheduling_enabled_)
    return;
  // TODO(crbug.com/590856): It's still broken when we choose not to crash when
  // the check fails.
  if (!CheckLayoutInvalidationIsAllowed())
    return;
  if (!NeedsLayout())
    return;
  if (!frame_->GetDocument()->ShouldScheduleLayout())
    return;
  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"),
                       "InvalidateLayout", TRACE_EVENT_SCOPE_THREAD, "data",
                       InspectorInvalidateLayoutEvent::Data(frame_.Get()));

  ClearLayoutSubtreeRootsAndMarkContainingBlocks();

  if (has_pending_layout_)
    return;
  has_pending_layout_ = true;

  if (!ShouldThrottleRendering())
    GetPage()->Animator().ScheduleVisualUpdate(frame_.Get());
}

void LocalFrameView::ScheduleRelayoutOfSubtree(LayoutObject* relayout_root) {
  DCHECK(frame_->View() == this);

  // TODO(crbug.com/590856): It's still broken when we choose not to crash when
  // the check fails.
  if (!CheckLayoutInvalidationIsAllowed())
    return;

  // FIXME: Should this call shouldScheduleLayout instead?
  if (!frame_->GetDocument()->IsActive())
    return;

  LayoutView* layout_view = this->GetLayoutView();
  if (layout_view && layout_view->NeedsLayout()) {
    if (relayout_root)
      relayout_root->MarkContainerChainForLayout(false);
    return;
  }

  if (relayout_root == layout_view)
    layout_subtree_root_list_.ClearAndMarkContainingBlocksForLayout();
  else
    layout_subtree_root_list_.Add(*relayout_root);

  if (layout_scheduling_enabled_) {
    has_pending_layout_ = true;

    if (!ShouldThrottleRendering())
      GetPage()->Animator().ScheduleVisualUpdate(frame_.Get());

    Lifecycle().EnsureStateAtMost(DocumentLifecycle::kStyleClean);
  }
  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"),
                       "InvalidateLayout", TRACE_EVENT_SCOPE_THREAD, "data",
                       InspectorInvalidateLayoutEvent::Data(frame_.Get()));
}

bool LocalFrameView::LayoutPending() const {
  // FIXME: This should check Document::lifecycle instead.
  return has_pending_layout_;
}

bool LocalFrameView::IsInPerformLayout() const {
  return Lifecycle().GetState() == DocumentLifecycle::kInPerformLayout;
}

bool LocalFrameView::NeedsLayout() const {
  // This can return true in cases where the document does not have a body yet.
  // Document::shouldScheduleLayout takes care of preventing us from scheduling
  // layout in that case.

  auto* layout_view = GetLayoutView();
  return LayoutPending() || (layout_view && layout_view->NeedsLayout()) ||
         IsSubtreeLayout();
}

NOINLINE bool LocalFrameView::CheckDoesNotNeedLayout() const {
  CHECK_FOR_DIRTY_LAYOUT(!LayoutPending());
  CHECK_FOR_DIRTY_LAYOUT(!GetLayoutView() || !GetLayoutView()->NeedsLayout());
  CHECK_FOR_DIRTY_LAYOUT(!IsSubtreeLayout());
  return true;
}

void LocalFrameView::SetNeedsLayout() {
  auto* layout_view = GetLayoutView();
  if (!layout_view)
    return;
  // TODO(crbug.com/590856): It's still broken if we choose not to crash when
  // the check fails.
  if (!CheckLayoutInvalidationIsAllowed())
    return;
  layout_view->SetNeedsLayout(LayoutInvalidationReason::kUnknown);
}

bool LocalFrameView::HasOpaqueBackground() const {
  return !base_background_color_.HasAlpha();
}

Color LocalFrameView::BaseBackgroundColor() const {
  return base_background_color_;
}

void LocalFrameView::SetBaseBackgroundColor(const Color& background_color) {
  if (base_background_color_ == background_color)
    return;

  base_background_color_ = background_color;

  if (auto* layout_view = GetLayoutView()) {
    if (layout_view->Layer()->HasCompositedLayerMapping()) {
      CompositedLayerMapping* composited_layer_mapping =
          layout_view->Layer()->GetCompositedLayerMapping();
      composited_layer_mapping->UpdateContentsOpaque();
      if (composited_layer_mapping->MainGraphicsLayer())
        composited_layer_mapping->MainGraphicsLayer()->SetNeedsDisplay();
      if (composited_layer_mapping->ScrollingContentsLayer())
        composited_layer_mapping->ScrollingContentsLayer()->SetNeedsDisplay();
    }
  }
  RecalculateScrollbarOverlayColorTheme(DocumentBackgroundColor());

  if (!ShouldThrottleRendering())
    GetPage()->Animator().ScheduleVisualUpdate(frame_.Get());
}

void LocalFrameView::UpdateBaseBackgroundColorRecursively(
    const Color& base_background_color) {
  ForAllNonThrottledLocalFrameViews(
      [base_background_color](LocalFrameView& frame_view) {
        frame_view.SetBaseBackgroundColor(base_background_color);
      });
}

void LocalFrameView::ScrollAndFocusFragmentAnchor() {
  Node* anchor_node = fragment_anchor_;
  if (!anchor_node)
    return;

  // Scrolling is disabled during updateScrollbars (see
  // isProgrammaticallyScrollable).  Bail now to avoid clearing m_fragmentAnchor
  // before we actually have a chance to scroll.
  if (in_update_scrollbars_)
    return;

  if (anchor_node->GetLayoutObject()) {
    LayoutRect rect;
    if (anchor_node != frame_->GetDocument()) {
      rect = anchor_node->BoundingBoxForScrollIntoView();
    } else if (RuntimeEnabledFeatures::RootLayerScrollingEnabled()) {
      if (Element* document_element = frame_->GetDocument()->documentElement())
        rect = document_element->BoundingBoxForScrollIntoView();
    }

    Frame* boundary_frame = frame_->FindUnsafeParentScrollPropagationBoundary();

    // FIXME: Handle RemoteFrames
    if (boundary_frame && boundary_frame->IsLocalFrame()) {
      ToLocalFrame(boundary_frame)
          ->View()
          ->SetSafeToPropagateScrollToParent(false);
    }

    // Scroll nested layers and frames to reveal the anchor.
    // Align to the top and to the closest side (this matches other browsers).
    anchor_node->GetLayoutObject()->ScrollRectToVisible(
        rect, WebScrollIntoViewParams(ScrollAlignment::kAlignToEdgeIfNeeded,
                                      ScrollAlignment::kAlignTopAlways));

    if (boundary_frame && boundary_frame->IsLocalFrame()) {
      ToLocalFrame(boundary_frame)
          ->View()
          ->SetSafeToPropagateScrollToParent(true);
    }

    if (AXObjectCache* cache = frame_->GetDocument()->ExistingAXObjectCache())
      cache->HandleScrolledToAnchor(anchor_node);

    // If the anchor accepts keyboard focus and fragment scrolling is allowed,
    // move focus there to aid users relying on keyboard navigation.
    // If anchorNode is not focusable or fragment scrolling is not allowed,
    // clear focus, which matches the behavior of other browsers.
    if (needs_focus_on_fragment_) {
      if (anchor_node->IsElementNode() &&
          ToElement(anchor_node)->IsFocusable()) {
        ToElement(anchor_node)->focus();
      } else {
        frame_->GetDocument()->SetSequentialFocusNavigationStartingPoint(
            anchor_node);
        frame_->GetDocument()->ClearFocusedElement();
      }
      needs_focus_on_fragment_ = false;
    }
  }

  // The fragment anchor should only be maintained while the frame is still
  // loading.  If the frame is done loading, clear the anchor now. Otherwise,
  // restore it since it may have been cleared during scrollRectToVisible.
  fragment_anchor_ =
      frame_->GetDocument()->IsLoadCompleted() ? nullptr : anchor_node;
}

bool LocalFrameView::UpdatePlugins() {
  // This is always called from UpdatePluginsTimerFired.
  // update_plugins_timer should only be scheduled if we have FrameViews to
  // update. Thus I believe we can stop checking isEmpty here, and just ASSERT
  // isEmpty:
  // FIXME: This assert has been temporarily removed due to
  // https://crbug.com/430344
  if (nested_layout_count_ > 1 || part_update_set_.IsEmpty())
    return true;

  // Need to swap because script will run inside the below loop and invalidate
  // the iterator.
  EmbeddedObjectSet objects;
  objects.swap(part_update_set_);

  for (const auto& embedded_object : objects) {
    LayoutEmbeddedObject& object = *embedded_object;
    HTMLPlugInElement* element = ToHTMLPlugInElement(object.GetNode());

    // The object may have already been destroyed (thus node cleared),
    // but LocalFrameView holds a manual ref, so it won't have been deleted.
    if (!element)
      continue;

    // No need to update if it's already crashed or known to be missing.
    if (object.ShowsUnavailablePluginIndicator())
      continue;

    if (element->NeedsPluginUpdate())
      element->UpdatePlugin();
    if (EmbeddedContentView* view = element->OwnedEmbeddedContentView())
      view->UpdateGeometry();

    // Prevent plugins from causing infinite updates of themselves.
    // FIXME: Do we really need to prevent this?
    part_update_set_.erase(&object);
  }

  return part_update_set_.IsEmpty();
}

void LocalFrameView::UpdatePluginsTimerFired(TimerBase*) {
  DCHECK(!IsInPerformLayout());
  for (unsigned i = 0; i < kMaxUpdatePluginsIterations; ++i) {
    if (UpdatePlugins())
      return;
  }
}

void LocalFrameView::FlushAnyPendingPostLayoutTasks() {
  DCHECK(!IsInPerformLayout());
  if (post_layout_tasks_timer_.IsActive())
    PerformPostLayoutTasks();
  if (update_plugins_timer_.IsActive()) {
    update_plugins_timer_.Stop();
    UpdatePluginsTimerFired(nullptr);
  }
}

void LocalFrameView::ScheduleUpdatePluginsIfNecessary() {
  DCHECK(!IsInPerformLayout());
  if (update_plugins_timer_.IsActive() || part_update_set_.IsEmpty())
    return;
  update_plugins_timer_.StartOneShot(TimeDelta(), FROM_HERE);
}

void LocalFrameView::PerformPostLayoutTasks() {
  // FIXME: We can reach here, even when the page is not active!
  // http/tests/inspector/elements/html-link-import.html and many other
  // tests hit that case.
  // We should DCHECK(isActive()); or at least return early if we can!

  // Always called before or after performLayout(), part of the highest-level
  // layout() call.
  DCHECK(!IsInPerformLayout());
  TRACE_EVENT0("blink,benchmark", "LocalFrameView::performPostLayoutTasks");

  post_layout_tasks_timer_.Stop();

  frame_->Selection().DidLayout();

  DCHECK(frame_->GetDocument());

  FontFaceSetDocument::DidLayout(*frame_->GetDocument());
  // Fire a fake a mouse move event to update hover state and mouse cursor, and
  // send the right mouse out/over events.
  frame_->GetEventHandler().DispatchFakeMouseMoveEventSoon(
      MouseEventManager::FakeMouseMoveReason::kPerFrame);

  UpdateGeometriesIfNeeded();

  // Plugins could have torn down the page inside updateGeometries().
  if (!GetLayoutView())
    return;

  ScheduleUpdatePluginsIfNecessary();

  if (ScrollingCoordinator* scrolling_coordinator =
          this->GetScrollingCoordinator()) {
    scrolling_coordinator->NotifyGeometryChanged(this);
  }

  if (SnapCoordinator* snap_coordinator =
          frame_->GetDocument()->GetSnapCoordinator())
    snap_coordinator->UpdateAllSnapContainerData();

  SendResizeEventIfNeeded();
}

bool LocalFrameView::WasViewportResized() {
  DCHECK(frame_);
  auto* layout_view = GetLayoutView();
  if (!layout_view)
    return false;
  return (GetLayoutSize(kIncludeScrollbars) != last_viewport_size_ ||
          layout_view->StyleRef().Zoom() != last_zoom_factor_);
}

void LocalFrameView::SendResizeEventIfNeeded() {
  DCHECK(frame_);

  auto* layout_view = GetLayoutView();
  if (!layout_view || layout_view->GetDocument().Printing())
    return;

  if (!WasViewportResized())
    return;

  last_viewport_size_ = GetLayoutSize(kIncludeScrollbars);
  last_zoom_factor_ = layout_view->StyleRef().Zoom();

  if (RuntimeEnabledFeatures::VisualViewportAPIEnabled())
    frame_->GetDocument()->EnqueueVisualViewportResizeEvent();

  frame_->GetDocument()->EnqueueResizeEvent();

  if (frame_->IsMainFrame())
    probe::didResizeMainFrame(frame_.Get());
}

void LocalFrameView::PostLayoutTimerFired(TimerBase*) {
  PerformPostLayoutTasks();
}

bool LocalFrameView::ShouldUseIntegerScrollOffset() const {
  if (frame_->GetSettings() &&
      !frame_->GetSettings()->GetPreferCompositingToLCDTextEnabled())
    return true;

  return ScrollableArea::ShouldUseIntegerScrollOffset();
}

bool LocalFrameView::IsActive() const {
  Page* page = GetFrame().GetPage();
  return page && page->GetFocusController().IsActive();
}

void LocalFrameView::InvalidatePaintForTickmarks() {
  ScrollableArea* scrollable_area = LayoutViewportScrollableArea();
  if (!scrollable_area)
    return;
  if (Scrollbar* scrollbar = scrollable_area->VerticalScrollbar()) {
    scrollbar->SetNeedsPaintInvalidation(
        static_cast<ScrollbarPart>(~kThumbPart));
  }
}

void LocalFrameView::GetTickmarks(Vector<IntRect>& tickmarks) const {
  if (!tickmarks_.IsEmpty()) {
    tickmarks = tickmarks_;
    return;
  }
  tickmarks =
      GetFrame().GetDocument()->Markers().LayoutRectsForTextMatchMarkers();
}

void LocalFrameView::SetInputEventsScaleForEmulation(
    float content_scale_factor) {
  input_events_scale_factor_for_emulation_ = content_scale_factor;
}

float LocalFrameView::InputEventsScaleFactor() const {
  float page_scale = frame_->GetPage()->GetVisualViewport().Scale();
  return page_scale * input_events_scale_factor_for_emulation_;
}

bool LocalFrameView::ScrollbarsCanBeActive() const {
  if (frame_->View() != this)
    return false;

  return !!frame_->GetDocument();
}

void LocalFrameView::ScrollbarVisibilityChanged() {
  UpdateScrollbarEnabledState();
  if (auto* layout_view = GetLayoutView())
    layout_view->ClearHitTestCache();
}

void LocalFrameView::ScrollbarFrameRectChanged() {
  SetNeedsPaintPropertyUpdate();
}

IntRect LocalFrameView::ScrollableAreaBoundingBox() const {
  auto* owner_layout_object = GetFrame().OwnerLayoutObject();
  if (!owner_layout_object)
    return FrameRect();

  LocalFrameView* local_root = GetFrame().LocalFrameRoot().View();
  return local_root->RootFrameToDocument(local_root->AbsoluteToRootFrame(
      owner_layout_object->AbsoluteContentQuad(kTraverseDocumentBoundaries)
          .EnclosingBoundingBox()));
}

bool LocalFrameView::IsScrollable() const {
  return GetScrollingReasons() == kScrollable;
}

bool LocalFrameView::IsProgrammaticallyScrollable() {
  return !in_update_scrollbars_;
}

LocalFrameView::ScrollingReasons LocalFrameView::GetScrollingReasons() const {
  // Check for:
  // 1) If there an actual overflow.
  // 2) display:none or visibility:hidden set to self or inherited.
  // 3) overflow{-x,-y}: hidden;
  // 4) scrolling: no;

  // Covers #1
  IntSize contents_size;
  if (GetLayoutView())
    contents_size = GetLayoutView()->DocumentRect().Size();
  else
    contents_size = ContentsSize();

  IntSize visible_content_size = VisibleContentSize();
  if (contents_size.Height() <= visible_content_size.Height() &&
      contents_size.Width() <= visible_content_size.Width())
    return kNotScrollableNoOverflow;

  // Covers #2.
  // FIXME: Do we need to fix this for OOPI?
  HTMLFrameOwnerElement* owner = frame_->DeprecatedLocalOwner();
  if (owner && (!owner->GetLayoutObject() ||
                !owner->GetLayoutObject()->VisibleToHitTesting()))
    return kNotScrollableNotVisible;

  // Cover #3 and #4.
  ScrollbarMode horizontal_mode;
  ScrollbarMode vertical_mode;
  GetLayoutView()->CalculateScrollbarModes(horizontal_mode, vertical_mode);
  if (horizontal_mode == kScrollbarAlwaysOff &&
      vertical_mode == kScrollbarAlwaysOff)
    return kNotScrollableExplicitlyDisabled;

  return kScrollable;
}

void LocalFrameView::UpdateParentScrollableAreaSet() {
  if (RuntimeEnabledFeatures::RootLayerScrollingEnabled())
    return;

  // That ensures that only inner frames are cached.
  LocalFrameView* parent_frame_view = ParentFrameView();
  if (!parent_frame_view)
    return;

  if (!IsScrollable()) {
    parent_frame_view->RemoveScrollableArea(this);
    return;
  }

  parent_frame_view->AddScrollableArea(this);
}

bool LocalFrameView::ShouldSuspendScrollAnimations() const {
  return !frame_->GetDocument()->LoadEventFinished();
}

void LocalFrameView::ScrollbarStyleChanged() {
  // FIXME: Why does this only apply to the main frame?
  if (!frame_->IsMainFrame())
    return;
  AdjustScrollbarOpacity();
  ContentsResized();
  UpdateScrollbars();
  PositionScrollbarLayers();
}

bool LocalFrameView::ScheduleAnimation() {
  if (ChromeClient* client = GetChromeClient()) {
    client->ScheduleAnimation(this);
    return true;
  }
  return false;
}

void LocalFrameView::NotifyPageThatContentAreaWillPaint() const {
  Page* page = frame_->GetPage();
  if (!page)
    return;

  ContentAreaWillPaint();

  if (!scrollable_areas_)
    return;

  for (const auto& scrollable_area : *scrollable_areas_) {
    if (!scrollable_area->ScrollbarsCanBeActive())
      continue;

    scrollable_area->ContentAreaWillPaint();
  }
}

CompositorElementId LocalFrameView::GetCompositorElementId() const {
  if (!RuntimeEnabledFeatures::RootLayerScrollingEnabled()) {
    return CompositorElementIdFromUniqueObjectId(
        unique_id_, CompositorElementIdNamespace::kScroll);
  } else {
    return PaintInvalidationCapableScrollableArea::GetCompositorElementId();
  }
}

bool LocalFrameView::ScrollAnimatorEnabled() const {
  return frame_->GetSettings() &&
         frame_->GetSettings()->GetScrollAnimatorEnabled();
}

void LocalFrameView::UpdateDocumentAnnotatedRegions() const {
  Document* document = frame_->GetDocument();
  if (!document->HasAnnotatedRegions())
    return;
  Vector<AnnotatedRegionValue> new_regions;
  CollectAnnotatedRegions(*(document->GetLayoutBox()), new_regions);
  if (new_regions == document->AnnotatedRegions())
    return;
  document->SetAnnotatedRegions(new_regions);

  DCHECK(frame_->Client());
  frame_->Client()->AnnotatedRegionsChanged();
}

void LocalFrameView::DidAttachDocument() {
  Page* page = frame_->GetPage();
  DCHECK(page);

  DCHECK(frame_->GetDocument());

  if (frame_->IsMainFrame()) {
    ScrollableArea& visual_viewport = frame_->GetPage()->GetVisualViewport();
    ScrollableArea* layout_viewport = LayoutViewportScrollableArea();
    DCHECK(layout_viewport);

    RootFrameViewport* root_frame_viewport =
        RootFrameViewport::Create(visual_viewport, *layout_viewport);
    viewport_scrollable_area_ = root_frame_viewport;

    page->GlobalRootScrollerController().InitializeViewportScrollCallback(
        *root_frame_viewport);
  }
}

void LocalFrameView::UpdateScrollCorner() {
  scoped_refptr<ComputedStyle> corner_style;
  IntRect corner_rect = ScrollCornerRect();
  Document* doc = frame_->GetDocument();

  if (doc && !corner_rect.IsEmpty()) {
    // Try the <body> element first as a scroll corner source.
    if (Element* body = doc->body()) {
      if (LayoutObject* layout_object = body->GetLayoutObject()) {
        corner_style = layout_object->GetUncachedPseudoStyle(
            PseudoStyleRequest(kPseudoIdScrollbarCorner),
            layout_object->Style());
      }
    }

    if (!corner_style) {
      // If the <body> didn't have a custom style, then the root element might.
      if (Element* doc_element = doc->documentElement()) {
        if (LayoutObject* layout_object = doc_element->GetLayoutObject()) {
          corner_style = layout_object->GetUncachedPseudoStyle(
              PseudoStyleRequest(kPseudoIdScrollbarCorner),
              layout_object->Style());
        }
      }
    }

    if (!corner_style) {
      // If we have an owning ipage/LocalFrame element, then it can set the
      // custom scrollbar also.
      auto* layout_object = frame_->OwnerLayoutObject();
      if (layout_object) {
        corner_style = layout_object->GetUncachedPseudoStyle(
            PseudoStyleRequest(kPseudoIdScrollbarCorner),
            layout_object->Style());
      }
    }
  }

  if (corner_style) {
    if (!scroll_corner_)
      scroll_corner_ = LayoutScrollbarPart::CreateAnonymous(doc, this);
    scroll_corner_->SetStyleWithWritingModeOfParent(std::move(corner_style));
    SetScrollCornerNeedsPaintInvalidation();
  } else if (scroll_corner_) {
    scroll_corner_->Destroy();
    scroll_corner_ = nullptr;
  }
}

Color LocalFrameView::DocumentBackgroundColor() const {
  // The LayoutView's background color is set in
  // Document::inheritHtmlAndBodyElementStyles.  Blend this with the base
  // background color of the LocalFrameView. This should match the color drawn
  // by ViewPainter::paintBoxDecorationBackground.
  Color result = BaseBackgroundColor();
  auto* layout_view = GetLayoutView();
  if (layout_view) {
    result = result.Blend(
        layout_view->ResolveColor(GetCSSPropertyBackgroundColor()));
  }
  return result;
}

void LocalFrameView::WillBeRemovedFromFrame() {
  if (paint_artifact_compositor_)
    paint_artifact_compositor_->WillBeRemovedFromFrame();
}

LocalFrameView* LocalFrameView::ParentFrameView() const {
  if (!is_attached_)
    return nullptr;

  Frame* parent_frame = frame_->Tree().Parent();
  if (parent_frame && parent_frame->IsLocalFrame())
    return ToLocalFrame(parent_frame)->View();

  return nullptr;
}

void LocalFrameView::DidChangeGlobalRootScroller() {
  // Being the global root scroller will affect clipping size due to browser
  // controls behavior so we need to update compositing based on updated clip
  // geometry.
  SetNeedsCompositingUpdate(kCompositingUpdateAfterGeometryChange);
  SetNeedsPaintPropertyUpdate();

  // Avoid drawing two sets of scrollbars when visual viewport provides
  // scrollbars.
  if (frame_->GetSettings() && frame_->GetSettings()->GetViewportEnabled())
    VisualViewportScrollbarsChanged();
}

// TODO(pdr): This logic is similar to adjustScrollbarExistence and the common
// logic should be factored into a helper.
void LocalFrameView::VisualViewportScrollbarsChanged() {
  bool has_horizontal_scrollbar = HorizontalScrollbar();
  bool has_vertical_scrollbar = VerticalScrollbar();
  bool should_have_horizontal_scrollbar = false;
  bool should_have_vertical_scrollbar = false;
  ComputeScrollbarExistence(should_have_horizontal_scrollbar,
                            should_have_vertical_scrollbar, ContentsSize());
  scrollbar_manager_.SetHasHorizontalScrollbar(
      should_have_horizontal_scrollbar);
  scrollbar_manager_.SetHasVerticalScrollbar(should_have_vertical_scrollbar);

  if (has_horizontal_scrollbar != should_have_horizontal_scrollbar ||
      has_vertical_scrollbar != should_have_vertical_scrollbar) {
    ScrollbarExistenceMaybeChanged();

    if (!VisualViewportSuppliesScrollbars())
      UpdateScrollbarGeometry();
  }

  if (RuntimeEnabledFeatures::RootLayerScrollingEnabled()) {
    if (LayoutView* layout_view = GetLayoutView())
      layout_view->Layer()->ClearClipRects();
  }
}

void LocalFrameView::UpdateGeometriesIfNeeded() {
  if (!needs_update_geometries_)
    return;
  needs_update_geometries_ = false;
  HeapVector<Member<EmbeddedContentView>> views;
  ForAllChildViewsAndPlugins(
      [&](EmbeddedContentView& view) { views.push_back(view); });

  for (const auto& view : views) {
    // Script or plugins could detach the frame so abort processing if that
    // happens.
    if (!GetLayoutView())
      break;

    view->UpdateGeometry();
  }
}

void LocalFrameView::UpdateAllLifecyclePhases() {
  GetFrame().LocalFrameRoot().View()->UpdateLifecyclePhasesInternal(
      DocumentLifecycle::kPaintClean);
}

bool LocalFrameView::UpdateLifecycleToPrePaintClean() {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
    return UpdateAllLifecyclePhasesExceptPaint();
  } else {
    return GetFrame().LocalFrameRoot().View()->UpdateLifecyclePhasesInternal(
        DocumentLifecycle::kPrePaintClean);
  }
}

// TODO(chrishtr): add a scrolling update lifecycle phase.
bool LocalFrameView::UpdateLifecycleToCompositingCleanPlusScrolling() {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
    return UpdateAllLifecyclePhasesExceptPaint();
  } else {
    return GetFrame().LocalFrameRoot().View()->UpdateLifecyclePhasesInternal(
        DocumentLifecycle::kCompositingClean);
  }
}

bool LocalFrameView::UpdateLifecycleToCompositingInputsClean() {
  // When SPv2 is enabled, the standard compositing lifecycle steps do not
  // exist; compositing is done after paint instead.
  DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV2Enabled());
  return GetFrame().LocalFrameRoot().View()->UpdateLifecyclePhasesInternal(
      DocumentLifecycle::kCompositingInputsClean);
}

bool LocalFrameView::UpdateAllLifecyclePhasesExceptPaint() {
  return GetFrame().LocalFrameRoot().View()->UpdateLifecyclePhasesInternal(
      DocumentLifecycle::kPrePaintClean);
}

void LocalFrameView::UpdateLifecyclePhasesForPrinting() {
  auto* local_frame_view_root = GetFrame().LocalFrameRoot().View();
  local_frame_view_root->UpdateLifecyclePhasesInternal(
      DocumentLifecycle::kPrePaintClean);

  auto* detached_frame_view = this;
  while (detached_frame_view->is_attached_ &&
         detached_frame_view != local_frame_view_root)
    detached_frame_view = detached_frame_view->parent_.Get();

  if (detached_frame_view == local_frame_view_root)
    return;
  DCHECK(!detached_frame_view->is_attached_);

  // We are printing a detached frame or a descendant of a detached frame which
  // was not reached in some phases during during |local_frame_view_root->
  // UpdateLifecyclePhasesInternalnormal()|. We need the subtree to be ready for
  // painting.
  detached_frame_view->UpdateLifecyclePhasesInternal(
      DocumentLifecycle::kPrePaintClean);
}

bool LocalFrameView::UpdateLifecycleToLayoutClean() {
  return GetFrame().LocalFrameRoot().View()->UpdateLifecyclePhasesInternal(
      DocumentLifecycle::kLayoutClean);
}

void LocalFrameView::ScheduleVisualUpdateForPaintInvalidationIfNeeded() {
  LocalFrame& local_frame_root = GetFrame().LocalFrameRoot();
  if (local_frame_root.View()->current_update_lifecycle_phases_target_state_ <
          DocumentLifecycle::kPrePaintClean ||
      Lifecycle().GetState() >= DocumentLifecycle::kPrePaintClean) {
    // Schedule visual update to process the paint invalidation in the next
    // cycle.
    local_frame_root.ScheduleVisualUpdateUnlessThrottled();
  }
  // Otherwise the paint invalidation will be handled in the pre-paint
  // phase of this cycle.
}

void LocalFrameView::NotifyResizeObservers() {
  // Controller exists only if ResizeObserver was created.
  if (!GetFrame().GetDocument()->GetResizeObserverController())
    return;

  ResizeObserverController& resize_controller =
      frame_->GetDocument()->EnsureResizeObserverController();

  DCHECK(Lifecycle().GetState() >= DocumentLifecycle::kLayoutClean);

  size_t min_depth = 0;
  for (min_depth = resize_controller.GatherObservations(0);
       min_depth != ResizeObserverController::kDepthBottom;
       min_depth = resize_controller.GatherObservations(min_depth)) {
    resize_controller.DeliverObservations();
    GetFrame().GetDocument()->UpdateStyleAndLayout();
  }

  if (resize_controller.SkippedObservations()) {
    resize_controller.ClearObservations();
    ErrorEvent* error = ErrorEvent::Create(
        "ResizeObserver loop limit exceeded",
        SourceLocation::Capture(frame_->GetDocument()), nullptr);
    frame_->GetDocument()->DispatchErrorEvent(error, kNotSharableCrossOrigin);
    // Ensure notifications will get delivered in next cycle.
    if (LocalFrameView* frame_view = frame_->View())
      frame_view->ScheduleAnimation();
  }

  DCHECK(!GetLayoutView()->NeedsLayout());
}

void LocalFrameView::DispatchEventsForPrintingOnAllFrames() {
  DCHECK(frame_->IsMainFrame());
  for (Frame* current_frame = frame_; current_frame;
       current_frame = current_frame->Tree().TraverseNext(frame_)) {
    if (current_frame->IsLocalFrame())
      ToLocalFrame(current_frame)->GetDocument()->DispatchEventsForPrinting();
  }
}

void LocalFrameView::SetupPrintContext() {
  if (frame_->GetDocument()->Printing())
    return;
  if (!print_context_) {
    print_context_ = new PrintContext(frame_, /*use_printing_layout=*/true);
  }
  if (frame_->GetSettings())
    frame_->GetSettings()->SetShouldPrintBackgrounds(true);
  bool is_us = DefaultLanguage() == "en-US";
  int width = is_us ? kLetterPortraitPageWidth : kA4PortraitPageWidth;
  int height = is_us ? kLetterPortraitPageHeight : kA4PortraitPageHeight;
  print_context_->BeginPrintMode(width, height);
  print_context_->ComputePageRects(FloatSize(width, height));
  DispatchEventsForPrintingOnAllFrames();
}

void LocalFrameView::ClearPrintContext() {
  if (!print_context_)
    return;
  print_context_->EndPrintMode();
  print_context_.Clear();
}

// TODO(leviw): We don't assert lifecycle information from documents in child
// WebPluginContainerImpls.
bool LocalFrameView::UpdateLifecyclePhasesInternal(
    DocumentLifecycle::LifecycleState target_state) {
  if (frame_->GetDocument() &&
      frame_->GetDocument()->Lifecycle().LifecyclePostponed())
    return false;
  if (current_update_lifecycle_phases_target_state_ !=
      DocumentLifecycle::kUninitialized) {
    NOTREACHED()
        << "LocalFrameView::updateLifecyclePhasesInternal() reentrance";
    return false;
  }

  // This must be called from the root frame, or a detached frame for printing,
  // since it recurses down, not up. Otherwise the lifecycles of the frames
  // might be out of sync.
  DCHECK(frame_->IsLocalRoot() || !is_attached_);

  // Only the following target states are supported.
  DCHECK(target_state == DocumentLifecycle::kLayoutClean ||
         target_state == DocumentLifecycle::kCompositingInputsClean ||
         target_state == DocumentLifecycle::kCompositingClean ||
         target_state == DocumentLifecycle::kPrePaintClean ||
         target_state == DocumentLifecycle::kPaintClean);

  if (!frame_->GetDocument()->IsActive())
    return Lifecycle().GetState() == target_state;

  base::AutoReset<DocumentLifecycle::LifecycleState> target_state_scope(
      &current_update_lifecycle_phases_target_state_, target_state);

  if (ShouldThrottleRendering()) {
    UpdateViewportIntersectionsForSubtree(
        std::min(target_state, DocumentLifecycle::kCompositingClean));
    return Lifecycle().GetState() == target_state;
  }

  if (RuntimeEnabledFeatures::PrintBrowserEnabled())
    SetupPrintContext();
  else
    ClearPrintContext();

  UpdateStyleAndLayoutIfNeededRecursive();
  DCHECK(Lifecycle().GetState() >= DocumentLifecycle::kLayoutClean);

  frame_->GetDocument()
      ->GetRootScrollerController()
      .PerformRootScrollerSelection();

  // PerformRootScrollerSelection can dirty layout if an effective root
  // scroller is changed so make sure we get back to LayoutClean.
  if (RuntimeEnabledFeatures::ImplicitRootScrollerEnabled() ||
      RuntimeEnabledFeatures::SetRootScrollerEnabled()) {
    ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
      if (frame_view.NeedsLayout())
        frame_view.UpdateLayout();

      DCHECK(frame_view.Lifecycle().GetState() >=
             DocumentLifecycle::kLayoutClean);
    });
  }

  if (target_state == DocumentLifecycle::kLayoutClean) {
    UpdateViewportIntersectionsForSubtree(target_state);
    return Lifecycle().GetState() == target_state;
  }

  base::AutoReset<bool> past_layout_lifecycle_update(
      &past_layout_lifecycle_update_, true);

  // OOPIF local frame roots that are throttled can return now that layout
  // is clean and intersection observations can be calculated.
  if (ShouldThrottleRendering()) {
    UpdateViewportIntersectionsForSubtree(target_state);
    return Lifecycle().GetState() == target_state;
  }

  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.PerformScrollAnchoringAdjustments();
  });

  if (target_state == DocumentLifecycle::kPaintClean) {
    ForAllNonThrottledLocalFrameViews(
        [](LocalFrameView& frame_view) { frame_view.NotifyResizeObservers(); });

    if (RuntimeEnabledFeatures::RootLayerScrollingEnabled())
      NotifyFrameRectsChangedIfNeededRecursive();
  }

  if (auto* layout_view = GetLayoutView()) {
    ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
      frame_view.CheckDoesNotNeedLayout();
      frame_view.allows_layout_invalidation_after_layout_clean_ = false;
    });

    {
      TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"),
                           "SetLayerTreeId", TRACE_EVENT_SCOPE_THREAD, "data",
                           InspectorSetLayerTreeId::Data(frame_.Get()));
      TRACE_EVENT1("devtools.timeline", "UpdateLayerTree", "data",
                   InspectorUpdateLayerTreeEvent::Data(frame_.Get()));

      if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
        SCOPED_UMA_AND_UKM_TIMER("Blink.Compositing.UpdateTime",
                                 UkmMetricNames::kCompositing);
        layout_view->Compositor()->UpdateIfNeededRecursive(target_state);
      } else {
        ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
          frame_view.GetLayoutView()->Layer()->UpdateDescendantDependentFlags();
          frame_view.GetLayoutView()->CommitPendingSelection();
        });
      }

      if (target_state >= DocumentLifecycle::kCompositingClean) {
        if (!RuntimeEnabledFeatures::RootLayerScrollingEnabled())
          ScrollContentsIfNeededRecursive();

        frame_->GetPage()->GlobalRootScrollerController().DidUpdateCompositing(
            *this);
      }

      if (target_state >= DocumentLifecycle::kPrePaintClean) {
        if (!RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
          if (layout_view->Compositor()->InCompositingMode()) {
            GetScrollingCoordinator()->UpdateAfterCompositingChangeIfNeeded(
                this);
          }
        }

        UpdateCompositedSelectionIfNeeded();

        // TODO(pdr): prePaint should be under the "Paint" devtools timeline
        // step for slimming paint v2.
        PrePaint();
      }
    }

    if (target_state == DocumentLifecycle::kPaintClean) {
      // While printing a document, the paint walk is done by the printing
      // component into a special canvas. There is no point doing a normal paint
      // step (or animations update for BlinkGenPropertyTrees/SPv2) when in this
      // mode.
      //
      // RuntimeEnabledFeatures::PrintBrowserEnabled is a mode which runs the
      // browser normally, but renders every page as if it were being printed.
      // See crbug.com/667547
      bool print_mode_enabled = frame_->GetDocument()->Printing() &&
                                !RuntimeEnabledFeatures::PrintBrowserEnabled();
      if (!print_mode_enabled)
        PaintTree();

      if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled() ||
          RuntimeEnabledFeatures::BlinkGenPropertyTreesEnabled()) {
        if (!print_mode_enabled) {
          base::Optional<CompositorElementIdSet> composited_element_ids =
              CompositorElementIdSet();
          PushPaintArtifactToCompositor(composited_element_ids.value());
          // TODO(wkorman): Add call to UpdateCompositorScrollAnimations here.
          DocumentAnimations::UpdateAnimations(GetLayoutView()->GetDocument(),
                                               DocumentLifecycle::kPaintClean,
                                               composited_element_ids);
        }
        // Notify the controller that the artifact has been pushed and some
        // lifecycle state can be freed (such as raster invalidations).
        paint_controller_->FinishCycle();
      }

      DCHECK(!frame_->Selection().NeedsLayoutSelectionUpdate());
      DCHECK(ShouldThrottleRendering() ||
             (frame_->GetDocument()->Printing() &&
              Lifecycle().GetState() == DocumentLifecycle::kPrePaintClean) ||
             Lifecycle().GetState() == DocumentLifecycle::kPaintClean);
    }

    ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
      frame_view.CheckDoesNotNeedLayout();
      frame_view.allows_layout_invalidation_after_layout_clean_ = true;
    });
  }

  {
    TRACE_EVENT0("blink,benchmark",
                 "LocalFrameView::UpdateViewportIntersectionsForSubtree");
    SCOPED_UMA_AND_UKM_TIMER("Blink.IntersectionObservation.UpdateTime",
                             UkmMetricNames::kIntersectionObservation);
    UpdateViewportIntersectionsForSubtree(target_state);
  }

  return Lifecycle().GetState() == target_state;
}

void LocalFrameView::EnqueueScrollAnchoringAdjustment(
    ScrollableArea* scrollable_area) {
  anchoring_adjustment_queue_.insert(scrollable_area);
}

void LocalFrameView::DequeueScrollAnchoringAdjustment(
    ScrollableArea* scrollable_area) {
  anchoring_adjustment_queue_.erase(scrollable_area);
}

void LocalFrameView::PerformScrollAnchoringAdjustments() {
  // Adjust() will cause a scroll which could end up causing a layout and
  // reentering this method. Copy and clear the queue so we don't modify it
  // during iteration.
  AnchoringAdjustmentQueue queue_copy = anchoring_adjustment_queue_;
  anchoring_adjustment_queue_.clear();

  for (WeakMember<ScrollableArea>& scroller : queue_copy) {
    if (scroller) {
      DCHECK(scroller->GetScrollAnchor());
      scroller->GetScrollAnchor()->Adjust();
    }
  }
}

void LocalFrameView::PrePaint() {
  TRACE_EVENT0("blink,benchmark", "LocalFrameView::prePaint");

  if (!paint_controller_)
    paint_controller_ = PaintController::Create();

  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.Lifecycle().AdvanceTo(DocumentLifecycle::kInPrePaint);
    if (frame_view.CanThrottleRendering()) {
      // This frame can be throttled but not throttled, meaning we are not in an
      // AllowThrottlingScope. Now this frame may contain dirty paint flags, and
      // we need to propagate the flags into the ancestor chain so that
      // PrePaintTreeWalk can reach this frame.
      frame_view.SetNeedsPaintPropertyUpdate();
      if (auto* owner = frame_view.GetFrame().OwnerLayoutObject())
        owner->SetMayNeedPaintInvalidation();
    }
  });

  {
    SCOPED_UMA_AND_UKM_TIMER("Blink.PrePaint.UpdateTime",
                             UkmMetricNames::kPrePaint);
    PrePaintTreeWalk().WalkTree(*this);
  }

  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.Lifecycle().AdvanceTo(DocumentLifecycle::kPrePaintClean);
  });
}

static void CollectDrawableLayersForLayerListRecursively(
    GraphicsContext& context,
    const GraphicsLayer* layer) {
  DCHECK(RuntimeEnabledFeatures::BlinkGenPropertyTreesEnabled());

  if (!layer || layer->Client().ShouldThrottleRendering())
    return;

  scoped_refptr<cc::Layer> contents_layer = layer->ContentsLayer();
  if (layer->DrawsContent() || contents_layer) {
    ScopedPaintChunkProperties scope(context.GetPaintController(),
                                     layer->GetPropertyTreeState(), *layer,
                                     DisplayItem::kForeignLayerWrapper);

    if (layer->DrawsContent()) {
      // TODO(trchen): Currently the GraphicsLayer hierarchy is still built
      // during CompositingUpdate, and we have to clear them here to ensure no
      // extraneous layers are still attached. In future we will disable all
      // those layer hierarchy code so we won't need this line.
      layer->CcLayer()->RemoveAllChildren();
      RecordForeignLayer(context, *layer, DisplayItem::kForeignLayerWrapper,
                         layer->CcLayer(), layer->GetOffsetFromTransformNode(),
                         RoundedIntSize(layer->Size()));
    }
    if (contents_layer) {
      auto position = contents_layer->position();
      auto size = contents_layer->bounds();
      RecordForeignLayer(context, *layer,
                         DisplayItem::kForeignLayerContentsWrapper,
                         std::move(contents_layer),
                         layer->GetOffsetFromTransformNode() +
                             FloatSize(position.x(), position.y()),
                         IntSize(size.width(), size.height()));
    }
  }

  DCHECK(!layer->ContentsClippingMaskLayer());
  for (const auto* child : layer->Children())
    CollectDrawableLayersForLayerListRecursively(context, child);
  CollectDrawableLayersForLayerListRecursively(context, layer->MaskLayer());
}

void LocalFrameView::PaintTree() {
  TRACE_EVENT0("blink,benchmark", "LocalFrameView::paintTree");
  SCOPED_UMA_AND_UKM_TIMER("Blink.Paint.UpdateTime", UkmMetricNames::kPaint);

  DCHECK(GetFrame() == GetPage()->MainFrame() ||
         (!GetFrame().Tree().Parent()->IsLocalFrame()));

  auto* layout_view = GetLayoutView();
  DCHECK(layout_view);
  paint_frame_count_++;
  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.Lifecycle().AdvanceTo(DocumentLifecycle::kInPaint);
  });

  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
    if (GetLayoutView()->Layer()->NeedsRepaint()) {
      GraphicsContext graphics_context(*paint_controller_);
      if (RuntimeEnabledFeatures::PrintBrowserEnabled())
        graphics_context.SetPrinting(true);

      if (Settings* settings = frame_->GetSettings()) {
        HighContrastSettings high_contrast_settings;
        high_contrast_settings.mode = settings->GetHighContrastMode();
        high_contrast_settings.grayscale = settings->GetHighContrastGrayscale();
        high_contrast_settings.contrast = settings->GetHighContrastContrast();
        high_contrast_settings.image_policy =
            settings->GetHighContrastImagePolicy();
        graphics_context.SetHighContrast(high_contrast_settings);
      }

      PaintInternal(graphics_context, kGlobalPaintNormalPhase,
                    CullRect(LayoutRect::InfiniteIntRect()));
      paint_controller_->CommitNewDisplayItems();
    }
  } else {
    // A null graphics layer can occur for painting of SVG images that are not
    // parented into the main frame tree, or when the LocalFrameView is the main
    // frame view of a page overlay. The page overlay is in the layer tree of
    // the host page and will be painted during painting of the host page.
    if (GraphicsLayer* root_graphics_layer =
            layout_view->Compositor()->PaintRootGraphicsLayer())
      root_graphics_layer->PaintRecursively();

    // TODO(sataya.m):Main frame doesn't create RootFrameViewport in some
    // webkit_unit_tests (http://crbug.com/644788).
    if (viewport_scrollable_area_) {
      if (GraphicsLayer* layer_for_horizontal_scrollbar =
              viewport_scrollable_area_->LayerForHorizontalScrollbar())
        layer_for_horizontal_scrollbar->PaintRecursively();
      if (GraphicsLayer* layer_for_vertical_scrollbar =
              viewport_scrollable_area_->LayerForVerticalScrollbar())
        layer_for_vertical_scrollbar->PaintRecursively();
      if (GraphicsLayer* layer_for_scroll_corner =
              viewport_scrollable_area_->LayerForScrollCorner())
        layer_for_scroll_corner->PaintRecursively();
    }
  }

  if (RuntimeEnabledFeatures::BlinkGenPropertyTreesEnabled()) {
    GraphicsContext context(*paint_controller_);
    // With BlinkGenPropertyTrees, |PaintRootGraphicsLayer| is the ancestor of
    // all drawable layers (see: PaintLayerCompositor::PaintRootGraphicsLayer)
    // so we do not need to collect scrollbars separately.
    CollectDrawableLayersForLayerListRecursively(
        context, layout_view->Compositor()->PaintRootGraphicsLayer());
    paint_controller_->CommitNewDisplayItems();
  }

  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.Lifecycle().AdvanceTo(DocumentLifecycle::kPaintClean);
    if (auto* layout_view = frame_view.GetLayoutView())
      layout_view->Layer()->ClearNeedsRepaintRecursively();
  });
}

void LocalFrameView::PushPaintArtifactToCompositor(
    CompositorElementIdSet& composited_element_ids) {
  TRACE_EVENT0("blink", "LocalFrameView::pushPaintArtifactToCompositor");

  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV2Enabled() ||
         RuntimeEnabledFeatures::BlinkGenPropertyTreesEnabled());

  if (!frame_->GetSettings()->GetAcceleratedCompositingEnabled())
    return;

  Page* page = GetFrame().GetPage();
  if (!page)
    return;

  if (!paint_artifact_compositor_) {
    paint_artifact_compositor_ =
        PaintArtifactCompositor::Create(WTF::BindRepeating(
            &ScrollingCoordinator::DidScroll,
            // The layer being scrolled is destroyed before the
            // ScrollingCoordinator.
            WrapWeakPersistent(page->GetScrollingCoordinator())));
    page->GetChromeClient().AttachRootLayer(
        paint_artifact_compositor_->GetCcLayer(), &GetFrame());
  }

  SCOPED_UMA_AND_UKM_TIMER("Blink.Compositing.UpdateTime",
                           UkmMetricNames::kCompositing);

  paint_artifact_compositor_->Update(paint_controller_->GetPaintArtifact(),
                                     composited_element_ids);
}

std::unique_ptr<JSONObject> LocalFrameView::CompositedLayersAsJSON(
    LayerTreeFlags flags) {
  return GetFrame()
      .LocalFrameRoot()
      .View()
      ->paint_artifact_compositor_->LayersAsJSON(flags);
}

void LocalFrameView::UpdateStyleAndLayoutIfNeededRecursive() {
  SCOPED_UMA_AND_UKM_TIMER("Blink.StyleAndLayout.UpdateTime",
                           UkmMetricNames::kStyleAndLayout);
  UpdateStyleAndLayoutIfNeededRecursiveInternal();
}

void LocalFrameView::UpdateStyleAndLayoutIfNeededRecursiveInternal() {
  if (ShouldThrottleRendering() || !frame_->GetDocument()->IsActive())
    return;

  ScopedFrameBlamer frame_blamer(frame_);
  TRACE_EVENT0("blink,benchmark",
               "LocalFrameView::updateStyleAndLayoutIfNeededRecursive");

  // We have to crawl our entire subtree looking for any FrameViews that need
  // layout and make sure they are up to date.
  // Mac actually tests for intersection with the dirty region and tries not to
  // update layout for frames that are outside the dirty region.  Not only does
  // this seem pointless (since those frames will have set a zero timer to
  // layout anyway), but it is also incorrect, since if two frames overlap, the
  // first could be excluded from the dirty region but then become included
  // later by the second frame adding rects to the dirty region when it lays
  // out.

  frame_->GetDocument()->UpdateStyleAndLayoutTree();

  // Update style for all embedded SVG documents underneath this frame, so
  // that intrinsic size computation for any embedded objects has up-to-date
  // information before layout.
  ForAllChildLocalFrameViews([](LocalFrameView& view) {
    Document& document = *view.GetFrame().GetDocument();
    if (document.IsSVGDocument())
      document.UpdateStyleAndLayoutTree();
  });

  CHECK(!ShouldThrottleRendering());
  CHECK(frame_->GetDocument()->IsActive());
  CHECK(!nested_layout_count_);

  if (NeedsLayout())
    UpdateLayout();

  CheckDoesNotNeedLayout();

  // WebView plugins need to update regardless of whether the
  // LayoutEmbeddedObject that owns them needed layout.
  // TODO(leviw): This currently runs the entire lifecycle on plugin WebViews.
  // We should have a way to only run these other Documents to the same
  // lifecycle stage as this frame.
  for (const auto& plugin : plugins_) {
    plugin->UpdateAllLifecyclePhases();
  }
  CheckDoesNotNeedLayout();

  // FIXME: Calling layout() shouldn't trigger script execution or have any
  // observable effects on the frame tree but we're not quite there yet.
  HeapVector<Member<LocalFrameView>> frame_views;
  for (Frame* child = frame_->Tree().FirstChild(); child;
       child = child->Tree().NextSibling()) {
    if (!child->IsLocalFrame())
      continue;
    if (LocalFrameView* view = ToLocalFrame(child)->View())
      frame_views.push_back(view);
  }

  for (const auto& frame_view : frame_views)
    frame_view->UpdateStyleAndLayoutIfNeededRecursiveInternal();

  // These asserts ensure that parent frames are clean, when child frames
  // finished updating layout and style.
  CheckDoesNotNeedLayout();
#if DCHECK_IS_ON()
  frame_->GetDocument()->GetLayoutView()->AssertLaidOut();
#endif

  UpdateGeometriesIfNeeded();

  if (Lifecycle().GetState() < DocumentLifecycle::kLayoutClean)
    Lifecycle().AdvanceTo(DocumentLifecycle::kLayoutClean);

  // Ensure that we become visually non-empty eventually.
  // TODO(esprehn): This should check isRenderingReady() instead.
  if (GetFrame().GetDocument()->HasFinishedParsing() &&
      GetFrame().Loader().StateMachine()->CommittedFirstRealDocumentLoad())
    is_visually_non_empty_ = true;

  GetFrame().Selection().UpdateStyleAndLayoutIfNeeded();
  GetFrame().GetPage()->GetDragCaret().UpdateStyleAndLayoutIfNeeded();
}

void LocalFrameView::EnableAutoSizeMode(const IntSize& min_size,
                                        const IntSize& max_size) {
  if (!auto_size_info_)
    auto_size_info_ = FrameViewAutoSizeInfo::Create(this);

  auto_size_info_->ConfigureAutoSizeMode(min_size, max_size);
  SetLayoutSizeFixedToFrameSize(true);
  SetNeedsLayout();
  ScheduleRelayout();
}

void LocalFrameView::DisableAutoSizeMode() {
  if (!auto_size_info_)
    return;

  SetLayoutSizeFixedToFrameSize(false);
  SetNeedsLayout();
  ScheduleRelayout();

  // Since autosize mode forces the scrollbar mode, change them to being auto.
  SetAutosizeScrollbarModes(kScrollbarAuto, kScrollbarAuto);
  SetScrollbarModes(kScrollbarAuto, kScrollbarAuto);
  auto_size_info_.Clear();
}

void LocalFrameView::ForceLayoutForPagination(
    const FloatSize& page_size,
    const FloatSize& original_page_size,
    float maximum_shrink_factor) {
  // Dumping externalRepresentation(m_frame->layoutObject()).ascii() is a good
  // trick to see the state of things before and after the layout
  if (LayoutView* layout_view = this->GetLayoutView()) {
    float page_logical_width = layout_view->Style()->IsHorizontalWritingMode()
                                   ? page_size.Width()
                                   : page_size.Height();
    float page_logical_height = layout_view->Style()->IsHorizontalWritingMode()
                                    ? page_size.Height()
                                    : page_size.Width();

    LayoutUnit floored_page_logical_width =
        static_cast<LayoutUnit>(page_logical_width);
    LayoutUnit floored_page_logical_height =
        static_cast<LayoutUnit>(page_logical_height);
    layout_view->SetLogicalWidth(floored_page_logical_width);
    layout_view->SetPageLogicalHeight(floored_page_logical_height);
    layout_view->SetNeedsLayoutAndPrefWidthsRecalcAndFullPaintInvalidation(
        LayoutInvalidationReason::kPrintingChanged);
    UpdateLayout();

    // If we don't fit in the given page width, we'll lay out again. If we don't
    // fit in the page width when shrunk, we will lay out at maximum shrink and
    // clip extra content.
    // FIXME: We are assuming a shrink-to-fit printing implementation.  A
    // cropping implementation should not do this!
    bool horizontal_writing_mode =
        layout_view->Style()->IsHorizontalWritingMode();
    const LayoutRect& document_rect = LayoutRect(layout_view->DocumentRect());
    LayoutUnit doc_logical_width = horizontal_writing_mode
                                       ? document_rect.Width()
                                       : document_rect.Height();
    if (doc_logical_width > page_logical_width) {
      FloatSize expected_page_size(
          std::min<float>(document_rect.Width().ToFloat(),
                          page_size.Width() * maximum_shrink_factor),
          std::min<float>(document_rect.Height().ToFloat(),
                          page_size.Height() * maximum_shrink_factor));
      FloatSize max_page_size = frame_->ResizePageRectsKeepingRatio(
          FloatSize(original_page_size.Width(), original_page_size.Height()),
          expected_page_size);
      page_logical_width = horizontal_writing_mode ? max_page_size.Width()
                                                   : max_page_size.Height();
      page_logical_height = horizontal_writing_mode ? max_page_size.Height()
                                                    : max_page_size.Width();

      floored_page_logical_width = static_cast<LayoutUnit>(page_logical_width);
      floored_page_logical_height =
          static_cast<LayoutUnit>(page_logical_height);
      layout_view->SetLogicalWidth(floored_page_logical_width);
      layout_view->SetPageLogicalHeight(floored_page_logical_height);
      layout_view->SetNeedsLayoutAndPrefWidthsRecalcAndFullPaintInvalidation(
          LayoutInvalidationReason::kPrintingChanged);
      UpdateLayout();

      const LayoutRect& updated_document_rect =
          LayoutRect(layout_view->DocumentRect());
      LayoutUnit doc_logical_height = horizontal_writing_mode
                                          ? updated_document_rect.Height()
                                          : updated_document_rect.Width();
      LayoutUnit doc_logical_top = horizontal_writing_mode
                                       ? updated_document_rect.Y()
                                       : updated_document_rect.X();
      LayoutUnit doc_logical_right = horizontal_writing_mode
                                         ? updated_document_rect.MaxX()
                                         : updated_document_rect.MaxY();
      LayoutUnit clipped_logical_left;
      if (!layout_view->Style()->IsLeftToRightDirection()) {
        clipped_logical_left =
            LayoutUnit(doc_logical_right - page_logical_width);
      }
      LayoutRect overflow(clipped_logical_left, doc_logical_top,
                          LayoutUnit(page_logical_width), doc_logical_height);

      if (!horizontal_writing_mode)
        overflow = overflow.TransposedRect();
      AdjustViewSizeAndLayout();
      // This is how we clip in case we overflow again.
      layout_view->ClearLayoutOverflow();
      layout_view->AddLayoutOverflow(overflow);
      return;
    }
  }

  if (TextAutosizer* text_autosizer = frame_->GetDocument()->GetTextAutosizer())
    text_autosizer->UpdatePageInfo();
  AdjustViewSizeAndLayout();
}

IntRect LocalFrameView::ConvertFromLayoutObject(
    const LayoutObject& layout_object,
    const IntRect& layout_object_rect) const {
  // Convert from page ("absolute") to LocalFrameView coordinates.
  LayoutRect rect = EnclosingLayoutRect(
      layout_object.LocalToAbsoluteQuad(FloatRect(layout_object_rect))
          .BoundingBox());
  rect.Move(LayoutSize(-GetScrollOffset()));
  return PixelSnappedIntRect(rect);
}

IntRect LocalFrameView::ConvertToLayoutObject(const LayoutObject& layout_object,
                                              const IntRect& frame_rect) const {
  IntRect rect_in_content = FrameToContents(frame_rect);

  // Convert from LocalFrameView coords into page ("absolute") coordinates.
  rect_in_content.Move(ScrollOffsetInt());

  // FIXME: we don't have a way to map an absolute rect down to a local quad, so
  // just move the rect for now.
  rect_in_content.SetLocation(RoundedIntPoint(layout_object.AbsoluteToLocal(
      rect_in_content.Location(), kUseTransforms)));
  return rect_in_content;
}

IntPoint LocalFrameView::ConvertFromLayoutObject(
    const LayoutObject& layout_object,
    const IntPoint& layout_object_point) const {
  return RoundedIntPoint(
      ConvertFromLayoutObject(layout_object, LayoutPoint(layout_object_point)));
}

IntPoint LocalFrameView::ConvertToLayoutObject(
    const LayoutObject& layout_object,
    const IntPoint& frame_point) const {
  return RoundedIntPoint(
      ConvertToLayoutObject(layout_object, LayoutPoint(frame_point)));
}

LayoutPoint LocalFrameView::ConvertFromLayoutObject(
    const LayoutObject& layout_object,
    const LayoutPoint& layout_object_point) const {
  LayoutPoint point(layout_object.LocalToAbsolute(
      FloatPoint(layout_object_point), kUseTransforms));

  // Convert from page ("absolute") to LocalFrameView coordinates.
  point.Move(-LayoutSize(GetScrollOffset()));
  return point;
}

LayoutPoint LocalFrameView::ConvertToLayoutObject(
    const LayoutObject& layout_object,
    const LayoutPoint& frame_point) const {
  return LayoutPoint(
      ConvertToLayoutObject(layout_object, FloatPoint(frame_point)));
}

FloatPoint LocalFrameView::ConvertToLayoutObject(
    const LayoutObject& layout_object,
    const FloatPoint& frame_point) const {
  FloatPoint point = frame_point;

  // Convert from LocalFrameView coords into page ("absolute") coordinates.
  point += FloatSize(ScrollX(), ScrollY());

  return layout_object.AbsoluteToLocal(point, kUseTransforms);
}

IntPoint LocalFrameView::ConvertSelfToChild(const EmbeddedContentView& child,
                                            const IntPoint& point) const {
  IntPoint new_point = point;
  new_point = FrameToContents(point);
  new_point.MoveBy(-child.FrameRect().Location());
  return new_point;
}

IntRect LocalFrameView::AbsoluteToRootFrame(
    const IntRect& absolute_rect) const {
  IntRect root_frame_rect(absolute_rect);
  root_frame_rect.Move(-ScrollOffsetInt());
  return ConvertToRootFrame(root_frame_rect);
}

IntPoint LocalFrameView::AbsoluteToRootFrame(
    const IntPoint& absolute_point) const {
  IntPoint point_in_frame(absolute_point);
  // With RLS turned on, this will be a no-op.
  point_in_frame -= ScrollOffsetInt();
  return ConvertToRootFrame(point_in_frame);
}

LayoutRect LocalFrameView::AbsoluteToRootFrame(
    const LayoutRect& layout_rect) const {
  LayoutPoint point_in_frame(layout_rect.Location());
  // With RLS turned on, this will be a no-op.
  point_in_frame -= LayoutSize(ScrollOffsetInt());
  return LayoutRect(ConvertToRootFrame(point_in_frame), layout_rect.Size());
}

IntRect LocalFrameView::RootFrameToDocument(const IntRect& rect_in_root_frame) {
  IntPoint offset = RootFrameToDocument(rect_in_root_frame.Location());
  IntRect local_rect = rect_in_root_frame;
  local_rect.SetLocation(offset);
  return local_rect;
}

IntPoint LocalFrameView::RootFrameToDocument(
    const IntPoint& point_in_root_frame) {
  return FlooredIntPoint(RootFrameToDocument(FloatPoint(point_in_root_frame)));
}

FloatPoint LocalFrameView::RootFrameToDocument(
    const FloatPoint& point_in_root_frame) {
  FloatPoint local_frame = ConvertFromRootFrame(point_in_root_frame);
  return local_frame + LayoutViewportScrollableArea()->GetScrollOffset();
}

LayoutPoint LocalFrameView::RootFrameToAbsolute(
    const LayoutPoint& point_in_root_frame) const {
  LayoutPoint local_frame = ConvertFromRootFrame(point_in_root_frame);
  // With RLS turned on, this will be a no-op.
  return local_frame + LayoutSize(GetScrollOffset());
}

IntPoint LocalFrameView::RootFrameToAbsolute(
    const IntPoint& point_in_root_frame) const {
  IntPoint local_frame = ConvertFromRootFrame(point_in_root_frame);
  // With RLS turned on, this will be a no-op.
  return local_frame + FlooredIntSize(GetScrollOffset());
}

IntRect LocalFrameView::RootFrameToAbsolute(
    const IntRect& rect_in_root_frame) const {
  IntRect absolute_rect = ConvertFromRootFrame(rect_in_root_frame);
  absolute_rect.Move(FlooredIntSize(GetScrollOffset()));
  return absolute_rect;
}

DoublePoint LocalFrameView::DocumentToAbsolute(
    const DoublePoint& point_in_document) const {
  return point_in_document -
         GetLayoutView()->GetScrollableArea()->GetScrollOffset();
}

FloatPoint LocalFrameView::DocumentToAbsolute(
    const FloatPoint& point_in_document) const {
  return FloatPoint(DocumentToAbsolute(DoublePoint(point_in_document)));
}

LayoutPoint LocalFrameView::DocumentToAbsolute(
    const LayoutPoint& point_in_document) const {
  return point_in_document -
         LayoutSize(GetLayoutView()->GetScrollableArea()->GetScrollOffset());
}

LayoutRect LocalFrameView::DocumentToAbsolute(
    const LayoutRect& rect_in_document) const {
  // With RLS turned off, this will be a no-op.
  return LayoutRect(DocumentToAbsolute(rect_in_document.Location()),
                    rect_in_document.Size());
}

LayoutPoint LocalFrameView::AbsoluteToDocument(
    const LayoutPoint& point_in_absolute) const {
  return point_in_absolute +
         LayoutSize(GetLayoutView()->GetScrollableArea()->GetScrollOffset());
}

LayoutRect LocalFrameView::AbsoluteToDocument(
    const LayoutRect& rect_in_absolute) const {
  return LayoutRect(AbsoluteToDocument(rect_in_absolute.Location()),
                    rect_in_absolute.Size());
}

IntRect LocalFrameView::ConvertToContainingEmbeddedContentView(
    const IntRect& local_rect) const {
  if (LocalFrameView* parent = ParentFrameView()) {
    auto* layout_object = frame_->OwnerLayoutObject();
    if (!layout_object)
      return local_rect;

    IntRect rect(local_rect);
    // Add borders and padding
    rect.Move(
        (layout_object->BorderLeft() + layout_object->PaddingLeft()).ToInt(),
        (layout_object->BorderTop() + layout_object->PaddingTop()).ToInt());
    return parent->ConvertFromLayoutObject(*layout_object, rect);
  }

  return local_rect;
}

IntRect LocalFrameView::ConvertFromContainingEmbeddedContentView(
    const IntRect& parent_rect) const {
  if (LocalFrameView* parent = ParentFrameView()) {
    IntRect local_rect = parent_rect;
    local_rect.SetLocation(
        parent->ConvertSelfToChild(*this, local_rect.Location()));
    return local_rect;
  }

  return parent_rect;
}

LayoutPoint LocalFrameView::ConvertToContainingEmbeddedContentView(
    const LayoutPoint& local_point) const {
  if (LocalFrameView* parent = ParentFrameView()) {
    auto* layout_object = frame_->OwnerLayoutObject();
    if (!layout_object)
      return local_point;

    LayoutPoint point(local_point);

    // Add borders and padding
    point.Move((layout_object->BorderLeft() + layout_object->PaddingLeft()),
               (layout_object->BorderTop() + layout_object->PaddingTop()));
    return parent->ConvertFromLayoutObject(*layout_object, point);
  }

  return local_point;
}

LayoutPoint LocalFrameView::ConvertFromContainingEmbeddedContentView(
    const LayoutPoint& parent_point) const {
  return LayoutPoint(
      ConvertFromContainingEmbeddedContentView(DoublePoint(parent_point)));
}

FloatPoint LocalFrameView::ConvertFromContainingEmbeddedContentView(
    const FloatPoint& parent_point) const {
  return FloatPoint(
      ConvertFromContainingEmbeddedContentView(DoublePoint(parent_point)));
}

DoublePoint LocalFrameView::ConvertFromContainingEmbeddedContentView(
    const DoublePoint& parent_point) const {
  if (LocalFrameView* parent = ParentFrameView()) {
    // Get our layoutObject in the parent view
    auto* layout_object = frame_->OwnerLayoutObject();
    if (!layout_object)
      return parent_point;

    DoublePoint point = DoublePoint(parent->ConvertToLayoutObject(
        *layout_object, FloatPoint(parent_point)));
    // Subtract borders and padding
    point.Move(
        (-layout_object->BorderLeft() - layout_object->PaddingLeft())
            .ToDouble(),
        (-layout_object->BorderTop() - layout_object->PaddingTop()).ToDouble());
    return point;
  }

  return parent_point;
}

IntPoint LocalFrameView::ConvertToContainingEmbeddedContentView(
    const IntPoint& local_point) const {
  return RoundedIntPoint(
      ConvertToContainingEmbeddedContentView(LayoutPoint(local_point)));
}

IntPoint LocalFrameView::ConvertFromContainingEmbeddedContentView(
    const IntPoint& parent_point) const {
  return RoundedIntPoint(
      ConvertFromContainingEmbeddedContentView(DoublePoint(parent_point)));
}

void LocalFrameView::SetInitialTracksPaintInvalidationsForTesting(
    bool track_paint_invalidations) {
  g_initial_track_all_paint_invalidations = track_paint_invalidations;
}

void LocalFrameView::SetTracksPaintInvalidations(
    bool track_paint_invalidations) {
  if (track_paint_invalidations == IsTrackingPaintInvalidations())
    return;

  // Ensure the document is up-to-date before tracking invalidations.
  UpdateAllLifecyclePhases();

  for (Frame* frame = &frame_->Tree().Top(); frame;
       frame = frame->Tree().TraverseNext()) {
    if (!frame->IsLocalFrame())
      continue;
    if (auto* layout_view = ToLocalFrame(frame)->ContentLayoutObject()) {
      layout_view->GetFrameView()->tracked_object_paint_invalidations_ =
          base::WrapUnique(track_paint_invalidations
                               ? new Vector<ObjectPaintInvalidation>
                               : nullptr);
      if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
        if (!paint_controller_)
          paint_controller_ = PaintController::Create();
        paint_controller_->SetTracksRasterInvalidations(
            track_paint_invalidations);
        if (paint_artifact_compositor_) {
          paint_artifact_compositor_->SetTracksRasterInvalidations(
              track_paint_invalidations);
        }
      } else {
        layout_view->Compositor()->UpdateTrackingRasterInvalidations();
      }
    }
  }

  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("blink.invalidation"),
                       "LocalFrameView::setTracksPaintInvalidations",
                       TRACE_EVENT_SCOPE_GLOBAL, "enabled",
                       track_paint_invalidations);
}

void LocalFrameView::TrackObjectPaintInvalidation(
    const DisplayItemClient& client,
    PaintInvalidationReason reason) {
  if (!tracked_object_paint_invalidations_)
    return;

  ObjectPaintInvalidation invalidation = {client.DebugName(), reason};
  tracked_object_paint_invalidations_->push_back(invalidation);
}

std::unique_ptr<JSONArray>
LocalFrameView::TrackedObjectPaintInvalidationsAsJSON() const {
  if (!tracked_object_paint_invalidations_)
    return nullptr;

  std::unique_ptr<JSONArray> result = JSONArray::Create();
  for (Frame* frame = &frame_->Tree().Top(); frame;
       frame = frame->Tree().TraverseNext()) {
    if (!frame->IsLocalFrame())
      continue;
    if (auto* layout_view = ToLocalFrame(frame)->ContentLayoutObject()) {
      if (!layout_view->GetFrameView()->tracked_object_paint_invalidations_)
        continue;
      for (const auto& item :
           *layout_view->GetFrameView()->tracked_object_paint_invalidations_) {
        std::unique_ptr<JSONObject> item_json = JSONObject::Create();
        item_json->SetString("object", item.name);
        item_json->SetString("reason",
                             PaintInvalidationReasonToString(item.reason));
        result->PushObject(std::move(item_json));
      }
    }
  }
  return result;
}

void LocalFrameView::AddResizerArea(LayoutBox& resizer_box) {
  if (!resizer_areas_)
    resizer_areas_ = std::make_unique<ResizerAreaSet>();
  resizer_areas_->insert(&resizer_box);
}

void LocalFrameView::RemoveResizerArea(LayoutBox& resizer_box) {
  if (!resizer_areas_)
    return;

  ResizerAreaSet::iterator it = resizer_areas_->find(&resizer_box);
  if (it != resizer_areas_->end())
    resizer_areas_->erase(it);
}

bool LocalFrameView::FrameIsScrollableDidChange() {
  DCHECK(GetFrame().IsLocalRoot());
  return GetScrollingContext()->WasScrollable() !=
         LayoutViewportScrollableArea()->IsScrollable();
}

void LocalFrameView::ClearFrameIsScrollableDidChange() {
  GetScrollingContext()->SetWasScrollable(GetFrame()
                                              .LocalFrameRoot()
                                              .View()
                                              ->LayoutViewportScrollableArea()
                                              ->IsScrollable());
}

void LocalFrameView::ScrollableAreasDidChange() {
  // Layout may update scrollable area bounding boxes. It also sets the same
  // dirty flag making this one redundant (See
  // |ScrollingCoordinator::notifyGeometryChanged|).
  // So if layout is expected, ignore this call allowing scrolling coordinator
  // to be notified post-layout to recompute gesture regions.
  // TODO(wjmaclean): It would be nice to move the !NeedsLayout() check from
  // here to SetScrollGestureRegionIsDirty(), but at present doing so breaks
  // layout tests. This suggests that there is something that wants to set the
  // dirty bit when layout is needed, and won't re-try setting the bit after
  // layout has completed - it would be nice to find that and fix it.
  if (!NeedsLayout())
    GetScrollingContext()->SetScrollGestureRegionIsDirty(true);
}

void LocalFrameView::AddScrollableArea(ScrollableArea* scrollable_area) {
  DCHECK(scrollable_area);
  if (!scrollable_areas_)
    scrollable_areas_ = new ScrollableAreaSet;
  scrollable_areas_->insert(scrollable_area);

  if (ScrollingCoordinator* scrolling_coordinator =
          this->GetScrollingCoordinator()) {
    ScrollableAreasDidChange();
  }
}

void LocalFrameView::RemoveScrollableArea(ScrollableArea* scrollable_area) {
  if (!scrollable_areas_)
    return;
  scrollable_areas_->erase(scrollable_area);

  if (ScrollingCoordinator* scrolling_coordinator =
          this->GetScrollingCoordinator()) {
    ScrollableAreasDidChange();
  }
}

void LocalFrameView::AddAnimatingScrollableArea(
    ScrollableArea* scrollable_area) {
  DCHECK(scrollable_area);
  if (!animating_scrollable_areas_)
    animating_scrollable_areas_ = new ScrollableAreaSet;
  animating_scrollable_areas_->insert(scrollable_area);
}

void LocalFrameView::RemoveAnimatingScrollableArea(
    ScrollableArea* scrollable_area) {
  if (!animating_scrollable_areas_)
    return;
  animating_scrollable_areas_->erase(scrollable_area);
}

void LocalFrameView::AttachToLayout() {
  // TODO(crbug.com/729196): Trace why LocalFrameView::DetachFromLayout crashes.
  CHECK(!is_attached_);
  if (frame_->GetDocument())
    CHECK_NE(Lifecycle().GetState(), DocumentLifecycle::kStopping);
  is_attached_ = true;
  parent_ = ParentFrameView();
  if (!parent_) {
    Frame* parent_frame = frame_->Tree().Parent();
    CHECK(parent_frame);
    CHECK(parent_frame->IsLocalFrame());
    CHECK(parent_frame->View());
  }
  CHECK(parent_);
  if (parent_->IsVisible())
    SetParentVisible(true);
  UpdateParentScrollableAreaSet();
  SetupRenderThrottling();
  subtree_throttled_ = ParentFrameView()->CanThrottleRendering();

  // We may have updated paint properties in detached frame subtree for
  // printing (see UpdateLifecyclePhasesForPrinting()). The paint properties
  // may change after the frame is attached.
  SetSubtreeNeedsPaintPropertyUpdate();
}

void LocalFrameView::DetachFromLayout() {
  // TODO(crbug.com/729196): Trace why LocalFrameView::DetachFromLayout crashes.
  CHECK(is_attached_);
  LocalFrameView* parent = ParentFrameView();
  if (!parent) {
    Frame* parent_frame = frame_->Tree().Parent();
    CHECK(parent_frame);
    CHECK(parent_frame->IsLocalFrame());
    CHECK(parent_frame->View());
  }
  CHECK(parent == parent_);
  if (!RuntimeEnabledFeatures::RootLayerScrollingEnabled())
    parent->RemoveScrollableArea(this);
  SetParentVisible(false);
  is_attached_ = false;

  // We may need update paint properties in detached frame subtree for printing.
  // See UpdateLifecyclePhasesForPrinting().
  SetSubtreeNeedsPaintPropertyUpdate();
}

void LocalFrameView::AddPlugin(WebPluginContainerImpl* plugin) {
  DCHECK(!plugins_.Contains(plugin));
  plugins_.insert(plugin);
}

void LocalFrameView::RemoveScrollbar(Scrollbar* scrollbar) {
  DCHECK(scrollbars_.Contains(scrollbar));
  scrollbars_.erase(scrollbar);
}

void LocalFrameView::AddScrollbar(Scrollbar* scrollbar) {
  DCHECK(!scrollbars_.Contains(scrollbar));
  scrollbars_.insert(scrollbar);
}

bool LocalFrameView::VisualViewportSuppliesScrollbars() {
  // On desktop, we always use the layout viewport's scrollbars.
  if (!frame_->GetSettings() || !frame_->GetSettings()->GetViewportEnabled() ||
      !frame_->GetDocument() || !frame_->GetPage())
    return false;

  const TopDocumentRootScrollerController& controller =
      frame_->GetPage()->GlobalRootScrollerController();

  if (!LayoutViewportScrollableArea())
    return false;

  return RootScrollerUtil::ScrollableAreaForRootScroller(
             controller.GlobalRootScroller()) == LayoutViewportScrollableArea();
}

AXObjectCache* LocalFrameView::ExistingAXObjectCache() const {
  if (GetFrame().GetDocument())
    return GetFrame().GetDocument()->ExistingAXObjectCache();
  return nullptr;
}

void LocalFrameView::SetCursor(const Cursor& cursor) {
  Page* page = GetFrame().GetPage();
  if (!page || frame_->GetEventHandler().IsMousePositionUnknown())
    return;
  page->GetChromeClient().SetCursor(cursor, frame_);
}

void LocalFrameView::FrameRectsChanged() {
  TRACE_EVENT0("blink", "LocalFrameView::frameRectsChanged");
  if (LayoutSizeFixedToFrameSize())
    SetLayoutSizeInternal(Size());

  ForAllChildViewsAndPlugins([](EmbeddedContentView& embedded_content_view) {
    embedded_content_view.FrameRectsChanged();
  });

  GetFrame().Client()->FrameRectsChanged(FrameRect());
}

void LocalFrameView::SetLayoutSizeInternal(const IntSize& size) {
  if (layout_size_ == size)
    return;

  layout_size_ = size;
  ContentsResized();
}

void LocalFrameView::DidAddScrollbar(Scrollbar& scrollbar,
                                     ScrollbarOrientation orientation) {
  ScrollableArea::DidAddScrollbar(scrollbar, orientation);
}

PaintLayer* LocalFrameView::Layer() const {
  auto* layout_view = GetLayoutView();
  if (!layout_view || !layout_view->Compositor())
    return nullptr;

  return layout_view->Compositor()->RootLayer();
}

IntSize LocalFrameView::MaximumScrollOffsetInt() const {
  // Make the same calculation as in CC's LayerImpl::MaxScrollOffset()
  // FIXME: We probably shouldn't be storing the bounds in a float.
  // crbug.com/422331.
  IntSize visible_size = VisibleContentSize(kExcludeScrollbars);
  IntSize content_bounds = ContentsSize();

  Page* page = frame_->GetPage();
  DCHECK(page);

  // We need to perform this const_cast since maximumScrollOffsetInt is a const
  // method but we can't make layoutViewportScrollableArea const since it can
  // return |this|. Once root-layer-scrolls ships layoutViewportScrollableArea
  // can be made const.
  const ScrollableArea* layout_viewport =
      const_cast<LocalFrameView*>(this)->LayoutViewportScrollableArea();
  TopDocumentRootScrollerController& controller =
      page->GlobalRootScrollerController();
  if (layout_viewport == controller.RootScrollerArea())
    visible_size = controller.RootScrollerVisibleArea();

  IntSize maximum_offset =
      ToIntSize(-ScrollOrigin() + (content_bounds - visible_size));
  return maximum_offset.ExpandedTo(MinimumScrollOffsetInt());
}

void LocalFrameView::SetScrollbarModes(ScrollbarMode horizontal_mode,
                                       ScrollbarMode vertical_mode) {
  bool needs_update = false;

  // If the page's overflow setting has disabled scrolling, do not allow
  // anything to override that setting, http://crbug.com/426447
  LayoutObject* viewport = ViewportLayoutObject();
  if (viewport && !ShouldIgnoreOverflowHidden()) {
    if (viewport->Style()->OverflowX() == EOverflow::kHidden)
      horizontal_mode = kScrollbarAlwaysOff;
    if (viewport->Style()->OverflowY() == EOverflow::kHidden)
      vertical_mode = kScrollbarAlwaysOff;
  }

  if (horizontal_mode != horizontal_scrollbar_mode_) {
    horizontal_scrollbar_mode_ = horizontal_mode;
    needs_update = true;
  }

  if (vertical_mode != vertical_scrollbar_mode_) {
    vertical_scrollbar_mode_ = vertical_mode;
    needs_update = true;
  }

  if (!needs_update)
    return;

  UpdateScrollbars();

  if (GetScrollingCoordinator())
    GetScrollingCoordinator()->UpdateUserInputScrollable(this);
}

ScrollbarMode LocalFrameView::EffectiveHorizontalScrollbarMode() const {
  // If we're autosizing, override the current scrollbar mode to whatever the
  // frame autosizer specifies.
  if (AutosizeHorizontalScrollbarMode() != kScrollbarAuto)
    return AutosizeHorizontalScrollbarMode();

  return horizontal_scrollbar_mode_;
}

ScrollbarMode LocalFrameView::EffectiveVerticalScrollbarMode() const {
  // If we're autosizing, override the current scrollbar mode to whatever the
  // frame autosizer specifies.
  if (AutosizeVerticalScrollbarMode() != kScrollbarAuto)
    return AutosizeVerticalScrollbarMode();

  return vertical_scrollbar_mode_;
}

IntSize LocalFrameView::VisibleContentSize(
    IncludeScrollbarsInRect scrollbar_inclusion) const {
  return scrollbar_inclusion == kExcludeScrollbars ? ExcludeScrollbars(Size())
                                                   : Size();
}

IntRect LocalFrameView::VisibleContentRect(
    IncludeScrollbarsInRect scrollbar_inclusion) const {
  return IntRect(IntPoint(FlooredIntSize(scroll_offset_)),
                 VisibleContentSize(scrollbar_inclusion));
}

LayoutRect LocalFrameView::VisibleScrollSnapportRect() const {
  const ComputedStyle* style = GetLayoutBox()->Style();
  LayoutRect visible_content_rect = LayoutRect(
      FloatPoint(scroll_offset_), VisibleContentSize(kExcludeScrollbars));
  LayoutRectOutsets padding(
      MinimumValueForLength(style->ScrollPaddingTop(),
                            visible_content_rect.Height()),
      MinimumValueForLength(style->ScrollPaddingRight(),
                            visible_content_rect.Width()),
      MinimumValueForLength(style->ScrollPaddingBottom(),
                            visible_content_rect.Height()),
      MinimumValueForLength(style->ScrollPaddingLeft(),
                            visible_content_rect.Width()));
  visible_content_rect.Contract(padding);
  return visible_content_rect;
}

IntSize LocalFrameView::ContentsSize() const {
  if (RuntimeEnabledFeatures::RootLayerScrollingEnabled())
    return Size();
  return layout_overflow_size_;
}

void LocalFrameView::ClipPaintRect(FloatRect* paint_rect) const {
  // Paint the whole rect if "mainFrameClipsContent" is false, meaning that
  // WebPreferences::record_whole_document is true.
  if (!frame_->GetSettings()->GetMainFrameClipsContent())
    return;

  paint_rect->Intersect(
      GetPage()->GetChromeClient().VisibleContentRectForPainting().value_or(
          VisibleContentRect()));
}

IntSize LocalFrameView::MinimumScrollOffsetInt() const {
  return ToIntSize(-ScrollOrigin());
}

void LocalFrameView::AdjustScrollbarOpacity() {
  if (HorizontalScrollbar() && LayerForHorizontalScrollbar()) {
    bool is_opaque_scrollbar = !HorizontalScrollbar()->IsOverlayScrollbar();
    LayerForHorizontalScrollbar()->SetContentsOpaque(is_opaque_scrollbar);
  }
  if (VerticalScrollbar() && LayerForVerticalScrollbar()) {
    bool is_opaque_scrollbar = !VerticalScrollbar()->IsOverlayScrollbar();
    LayerForVerticalScrollbar()->SetContentsOpaque(is_opaque_scrollbar);
  }
}

int LocalFrameView::ScrollSize(ScrollbarOrientation orientation) const {
  Scrollbar* scrollbar =
      ((orientation == kHorizontalScrollbar) ? HorizontalScrollbar()
                                             : VerticalScrollbar());

  // If no scrollbars are present, the content may still be scrollable.
  if (!scrollbar) {
    IntSize scroll_size = ContentsSize() - VisibleContentSize();
    scroll_size.ClampNegativeToZero();
    return orientation == kHorizontalScrollbar ? scroll_size.Width()
                                               : scroll_size.Height();
  }

  return scrollbar->TotalSize() - scrollbar->VisibleSize();
}

void LocalFrameView::UpdateScrollOffset(const ScrollOffset& offset,
                                        ScrollType scroll_type) {
  ScrollOffset scroll_delta = offset - scroll_offset_;
  if (scroll_delta.IsZero())
    return;

  if (RuntimeEnabledFeatures::RootLayerScrollingEnabled()) {
    // Don't scroll the LocalFrameView!
    NOTREACHED();
  }

  scroll_offset_ = offset;

  if (!ScrollbarsSuppressed())
    pending_scroll_delta_ += scroll_delta;

  UpdateLayersAndCompositingAfterScrollIfNeeded();

  Document* document = frame_->GetDocument();
  document->EnqueueScrollEventForNode(document);

  GetLayoutView()->DispatchFakeMouseMoveEventSoon(GetFrame().GetEventHandler());

  if (scroll_type == kUserScroll || scroll_type == kCompositorScroll) {
    Page* page = GetFrame().GetPage();
    if (page)
      page->GetChromeClient().ClearToolTip(*frame_);
  }

  auto* layout_view = GetLayoutView();
  if (layout_view)
    layout_view->ClearHitTestCache();

  did_scroll_timer_.StartOneShot(kResourcePriorityUpdateDelayAfterScroll,
                                 FROM_HERE);

  if (AXObjectCache* cache = frame_->GetDocument()->ExistingAXObjectCache())
    cache->HandleScrollPositionChanged(this);

  GetFrame().Loader().SaveScrollState();
  DidChangeScrollOffset();

  if (scroll_type == kCompositorScroll || scroll_type == kUserScroll) {
    if (DocumentLoader* document_loader = frame_->Loader().GetDocumentLoader())
      document_loader->GetInitialScrollState().was_scrolled_by_user = true;
  }

  if (IsExplicitScrollType(scroll_type)) {
    if (scroll_type != kCompositorScroll)
      ShowOverlayScrollbars();
    ClearFragmentAnchor();
    ClearScrollAnchor();
  }
}

void LocalFrameView::DidChangeScrollOffset() {
  GetFrame().Client()->DidChangeScrollOffset();
  if (GetFrame().IsMainFrame())
    GetFrame().GetPage()->GetChromeClient().MainFrameScrollOffsetChanged();
}

void LocalFrameView::ClearScrollAnchor() {
  scroll_anchor_.Clear();
}

bool LocalFrameView::HasOverlayScrollbars() const {
  return (HorizontalScrollbar() &&
          HorizontalScrollbar()->IsOverlayScrollbar()) ||
         (VerticalScrollbar() && VerticalScrollbar()->IsOverlayScrollbar());
}

void LocalFrameView::ComputeScrollbarExistence(
    bool& new_has_horizontal_scrollbar,
    bool& new_has_vertical_scrollbar,
    const IntSize& doc_size,
    ComputeScrollbarExistenceOption option) {
  if ((frame_->GetSettings() && frame_->GetSettings()->GetHideScrollbars()) ||
      VisualViewportSuppliesScrollbars()) {
    new_has_horizontal_scrollbar = false;
    new_has_vertical_scrollbar = false;
    return;
  }

  bool has_horizontal_scrollbar = HorizontalScrollbar();
  bool has_vertical_scrollbar = VerticalScrollbar();

  new_has_horizontal_scrollbar = has_horizontal_scrollbar;
  new_has_vertical_scrollbar = has_vertical_scrollbar;

  if (RuntimeEnabledFeatures::RootLayerScrollingEnabled())
    return;

  ScrollbarMode h_scroll = EffectiveHorizontalScrollbarMode();
  ScrollbarMode v_scroll = EffectiveVerticalScrollbarMode();

  if (h_scroll != kScrollbarAuto)
    new_has_horizontal_scrollbar = (h_scroll == kScrollbarAlwaysOn);
  if (v_scroll != kScrollbarAuto)
    new_has_vertical_scrollbar = (v_scroll == kScrollbarAlwaysOn);

  if (scrollbars_suppressed_ ||
      (h_scroll != kScrollbarAuto && v_scroll != kScrollbarAuto))
    return;

  if (h_scroll == kScrollbarAuto)
    new_has_horizontal_scrollbar = doc_size.Width() > VisibleWidth();
  if (v_scroll == kScrollbarAuto)
    new_has_vertical_scrollbar = doc_size.Height() > VisibleHeight();

  if (HasOverlayScrollbars())
    return;

  IntSize full_visible_size = VisibleContentSize(kIncludeScrollbars);

  bool attempt_to_remove_scrollbars =
      (option == kFirstPass && doc_size.Width() <= full_visible_size.Width() &&
       doc_size.Height() <= full_visible_size.Height());
  if (attempt_to_remove_scrollbars) {
    if (h_scroll == kScrollbarAuto)
      new_has_horizontal_scrollbar = false;
    if (v_scroll == kScrollbarAuto)
      new_has_vertical_scrollbar = false;
  }
}

void LocalFrameView::UpdateScrollbarEnabledState() {
  bool force_disabled =
      GetPageScrollbarTheme().ShouldDisableInvisibleScrollbars() &&
      ScrollbarsHiddenIfOverlay();

  if (HorizontalScrollbar()) {
    HorizontalScrollbar()->SetEnabled(ContentsWidth() > VisibleWidth() &&
                                      !force_disabled);
  }
  if (VerticalScrollbar()) {
    VerticalScrollbar()->SetEnabled(ContentsHeight() > VisibleHeight() &&
                                    !force_disabled);
  }
}

void LocalFrameView::UpdateScrollbarGeometry() {
  UpdateScrollbarEnabledState();
  if (HorizontalScrollbar()) {
    int thickness = HorizontalScrollbar()->ScrollbarThickness();
    IntRect old_rect(HorizontalScrollbar()->FrameRect());
    IntRect h_bar_rect(
        (ShouldPlaceVerticalScrollbarOnLeft() && VerticalScrollbar())
            ? VerticalScrollbar()->Width()
            : 0,
        Height() - thickness,
        Width() - (VerticalScrollbar() ? VerticalScrollbar()->Width() : 0),
        thickness);
    HorizontalScrollbar()->SetFrameRect(h_bar_rect);
    if (old_rect != HorizontalScrollbar()->FrameRect())
      SetScrollbarNeedsPaintInvalidation(kHorizontalScrollbar);

    HorizontalScrollbar()->SetProportion(VisibleWidth(), ContentsWidth());
    HorizontalScrollbar()->OffsetDidChange();
  }

  if (VerticalScrollbar()) {
    int thickness = VerticalScrollbar()->ScrollbarThickness();
    IntRect old_rect(VerticalScrollbar()->FrameRect());
    IntRect v_bar_rect(
        ShouldPlaceVerticalScrollbarOnLeft() ? 0 : (Width() - thickness), 0,
        thickness,
        Height() -
            (HorizontalScrollbar() ? HorizontalScrollbar()->Height() : 0));
    VerticalScrollbar()->SetFrameRect(v_bar_rect);
    if (old_rect != VerticalScrollbar()->FrameRect())
      SetScrollbarNeedsPaintInvalidation(kVerticalScrollbar);

    VerticalScrollbar()->SetProportion(VisibleHeight(), ContentsHeight());
    VerticalScrollbar()->OffsetDidChange();
  }
}

bool LocalFrameView::AdjustScrollbarExistence(
    ComputeScrollbarExistenceOption option) {
  DCHECK(in_update_scrollbars_);

  // If we came in here with the view already needing a layout, then go ahead
  // and do that first.  (This will be the common case, e.g., when the page
  // changes due to window resizing for example).  This layout will not re-enter
  // updateScrollbars and does not count towards our max layout pass total.
  if (!scrollbars_suppressed_)
    ScrollbarExistenceMaybeChanged();

  bool has_horizontal_scrollbar = HorizontalScrollbar();
  bool has_vertical_scrollbar = VerticalScrollbar();

  bool new_has_horizontal_scrollbar = false;
  bool new_has_vertical_scrollbar = false;
  ComputeScrollbarExistence(new_has_horizontal_scrollbar,
                            new_has_vertical_scrollbar, ContentsSize(), option);

  bool scrollbar_existence_changed =
      has_horizontal_scrollbar != new_has_horizontal_scrollbar ||
      has_vertical_scrollbar != new_has_vertical_scrollbar;
  if (!scrollbar_existence_changed)
    return false;

  scrollbar_manager_.SetHasHorizontalScrollbar(new_has_horizontal_scrollbar);
  scrollbar_manager_.SetHasVerticalScrollbar(new_has_vertical_scrollbar);

  if (scrollbars_suppressed_)
    return true;

  Element* custom_scrollbar_element = nullptr;
  bool uses_overlay_scrollbars =
      GetPageScrollbarTheme().UsesOverlayScrollbars() &&
      !ShouldUseCustomScrollbars(custom_scrollbar_element);

  if (!uses_overlay_scrollbars)
    SetNeedsLayout();

  ScrollbarExistenceMaybeChanged();
  return true;
}

bool LocalFrameView::NeedsScrollbarReconstruction() const {
  // We have no scrollbar to reconstruct.
  if (!HorizontalScrollbar() && !VerticalScrollbar())
    return false;

  Element* style_source = nullptr;
  bool needs_custom = ShouldUseCustomScrollbars(style_source);

  Scrollbar* scrollbars[] = {HorizontalScrollbar(), VerticalScrollbar()};

  for (Scrollbar* scrollbar : scrollbars) {
    if (!scrollbar)
      continue;

    // We have a native scrollbar that should be custom, or vice versa.
    if (scrollbar->IsCustomScrollbar() != needs_custom)
      return true;

    if (needs_custom) {
      DCHECK(scrollbar->IsCustomScrollbar());
      // We have a custom scrollbar with a stale m_owner.
      if (ToLayoutScrollbar(scrollbar)->StyleSource() !=
          style_source->GetLayoutObject())
        return true;

      // Should use custom scrollbar and nothing should change.
      continue;
    }

    // Check if native scrollbar should change.
    Page* page = frame_->GetPage();
    DCHECK(page);
    ScrollbarTheme* current_theme = &page->GetScrollbarTheme();

    if (current_theme != &scrollbar->GetTheme())
      return true;
  }
  return false;
}

bool LocalFrameView::ShouldIgnoreOverflowHidden() const {
  return frame_->GetSettings()->GetIgnoreMainFrameOverflowHiddenQuirk() &&
         frame_->IsMainFrame();
}

void LocalFrameView::UpdateScrollbarsIfNeeded() {
  if (needs_scrollbars_update_ || NeedsScrollbarReconstruction() ||
      ScrollOriginChanged())
    UpdateScrollbars();
}

void LocalFrameView::UpdateScrollbars() {
  needs_scrollbars_update_ = false;

  if (RuntimeEnabledFeatures::RootLayerScrollingEnabled())
    return;

  SetNeedsPaintPropertyUpdate();

  // Avoid drawing two sets of scrollbars when visual viewport is enabled.
  if (VisualViewportSuppliesScrollbars()) {
    if (scrollbar_manager_.HasHorizontalScrollbar() ||
        scrollbar_manager_.HasVerticalScrollbar()) {
      scrollbar_manager_.SetHasHorizontalScrollbar(false);
      scrollbar_manager_.SetHasVerticalScrollbar(false);
      ScrollbarExistenceMaybeChanged();
    }
    AdjustScrollOffsetFromUpdateScrollbars();
    return;
  }

  if (in_update_scrollbars_)
    return;
  InUpdateScrollbarsScope in_update_scrollbars_scope(this);

  bool scrollbar_existence_changed = false;

  if (NeedsScrollbarReconstruction()) {
    scrollbar_manager_.SetHasHorizontalScrollbar(false);
    scrollbar_manager_.SetHasVerticalScrollbar(false);
    scrollbar_existence_changed = true;
  }

  int max_update_scrollbars_pass =
      HasOverlayScrollbars() || scrollbars_suppressed_ ? 1 : 3;
  for (int update_scrollbars_pass = 0;
       update_scrollbars_pass < max_update_scrollbars_pass;
       update_scrollbars_pass++) {
    if (!AdjustScrollbarExistence(update_scrollbars_pass ? kIncremental
                                                         : kFirstPass))
      break;
    scrollbar_existence_changed = true;
  }

  UpdateScrollbarGeometry();

  if (scrollbar_existence_changed) {
    // FIXME: Is frameRectsChanged really necessary here? Have any frame rects
    // changed?
    FrameRectsChanged();
    PositionScrollbarLayers();
    UpdateScrollCorner();
  }

  AdjustScrollOffsetFromUpdateScrollbars();
}

void LocalFrameView::AdjustScrollOffsetFromUpdateScrollbars() {
  ScrollOffset clamped = ClampScrollOffset(GetScrollOffset());
  if (clamped != GetScrollOffset() || ScrollOriginChanged())
    SetScrollOffset(clamped, kClampingScroll);
}

ScrollableArea* LocalFrameView::ScrollableAreaWithElementId(
    const CompositorElementId& id) {
  // Check for the layout viewport, which may not be in scrollable_areas_ if it
  // is styled overflow: hidden.  (Other overflow: hidden elements won't have
  // composited scrolling layers per crbug.com/784053, so we don't have to worry
  // about them.)
  ScrollableArea* viewport = LayoutViewportScrollableArea();
  if (id == viewport->GetCompositorElementId())
    return viewport;

  if (scrollable_areas_) {
    // This requires iterating over all scrollable areas. We may want to store a
    // map of ElementId to ScrollableArea if this is an issue for performance.
    for (ScrollableArea* scrollable_area : *scrollable_areas_) {
      if (id == scrollable_area->GetCompositorElementId())
        return scrollable_area;
    }
  }
  return nullptr;
}

void LocalFrameView::ScrollRectToVisibleInRemoteParent(
    const LayoutRect& rect_to_scroll,
    const WebScrollIntoViewParams& params) {
  DCHECK(GetFrame().IsLocalRoot() && !GetFrame().IsMainFrame() &&
         safe_to_propagate_scroll_to_parent_);
  LayoutRect new_rect = AbsoluteToRootFrame(rect_to_scroll);
  GetFrame().Client()->ScrollRectToVisibleInParentFrame(
      WebRect(new_rect.X().ToInt(), new_rect.Y().ToInt(),
              new_rect.Width().ToInt(), new_rect.Height().ToInt()),
      params);
}

void LocalFrameView::NotifyFrameRectsChangedIfNeeded() {
  if (root_layer_did_scroll_) {
    root_layer_did_scroll_ = false;
    FrameRectsChanged();
  }
}

void LocalFrameView::ScrollContentsIfNeeded() {
  if (pending_scroll_delta_.IsZero())
    return;
  ScrollOffset scroll_delta = pending_scroll_delta_;
  pending_scroll_delta_ = ScrollOffset();
  // FIXME: Change scrollContents() to take DoubleSize. crbug.com/414283.
  ScrollContents(FlooredIntSize(scroll_delta));
}

void LocalFrameView::ScrollContents(const IntSize& scroll_delta) {
  ChromeClient* client = GetChromeClient();
  if (!client)
    return;

  TRACE_EVENT0("blink", "LocalFrameView::scrollContents");

  if (!ScrollContentsFastPath(-scroll_delta))
    ScrollContentsSlowPath();

  // This call will move children with native FrameViews (plugins) and
  // invalidate them as well.
  FrameRectsChanged();
}

FloatPoint LocalFrameView::ContentsToFrame(
    const FloatPoint& point_in_content_space) const {
  return point_in_content_space - GetScrollOffset();
}

LayoutPoint LocalFrameView::ContentsToFrame(
    const LayoutPoint& point_in_content_space) const {
  return point_in_content_space - LayoutSize(GetScrollOffset());
}

IntPoint LocalFrameView::ContentsToFrame(
    const IntPoint& point_in_content_space) const {
  return point_in_content_space - ScrollOffsetInt();
}

IntRect LocalFrameView::ContentsToFrame(
    const IntRect& rect_in_content_space) const {
  return IntRect(ContentsToFrame(rect_in_content_space.Location()),
                 rect_in_content_space.Size());
}

FloatPoint LocalFrameView::FrameToContents(
    const FloatPoint& point_in_frame) const {
  return point_in_frame + GetScrollOffset();
}

LayoutPoint LocalFrameView::FrameToContents(
    const LayoutPoint& point_in_frame) const {
  return point_in_frame + LayoutSize(GetScrollOffset());
}

IntPoint LocalFrameView::FrameToContents(const IntPoint& point_in_frame) const {
  return point_in_frame + ScrollOffsetInt();
}

IntRect LocalFrameView::FrameToContents(const IntRect& rect_in_frame) const {
  return IntRect(FrameToContents(rect_in_frame.Location()),
                 rect_in_frame.Size());
}

LayoutPoint LocalFrameView::RootFrameToContents(
    const LayoutPoint& point_in_root_frame) const {
  LayoutPoint frame_point = ConvertFromRootFrame(point_in_root_frame);
  return FrameToContents(frame_point);
}

FloatPoint LocalFrameView::RootFrameToContents(
    const FloatPoint& point_in_root_frame) const {
  FloatPoint frame_point = ConvertFromRootFrame(point_in_root_frame);
  return FrameToContents(frame_point);
}

IntPoint LocalFrameView::RootFrameToContents(
    const IntPoint& root_frame_point) const {
  IntPoint frame_point = ConvertFromRootFrame(root_frame_point);
  return FrameToContents(frame_point);
}

IntRect LocalFrameView::RootFrameToContents(
    const IntRect& root_frame_rect) const {
  return IntRect(RootFrameToContents(root_frame_rect.Location()),
                 root_frame_rect.Size());
}

LayoutPoint LocalFrameView::ContentsToRootFrame(
    const LayoutPoint& contents_point) const {
  LayoutPoint frame_point = ContentsToFrame(contents_point);
  return ConvertToRootFrame(frame_point);
}

IntPoint LocalFrameView::ContentsToRootFrame(
    const IntPoint& contents_point) const {
  IntPoint frame_point = ContentsToFrame(contents_point);
  return ConvertToRootFrame(frame_point);
}

IntRect LocalFrameView::ContentsToRootFrame(
    const IntRect& contents_rect) const {
  IntRect rect_in_frame = ContentsToFrame(contents_rect);
  return ConvertToRootFrame(rect_in_frame);
}

LayoutPoint LocalFrameView::ViewportToContents(
    const LayoutPoint& point_in_viewport) const {
  LayoutPoint point_in_root_frame(
      frame_->GetPage()->GetVisualViewport().ViewportToRootFrame(
          FloatPoint(point_in_viewport)));
  LayoutPoint point_in_frame = ConvertFromRootFrame(point_in_root_frame);
  return FrameToContents(point_in_frame);
}

FloatPoint LocalFrameView::ViewportToContents(
    const FloatPoint& point_in_viewport) const {
  FloatPoint point_in_root_frame(
      frame_->GetPage()->GetVisualViewport().ViewportToRootFrame(
          point_in_viewport));
  FloatPoint point_in_frame = ConvertFromRootFrame(point_in_root_frame);
  return FrameToContents(point_in_frame);
}

IntRect LocalFrameView::ViewportToContents(
    const IntRect& rect_in_viewport) const {
  IntRect rect_in_root_frame =
      frame_->GetPage()->GetVisualViewport().ViewportToRootFrame(
          rect_in_viewport);
  IntRect frame_rect = ConvertFromRootFrame(rect_in_root_frame);
  return FrameToContents(frame_rect);
}

IntPoint LocalFrameView::ViewportToContents(
    const IntPoint& point_in_viewport) const {
  return RoundedIntPoint(ViewportToContents(LayoutPoint(point_in_viewport)));
}

IntRect LocalFrameView::ContentsToViewport(
    const IntRect& rect_in_contents) const {
  IntRect rect_in_frame = ContentsToFrame(rect_in_contents);
  IntRect rect_in_root_frame = ConvertToRootFrame(rect_in_frame);
  return frame_->GetPage()->GetVisualViewport().RootFrameToViewport(
      rect_in_root_frame);
}

IntPoint LocalFrameView::ContentsToViewport(
    const IntPoint& point_in_contents) const {
  IntPoint point_in_frame = ContentsToFrame(point_in_contents);
  IntPoint point_in_root_frame = ConvertToRootFrame(point_in_frame);
  return frame_->GetPage()->GetVisualViewport().RootFrameToViewport(
      point_in_root_frame);
}

IntRect LocalFrameView::ContentsToScreen(const IntRect& rect) const {
  ChromeClient* client = GetChromeClient();
  if (!client)
    return IntRect();
  return client->ViewportToScreen(ContentsToViewport(rect), this);
}

IntPoint LocalFrameView::SoonToBeRemovedUnscaledViewportToContents(
    const IntPoint& point_in_viewport) const {
  IntPoint point_in_root_frame = FlooredIntPoint(
      frame_->GetPage()->GetVisualViewport().ViewportCSSPixelsToRootFrame(
          point_in_viewport));
  IntPoint point_in_this_frame = ConvertFromRootFrame(point_in_root_frame);
  return FrameToContents(point_in_this_frame);
}

Scrollbar* LocalFrameView::ScrollbarAtFramePoint(
    const IntPoint& point_in_frame) {
  if (HorizontalScrollbar() &&
      HorizontalScrollbar()->ShouldParticipateInHitTesting() &&
      HorizontalScrollbar()->FrameRect().Contains(point_in_frame))
    return HorizontalScrollbar();
  if (VerticalScrollbar() &&
      VerticalScrollbar()->ShouldParticipateInHitTesting() &&
      VerticalScrollbar()->FrameRect().Contains(point_in_frame))
    return VerticalScrollbar();
  return nullptr;
}

static void PositionScrollbarLayer(GraphicsLayer* graphics_layer,
                                   Scrollbar* scrollbar) {
  if (!graphics_layer || !scrollbar)
    return;

  IntRect scrollbar_rect = scrollbar->FrameRect();
  graphics_layer->SetPosition(scrollbar_rect.Location());
  graphics_layer->SetOffsetFromLayoutObject(
      ToIntSize(scrollbar_rect.Location()));

  if (scrollbar_rect.Size() == graphics_layer->Size())
    return;

  graphics_layer->SetSize(scrollbar_rect.Size());

  if (graphics_layer->HasContentsLayer()) {
    graphics_layer->SetContentsRect(
        IntRect(0, 0, scrollbar_rect.Width(), scrollbar_rect.Height()));
    return;
  }

  graphics_layer->SetDrawsContent(true);
  graphics_layer->SetNeedsDisplay();
}

static void PositionScrollCornerLayer(GraphicsLayer* graphics_layer,
                                      const IntRect& corner_rect) {
  if (!graphics_layer)
    return;
  graphics_layer->SetDrawsContent(!corner_rect.IsEmpty());
  graphics_layer->SetPosition(corner_rect.Location());
  graphics_layer->SetOffsetFromLayoutObject(ToIntSize(corner_rect.Location()));
  if (corner_rect.Size() != graphics_layer->Size())
    graphics_layer->SetNeedsDisplay();
  graphics_layer->SetSize(corner_rect.Size());
}

void LocalFrameView::PositionScrollbarLayers() {
  PositionScrollbarLayer(LayerForHorizontalScrollbar(), HorizontalScrollbar());
  PositionScrollbarLayer(LayerForVerticalScrollbar(), VerticalScrollbar());
  PositionScrollCornerLayer(LayerForScrollCorner(), ScrollCornerRect());
}

bool LocalFrameView::UpdateAfterCompositingChange() {
  if (ScrollOriginChanged())
    ResetScrollOriginChanged();

  return false;
}

bool LocalFrameView::UserInputScrollable(
    ScrollbarOrientation orientation) const {
  Document* document = GetFrame().GetDocument();
  Element* fullscreen_element = Fullscreen::FullscreenElementFrom(*document);
  if (fullscreen_element && fullscreen_element != document->documentElement())
    return false;

  if (RuntimeEnabledFeatures::RootLayerScrollingEnabled())
    return false;

  ScrollbarMode mode = (orientation == kHorizontalScrollbar)
                           ? EffectiveHorizontalScrollbarMode()
                           : EffectiveVerticalScrollbarMode();

  return mode == kScrollbarAuto || mode == kScrollbarAlwaysOn;
}

bool LocalFrameView::ShouldPlaceVerticalScrollbarOnLeft() const {
  return false;
}

LayoutRect LocalFrameView::ScrollIntoView(
    const LayoutRect& rect_in_absolute,
    const WebScrollIntoViewParams& params) {
  GetLayoutBox()->SetPendingOffsetToScroll(LayoutSize());

  LayoutRect scroll_snapport_rect(VisibleScrollSnapportRect());

  ScrollOffset new_scroll_offset =
      ClampScrollOffset(ScrollAlignment::GetScrollOffsetToExpose(
          scroll_snapport_rect, rect_in_absolute, params.GetScrollAlignmentX(),
          params.GetScrollAlignmentY(), GetScrollOffset()));
  ScrollOffset old_scroll_offset = GetScrollOffset();
  if (params.GetScrollType() == kUserScroll) {
    if (!UserInputScrollable(kHorizontalScrollbar))
      new_scroll_offset.SetWidth(old_scroll_offset.Width());
    if (!UserInputScrollable(kVerticalScrollbar))
      new_scroll_offset.SetHeight(old_scroll_offset.Height());
  }

  if (new_scroll_offset != old_scroll_offset) {
    new_scroll_offset = ShouldUseIntegerScrollOffset()
                            ? ScrollOffset(FlooredIntSize(new_scroll_offset))
                            : new_scroll_offset;
    if (params.is_for_scroll_sequence) {
      DCHECK(params.GetScrollType() == kProgrammaticScroll ||
             params.GetScrollType() == kUserScroll);
      ScrollBehavior behavior =
          DetermineScrollBehavior(params.GetScrollBehavior(),
                                  GetLayoutBox()->Style()->GetScrollBehavior());
      GetSmoothScrollSequencer()->QueueAnimation(this, new_scroll_offset,
                                                 behavior);
      ScrollOffset scroll_offset_difference =
          new_scroll_offset - old_scroll_offset;
      GetLayoutBox()->SetPendingOffsetToScroll(
          -LayoutSize(scroll_offset_difference));
    } else {
      SetScrollOffset(new_scroll_offset, params.GetScrollType());
    }
  }

  // Scrolling the LocalFrameView cannot change the input rect's location
  // relative to the document.
  // TODO(szager): PaintLayerScrollableArea::ScrollIntoView clips the return
  // value to the visible content rect, but this does not.
  return rect_in_absolute;
}

IntRect LocalFrameView::ScrollCornerRect() const {
  IntRect corner_rect;

  if (HasOverlayScrollbars())
    return corner_rect;

  if (HorizontalScrollbar() && Width() - HorizontalScrollbar()->Width() > 0) {
    corner_rect.Unite(IntRect(ShouldPlaceVerticalScrollbarOnLeft()
                                  ? 0
                                  : HorizontalScrollbar()->Width(),
                              Height() - HorizontalScrollbar()->Height(),
                              Width() - HorizontalScrollbar()->Width(),
                              HorizontalScrollbar()->Height()));
  }

  if (VerticalScrollbar() && Height() - VerticalScrollbar()->Height() > 0) {
    corner_rect.Unite(IntRect(ShouldPlaceVerticalScrollbarOnLeft()
                                  ? 0
                                  : (Width() - VerticalScrollbar()->Width()),
                              VerticalScrollbar()->Height(),
                              VerticalScrollbar()->Width(),
                              Height() - VerticalScrollbar()->Height()));
  }

  return corner_rect;
}

bool LocalFrameView::IsScrollCornerVisible() const {
  return !ScrollCornerRect().IsEmpty();
}

ScrollBehavior LocalFrameView::ScrollBehaviorStyle() const {
  Element* scroll_element = frame_->GetDocument()->scrollingElement();
  LayoutObject* layout_object =
      scroll_element ? scroll_element->GetLayoutObject() : nullptr;
  if (layout_object &&
      layout_object->Style()->GetScrollBehavior() == kScrollBehaviorSmooth)
    return kScrollBehaviorSmooth;

  return kScrollBehaviorInstant;
}

void LocalFrameView::Paint(GraphicsContext& context,
                           const GlobalPaintFlags global_paint_flags,
                           const CullRect& cull_rect,
                           const IntSize& paint_offset) const {
  // |paint_offset| is not used because paint properties of the contents will
  // ensure the correct location.
  PaintInternal(context, global_paint_flags, cull_rect);
}

void LocalFrameView::PaintWithLifecycleUpdate(
    GraphicsContext& context,
    const GlobalPaintFlags global_paint_flags,
    const CullRect& cull_rect) {
  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.Lifecycle().AdvanceTo(DocumentLifecycle::kInPaint);
  });

  PaintInternal(context, global_paint_flags, cull_rect);

  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.Lifecycle().AdvanceTo(DocumentLifecycle::kPaintClean);
  });
}

void LocalFrameView::PaintInternal(GraphicsContext& context,
                                   const GlobalPaintFlags global_paint_flags,
                                   const CullRect& cull_rect) const {
  FramePainter(*this).Paint(context, global_paint_flags, cull_rect);
}

void LocalFrameView::PaintContents(GraphicsContext& context,
                                   const GlobalPaintFlags global_paint_flags,
                                   const IntRect& damage_rect) {
  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.Lifecycle().AdvanceTo(DocumentLifecycle::kInPaint);
  });

  FramePainter(*this).PaintContents(context, global_paint_flags, damage_rect);

  ForAllNonThrottledLocalFrameViews([](LocalFrameView& frame_view) {
    frame_view.Lifecycle().AdvanceTo(DocumentLifecycle::kPaintClean);
  });
}

bool LocalFrameView::IsPointInScrollbarCorner(
    const IntPoint& point_in_root_frame) {
  if (!ScrollbarCornerPresent())
    return false;

  IntPoint frame_point = ConvertFromRootFrame(point_in_root_frame);

  if (HorizontalScrollbar()) {
    int horizontal_scrollbar_y_min = HorizontalScrollbar()->FrameRect().Y();
    int horizontal_scrollbar_y_max =
        HorizontalScrollbar()->FrameRect().Y() +
        HorizontalScrollbar()->FrameRect().Height();
    int horizontal_scrollbar_x_min = HorizontalScrollbar()->FrameRect().X() +
                                     HorizontalScrollbar()->FrameRect().Width();

    return frame_point.Y() > horizontal_scrollbar_y_min &&
           frame_point.Y() < horizontal_scrollbar_y_max &&
           frame_point.X() > horizontal_scrollbar_x_min;
  }

  int vertical_scrollbar_x_min = VerticalScrollbar()->FrameRect().X();
  int vertical_scrollbar_x_max = VerticalScrollbar()->FrameRect().X() +
                                 VerticalScrollbar()->FrameRect().Width();
  int vertical_scrollbar_y_min = VerticalScrollbar()->FrameRect().Y() +
                                 VerticalScrollbar()->FrameRect().Height();

  return frame_point.X() > vertical_scrollbar_x_min &&
         frame_point.X() < vertical_scrollbar_x_max &&
         frame_point.Y() > vertical_scrollbar_y_min;
}

bool LocalFrameView::ScrollbarCornerPresent() const {
  return (HorizontalScrollbar() &&
          Width() - HorizontalScrollbar()->Width() > 0) ||
         (VerticalScrollbar() && Height() - VerticalScrollbar()->Height() > 0);
}

IntRect LocalFrameView::ConvertToRootFrame(const IntRect& local_rect) const {
  if (LocalFrameView* parent = ParentFrameView()) {
    IntRect parent_rect = ConvertToContainingEmbeddedContentView(local_rect);
    return parent->ConvertToRootFrame(parent_rect);
  }
  return local_rect;
}

IntPoint LocalFrameView::ConvertToRootFrame(const IntPoint& local_point) const {
  return RoundedIntPoint(ConvertToRootFrame(LayoutPoint(local_point)));
}

LayoutPoint LocalFrameView::ConvertToRootFrame(
    const LayoutPoint& local_point) const {
  if (LocalFrameView* parent = ParentFrameView()) {
    LayoutPoint parent_point =
        ConvertToContainingEmbeddedContentView(local_point);
    return parent->ConvertToRootFrame(parent_point);
  }
  return local_point;
}

IntRect LocalFrameView::ConvertFromRootFrame(
    const IntRect& rect_in_root_frame) const {
  if (LocalFrameView* parent = ParentFrameView()) {
    IntRect parent_rect = parent->ConvertFromRootFrame(rect_in_root_frame);
    return ConvertFromContainingEmbeddedContentView(parent_rect);
  }
  return rect_in_root_frame;
}

IntPoint LocalFrameView::ConvertFromRootFrame(
    const IntPoint& point_in_root_frame) const {
  return RoundedIntPoint(
      ConvertFromRootFrame(LayoutPoint(point_in_root_frame)));
}

LayoutPoint LocalFrameView::ConvertFromRootFrame(
    const LayoutPoint& point_in_root_frame) const {
  if (LocalFrameView* parent = ParentFrameView()) {
    LayoutPoint parent_point =
        parent->ConvertFromRootFrame(point_in_root_frame);
    return ConvertFromContainingEmbeddedContentView(parent_point);
  }
  return point_in_root_frame;
}

FloatPoint LocalFrameView::ConvertFromRootFrame(
    const FloatPoint& point_in_root_frame) const {
  if (LocalFrameView* parent = ParentFrameView()) {
    FloatPoint parent_point = parent->ConvertFromRootFrame(point_in_root_frame);
    return ConvertFromContainingEmbeddedContentView(parent_point);
  }
  return point_in_root_frame;
}

IntPoint LocalFrameView::ConvertFromContainingEmbeddedContentViewToScrollbar(
    const Scrollbar& scrollbar,
    const IntPoint& parent_point) const {
  IntPoint new_point = parent_point;
  // Scrollbars won't be transformed within us
  new_point.MoveBy(-scrollbar.Location());
  return new_point;
}

void LocalFrameView::SetParentVisible(bool visible) {
  if (IsParentVisible() == visible)
    return;

  // As parent visibility changes, we may need to recomposite this frame view
  // and potentially child frame views.
  SetNeedsCompositingUpdate(kCompositingUpdateRebuildTree);

  parent_visible_ = visible;

  if (!IsSelfVisible())
    return;

  ForAllChildViewsAndPlugins(
      [visible](EmbeddedContentView& embedded_content_view) {
        embedded_content_view.SetParentVisible(visible);
      });
}

void LocalFrameView::Show() {
  if (!IsSelfVisible()) {
    SetSelfVisible(true);
    if (ScrollingCoordinator* scrolling_coordinator =
            this->GetScrollingCoordinator()) {
      GetScrollingContext()->SetScrollGestureRegionIsDirty(true);
    }
    SetNeedsCompositingUpdate(kCompositingUpdateRebuildTree);
    UpdateParentScrollableAreaSet();
    if (IsParentVisible()) {
      ForAllChildViewsAndPlugins(
          [](EmbeddedContentView& embedded_content_view) {
            embedded_content_view.SetParentVisible(true);
          });
    }
  }
}

void LocalFrameView::Hide() {
  if (IsSelfVisible()) {
    if (IsParentVisible()) {
      ForAllChildViewsAndPlugins(
          [](EmbeddedContentView& embedded_content_view) {
            embedded_content_view.SetParentVisible(false);
          });
    }
    SetSelfVisible(false);
    if (ScrollingCoordinator* scrolling_coordinator =
            this->GetScrollingCoordinator()) {
      GetScrollingContext()->SetScrollGestureRegionIsDirty(true);
    }
    SetNeedsCompositingUpdate(kCompositingUpdateRebuildTree);
    UpdateParentScrollableAreaSet();
  }
}

int LocalFrameView::ViewportWidth() const {
  int viewport_width = GetLayoutSize(kIncludeScrollbars).Width();
  return AdjustForAbsoluteZoom::AdjustInt(viewport_width, GetLayoutView());
}

ScrollableArea* LocalFrameView::GetScrollableArea() {
  if (viewport_scrollable_area_)
    return viewport_scrollable_area_.Get();

  return LayoutViewportScrollableArea();
}

ScrollableArea* LocalFrameView::LayoutViewportScrollableArea() {
  if (!RuntimeEnabledFeatures::RootLayerScrollingEnabled())
    return this;

  auto* layout_view = this->GetLayoutView();
  return layout_view ? layout_view->GetScrollableArea() : nullptr;
}

RootFrameViewport* LocalFrameView::GetRootFrameViewport() {
  return viewport_scrollable_area_.Get();
}

LayoutObject* LocalFrameView::ViewportLayoutObject() const {
  if (Document* document = GetFrame().GetDocument()) {
    if (Element* element = document->ViewportDefiningElement())
      return element->GetLayoutObject();
  }
  return nullptr;
}

void LocalFrameView::CollectAnnotatedRegions(
    LayoutObject& layout_object,
    Vector<AnnotatedRegionValue>& regions) const {
  // LayoutTexts don't have their own style, they just use their parent's style,
  // so we don't want to include them.
  if (layout_object.IsText())
    return;

  layout_object.AddAnnotatedRegions(regions);
  for (LayoutObject* curr = layout_object.SlowFirstChild(); curr;
       curr = curr->NextSibling())
    CollectAnnotatedRegions(*curr, regions);
}

void LocalFrameView::UpdateViewportIntersectionsForSubtree(
    DocumentLifecycle::LifecycleState target_state) {
  // TODO(dcheng): Since LocalFrameView tree updates are deferred, FrameViews
  // might still be in the LocalFrameView hierarchy even though the associated
  // Document is already detached. Investigate if this check and a similar check
  // in lifecycle updates are still needed when there are no more deferred
  // LocalFrameView updates: https://crbug.com/561683
  if (!GetFrame().GetDocument()->IsActive())
    return;

  if (target_state == DocumentLifecycle::kPaintClean) {
    RecordDeferredLoadingStats();
    if (!NeedsLayout()) {
      // Notify javascript IntersectionObservers
      if (GetFrame().GetDocument()->GetIntersectionObserverController()) {
        GetFrame()
            .GetDocument()
            ->GetIntersectionObserverController()
            ->ComputeTrackedIntersectionObservations();
      }
    }
  }

  // Don't throttle display:none frames (see updateRenderThrottlingStatus).
  HTMLFrameOwnerElement* owner_element = frame_->DeprecatedLocalOwner();
  if (hidden_for_throttling_ && owner_element &&
      !owner_element->GetLayoutObject()) {
    // No need to notify children because descendants of display:none frames
    // should remain throttled.
    UpdateRenderThrottlingStatus(hidden_for_throttling_, subtree_throttled_,
                                 kDontForceThrottlingInvalidation,
                                 kDontNotifyChildren);
  }

  for (Frame* child = frame_->Tree().FirstChild(); child;
       child = child->Tree().NextSibling()) {
    child->View()->UpdateViewportIntersectionsForSubtree(target_state);
  }
  needs_intersection_observation_ = false;
}

void LocalFrameView::UpdateRenderThrottlingStatusForTesting() {
  visibility_observer_->DeliverObservationsForTesting();
}

void LocalFrameView::CrossOriginStatusChanged() {
  // Cross-domain status is not stored as a dirty bit within LocalFrameView,
  // so force-invalidate throttling status when it changes regardless of
  // previous or new value.
  UpdateRenderThrottlingStatus(hidden_for_throttling_, subtree_throttled_,
                               kForceThrottlingInvalidation);
}

void LocalFrameView::UpdateRenderThrottlingStatus(
    bool hidden,
    bool subtree_throttled,
    ForceThrottlingInvalidationBehavior force_throttling_invalidation_behavior,
    NotifyChildrenBehavior notify_children_behavior) {
  TRACE_EVENT0("blink", "LocalFrameView::updateRenderThrottlingStatus");
  DCHECK(!IsInPerformLayout());
  DCHECK(!frame_->GetDocument() || !frame_->GetDocument()->InStyleRecalc());
  bool was_throttled = CanThrottleRendering();

  // Note that we disallow throttling of 0x0 and display:none frames because
  // some sites use them to drive UI logic.
  hidden_for_throttling_ = hidden && !Size().IsEmpty();
  subtree_throttled_ = subtree_throttled;
  HTMLFrameOwnerElement* owner_element = frame_->DeprecatedLocalOwner();
  if (owner_element)
    hidden_for_throttling_ &= !!owner_element->GetLayoutObject();

  bool is_throttled = CanThrottleRendering();
  bool became_unthrottled = was_throttled && !is_throttled;

  // If this LocalFrameView became unthrottled or throttled, we must make sure
  // all its children are notified synchronously. Otherwise we 1) might attempt
  // to paint one of the children with an out-of-date layout before
  // |updateRenderThrottlingStatus| has made it throttled or 2) fail to
  // unthrottle a child whose parent is unthrottled by a later notification.
  if (notify_children_behavior == kNotifyChildren &&
      (was_throttled != is_throttled ||
       force_throttling_invalidation_behavior ==
           kForceThrottlingInvalidation)) {
    ForAllChildLocalFrameViews([is_throttled](LocalFrameView& frame_view) {
      frame_view.UpdateRenderThrottlingStatus(frame_view.hidden_for_throttling_,
                                              is_throttled);
    });
  }

  ScrollingCoordinator* scrolling_coordinator = this->GetScrollingCoordinator();
  if (became_unthrottled ||
      force_throttling_invalidation_behavior == kForceThrottlingInvalidation) {
    // ScrollingCoordinator needs to update according to the new throttling
    // status.
    if (scrolling_coordinator)
      scrolling_coordinator->NotifyGeometryChanged(this);
    // Start ticking animation frames again if necessary.
    if (GetPage())
      GetPage()->Animator().ScheduleVisualUpdate(frame_.Get());
    // Force a full repaint of this frame to ensure we are not left with a
    // partially painted version of this frame's contents if we skipped
    // painting them while the frame was throttled.
    auto* layout_view = GetLayoutView();
    if (layout_view)
      layout_view->InvalidatePaintForViewAndCompositedLayers();
    // Also need to update all paint properties that might be skipped while
    // the frame was throttled.
    SetSubtreeNeedsPaintPropertyUpdate();
  }

  EventHandlerRegistry& registry = frame_->GetEventHandlerRegistry();
  bool has_handlers =
      (registry.HasEventHandlers(EventHandlerRegistry::kTouchAction) ||
       registry.HasEventHandlers(
           EventHandlerRegistry::kTouchStartOrMoveEventBlocking) ||
       registry.HasEventHandlers(
           EventHandlerRegistry::kTouchStartOrMoveEventBlockingLowLatency));
  if (was_throttled != CanThrottleRendering() && scrolling_coordinator &&
      has_handlers) {
    scrolling_coordinator->TouchEventTargetRectsDidChange(
        &GetFrame().LocalFrameRoot());
  }

  if (FrameScheduler* frame_scheduler = frame_->GetFrameScheduler()) {
    frame_scheduler->SetFrameVisible(!hidden_for_throttling_);
    frame_scheduler->SetCrossOrigin(frame_->IsCrossOriginSubframe());
    frame_scheduler->TraceUrlChange(frame_->GetDocument()->Url().GetString());
  }

#if DCHECK_IS_ON()
  // Make sure we never have an unthrottled frame inside a throttled one.
  LocalFrameView* parent = ParentFrameView();
  while (parent) {
    DCHECK(CanThrottleRendering() || !parent->CanThrottleRendering());
    parent = parent->ParentFrameView();
  }
#endif
}

void LocalFrameView::RecordDeferredLoadingStats() {
  if (!GetFrame().GetDocument()->GetFrame() ||
      !GetFrame().IsCrossOriginSubframe())
    return;

  LocalFrameView* parent = ParentFrameView();
  if (!parent) {
    HTMLFrameOwnerElement* element = GetFrame().DeprecatedLocalOwner();
    // We would fall into an else block on some teardowns and other weird cases.
    if (!element || !element->GetLayoutObject()) {
      GetFrame().GetDocument()->RecordDeferredLoadReason(
          WouldLoadReason::kNoParent);
    }
    return;
  }
  // Small inaccuracy: frames with origins that match the top level might be
  // nested in a cross-origin frame. To keep code simpler, count such frames as
  // WouldLoadVisible, even when their parent is offscreen.
  WouldLoadReason why_parent_loaded = WouldLoadReason::kVisible;
  if (parent->ParentFrameView() && parent->GetFrame().IsCrossOriginSubframe())
    why_parent_loaded = parent->GetFrame().GetDocument()->DeferredLoadReason();

  // If the parent wasn't loaded, the children won't be either.
  if (why_parent_loaded == WouldLoadReason::kCreated)
    return;
  // These frames are never meant to be seen so we will need to load them.
  IntRect frame_rect(FrameRect());
  if (frame_rect.IsEmpty() || frame_rect.MaxY() < 0 || frame_rect.MaxX() < 0) {
    GetFrame().GetDocument()->RecordDeferredLoadReason(why_parent_loaded);
    return;
  }

  IntRect parent_rect = parent->FrameRect();
  // First clause: for this rough data collection we assume the user never
  // scrolls right.
  if (frame_rect.X() >= parent_rect.Width() || parent_rect.Height() <= 0)
    return;

  int this_frame_screens_away = 0;
  // If an frame is created above the current scoll position, this logic counts
  // it as visible.
  if (frame_rect.Y() > parent->GetScrollOffset().Height()) {
    this_frame_screens_away =
        (frame_rect.Y() - parent->GetScrollOffset().Height()) /
        parent_rect.Height();
  }
  DCHECK_GE(this_frame_screens_away, 0);

  int parent_screens_away = 0;
  if (why_parent_loaded <= WouldLoadReason::kVisible) {
    parent_screens_away = static_cast<int>(WouldLoadReason::kVisible) -
                          static_cast<int>(why_parent_loaded);
  }

  int total_screens_away = this_frame_screens_away + parent_screens_away;

  // We're collecting data for frames that are at most 3 screens away.
  if (total_screens_away > 3)
    return;

  GetFrame().GetDocument()->RecordDeferredLoadReason(
      static_cast<WouldLoadReason>(static_cast<int>(WouldLoadReason::kVisible) -
                                   total_screens_away));
}

void LocalFrameView::SetNeedsForcedCompositingUpdate() {
  needs_forced_compositing_update_ = true;
  if (LocalFrameView* parent = ParentFrameView())
    parent->SetNeedsForcedCompositingUpdate();
}

void LocalFrameView::SetNeedsIntersectionObservation() {
  needs_intersection_observation_ = true;
  if (LocalFrameView* parent = ParentFrameView())
    parent->SetNeedsIntersectionObservation();
}

bool LocalFrameView::ShouldThrottleRendering() const {
  bool throttled_for_global_reasons = CanThrottleRendering() &&
                                      frame_->GetDocument() &&
                                      Lifecycle().ThrottlingAllowed();
  if (!throttled_for_global_reasons || needs_forced_compositing_update_)
    return false;

  // Only lifecycle phases up to layout are needed to generate an
  // intersection observation.
  return !needs_intersection_observation_ ||
         GetFrame().LocalFrameRoot().View()->past_layout_lifecycle_update_;
}

bool LocalFrameView::CanThrottleRendering() const {
  if (lifecycle_updates_throttled_)
    return true;
  if (!RuntimeEnabledFeatures::RenderingPipelineThrottlingEnabled())
    return false;
  if (subtree_throttled_)
    return true;
  // We only throttle hidden cross-origin frames. This is to avoid a situation
  // where an ancestor frame directly depends on the pipeline timing of a
  // descendant and breaks as a result of throttling. The rationale is that
  // cross-origin frames must already communicate with asynchronous messages,
  // so they should be able to tolerate some delay in receiving replies from a
  // throttled peer.
  return hidden_for_throttling_ && frame_->IsCrossOriginSubframe();
}

void LocalFrameView::BeginLifecycleUpdates() {
  // Avoid pumping frames for the initially empty document.
  if (!GetFrame().Loader().StateMachine()->CommittedFirstRealDocumentLoad())
    return;
  lifecycle_updates_throttled_ = false;
  if (auto* owner = GetFrame().OwnerLayoutObject())
    owner->SetMayNeedPaintInvalidation();

  LayoutView* layout_view = GetLayoutView();
  bool layout_view_is_empty = layout_view && !layout_view->FirstChild();
  if (layout_view_is_empty && !DidFirstLayout() && !NeedsLayout()) {
    // Make sure a display:none iframe gets an initial layout pass.
    layout_view->SetNeedsLayout(LayoutInvalidationReason::kAddedToLayout,
                                kMarkOnlyThis);
  }

  SetupRenderThrottling();
  UpdateRenderThrottlingStatus(hidden_for_throttling_, subtree_throttled_);
  // The compositor will "defer commits" for the main frame until we
  // explicitly request them.
  if (GetFrame().IsMainFrame())
    GetFrame().GetPage()->GetChromeClient().BeginLifecycleUpdates();
}

void LocalFrameView::SetInitialViewportSize(const IntSize& viewport_size) {
  if (viewport_size == initial_viewport_size_)
    return;

  initial_viewport_size_ = viewport_size;
  if (Document* document = frame_->GetDocument())
    document->GetStyleEngine().InitialViewportChanged();
}

int LocalFrameView::InitialViewportWidth() const {
  DCHECK(frame_->IsMainFrame());
  return initial_viewport_size_.Width();
}

int LocalFrameView::InitialViewportHeight() const {
  DCHECK(frame_->IsMainFrame());
  return initial_viewport_size_.Height();
}

bool LocalFrameView::HasVisibleSlowRepaintViewportConstrainedObjects() const {
  if (!ViewportConstrainedObjects())
    return false;
  for (const LayoutObject* layout_object : *ViewportConstrainedObjects()) {
    DCHECK(layout_object->IsBoxModelObject() && layout_object->HasLayer());
    DCHECK(layout_object->Style()->GetPosition() == EPosition::kFixed ||
           layout_object->Style()->GetPosition() == EPosition::kSticky);
    if (ToLayoutBoxModelObject(layout_object)->IsSlowRepaintConstrainedObject())
      return true;
  }
  return false;
}

void LocalFrameView::UpdateSubFrameScrollOnMainReason(
    const Frame& frame,
    MainThreadScrollingReasons parent_reason) {
  MainThreadScrollingReasons reasons = parent_reason;

  if (!GetPage()->GetSettings().GetThreadedScrollingEnabled())
    reasons |= MainThreadScrollingReason::kThreadedScrollingDisabled;

  if (!frame.IsLocalFrame())
    return;

  LocalFrameView& frame_view = *ToLocalFrame(frame).View();
  if (frame_view.ShouldThrottleRendering())
    return;

  if (!frame_view.LayoutViewportScrollableArea())
    return;

  reasons |= frame_view.MainThreadScrollingReasonsPerFrame();
  if (GraphicsLayer* layer_for_scrolling = ToLocalFrame(frame)
                                               .View()
                                               ->LayoutViewportScrollableArea()
                                               ->LayerForScrolling()) {
    if (cc::Layer* platform_layer_for_scrolling =
            layer_for_scrolling->CcLayer()) {
      if (reasons) {
        platform_layer_for_scrolling->AddMainThreadScrollingReasons(reasons);
      } else {
        // Clear all main thread scrolling reasons except the one that's set
        // if there is a running scroll animation.
        platform_layer_for_scrolling->ClearMainThreadScrollingReasons(
            ~MainThreadScrollingReason::kHandlingScrollFromMainThread);
      }
    }
  }

  Frame* child = frame.Tree().FirstChild();
  while (child) {
    UpdateSubFrameScrollOnMainReason(*child, reasons);
    child = child->Tree().NextSibling();
  }

  if (frame.IsMainFrame())
    main_thread_scrolling_reasons_ = reasons;
  DCHECK(!MainThreadScrollingReason::HasNonCompositedScrollReasons(
      main_thread_scrolling_reasons_));
}

MainThreadScrollingReasons LocalFrameView::MainThreadScrollingReasonsPerFrame()
    const {
  MainThreadScrollingReasons reasons =
      static_cast<MainThreadScrollingReasons>(0);

  if (ShouldThrottleRendering())
    return reasons;

  if (HasBackgroundAttachmentFixedObjects())
    reasons |= MainThreadScrollingReason::kHasBackgroundAttachmentFixedObjects;

  ScrollingReasons scrolling_reasons = GetScrollingReasons();
  const bool may_be_scrolled_by_input = (scrolling_reasons == kScrollable);
  const bool may_be_scrolled_by_script =
      may_be_scrolled_by_input ||
      (scrolling_reasons == kNotScrollableExplicitlyDisabled);

  // TODO(awoloszyn) Currently crbug.com/304810 will let certain
  // overflow:hidden elements scroll on the compositor thread, so we should
  // not let this move there path as an optimization, when we have
  // slow-repaint elements.
  if (may_be_scrolled_by_script &&
      HasVisibleSlowRepaintViewportConstrainedObjects()) {
    reasons |=
        MainThreadScrollingReason::kHasNonLayerViewportConstrainedObjects;
  }
  return reasons;
}

MainThreadScrollingReasons LocalFrameView::GetMainThreadScrollingReasons()
    const {
  MainThreadScrollingReasons reasons =
      static_cast<MainThreadScrollingReasons>(0);

  if (!GetPage()->GetSettings().GetThreadedScrollingEnabled())
    reasons |= MainThreadScrollingReason::kThreadedScrollingDisabled;

  if (!GetPage()->MainFrame()->IsLocalFrame())
    return reasons;

  // TODO(alexmos,kenrb): For OOPIF, local roots that are different from
  // the main frame can't be used in the calculation, since they use
  // different compositors with unrelated state, which breaks some of the
  // calculations below.
  if (&frame_->LocalFrameRoot() != GetPage()->MainFrame())
    return reasons;

  // Walk the tree to the root. Use the gathered reasons to determine
  // whether the target frame should be scrolled on main thread regardless
  // other subframes on the same page.
  for (Frame* frame = frame_; frame; frame = frame->Tree().Parent()) {
    if (!frame->IsLocalFrame())
      continue;
    reasons |=
        ToLocalFrame(frame)->View()->MainThreadScrollingReasonsPerFrame();
  }

  DCHECK(!MainThreadScrollingReason::HasNonCompositedScrollReasons(reasons));
  return reasons;
}

String LocalFrameView::MainThreadScrollingReasonsAsText() {
  MainThreadScrollingReasons reasons = main_thread_scrolling_reasons_;
  // TODO(pdr): We should also use the property tree main thread scrolling
  // reasons when RuntimeEnabledFeatures::BlinkGenPropertyTreesEnabled is true.
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
    DCHECK(Lifecycle().GetState() >= DocumentLifecycle::kPrePaintClean);

    // Slimming paint v2 stores main thread scrolling reasons on property
    // trees instead of in |main_thread_scrolling_reasons_|.
    if (const auto* scroll =
            GetLayoutView()->FirstFragment().PaintProperties()->Scroll()) {
      reasons = scroll->GetMainThreadScrollingReasons();
    }
  } else {
    DCHECK(Lifecycle().GetState() >= DocumentLifecycle::kCompositingClean);
    if (GraphicsLayer* layer_for_scrolling =
            LayoutViewportScrollableArea()->LayerForScrolling()) {
      if (cc::Layer* cc_layer = layer_for_scrolling->CcLayer())
        reasons = cc_layer->main_thread_scrolling_reasons();
    }
  }

  return String(MainThreadScrollingReason::AsText(reasons).c_str());
}

IntRect LocalFrameView::RemoteViewportIntersection() {
  IntRect intersection(GetFrame().RemoteViewportIntersection());
  intersection.Move(ScrollOffsetInt());
  return intersection;
}

void LocalFrameView::MapQuadToAncestorFrameIncludingScrollOffset(
    LayoutRect& rect,
    const LayoutObject* descendant,
    const LayoutView* ancestor,
    MapCoordinatesFlags mode) {
  FloatQuad mapped_quad = descendant->LocalToAncestorQuad(
      FloatQuad(FloatRect(rect)), ancestor, mode);
  rect = LayoutRect(mapped_quad.BoundingBox());

  // localToAncestorQuad accounts for scroll offset if it encounters a remote
  // frame in the ancestor chain, otherwise it needs to be added explicitly.
  if (GetFrame().LocalFrameRoot() == GetFrame().Tree().Top() ||
      (ancestor &&
       ancestor->GetFrame()->LocalFrameRoot() == GetFrame().LocalFrameRoot())) {
    LocalFrameView* ancestor_view =
        (ancestor ? ancestor->GetFrameView()
                  : ToLocalFrame(GetFrame().Tree().Top()).View());
    LayoutSize scroll_position = LayoutSize(ancestor_view->GetScrollOffset());
    rect.Move(-scroll_position);
  }
}

bool LocalFrameView::MapToVisualRectInTopFrameSpace(LayoutRect& rect) {
  // This is the top-level frame, so no mapping necessary.
  if (frame_->IsMainFrame())
    return true;

  LayoutRect viewport_intersection_rect(RemoteViewportIntersection());
  rect.Intersect(viewport_intersection_rect);
  if (rect.IsEmpty())
    return false;
  return true;
}

void LocalFrameView::ApplyTransformForTopFrameSpace(
    TransformState& transform_state) {
  // This is the top-level frame, so no mapping necessary.
  if (frame_->IsMainFrame())
    return;

  LayoutRect viewport_intersection_rect(RemoteViewportIntersection());
  transform_state.Move(LayoutSize(-viewport_intersection_rect.X(),
                                  -viewport_intersection_rect.Y()));
}

LayoutUnit LocalFrameView::CaretWidth() const {
  return LayoutUnit(
      std::max<float>(1.0, GetChromeClient()->WindowToViewportScalar(1)));
}

ScrollbarTheme& LocalFrameView::GetPageScrollbarTheme() const {
  Page* page = frame_->GetPage();
  DCHECK(page);

  return page->GetScrollbarTheme();
}

UkmTimeAggregator& LocalFrameView::EnsureUkmTimeAggregator() {
  if (!ukm_time_aggregator_) {
    ukm_time_aggregator_.reset(new UkmTimeAggregator(
        "Blink.UpdateTime", frame_->GetDocument()->UkmSourceID(),
        frame_->GetDocument()->UkmRecorder(),
        // Note that changing the order or values of the following vector
        // requires changing the UkmMetricNames enum.
        {"Compositing", "IntersectionObservation", "Paint", "PrePaint",
         "StyleAndLayout"},
        TimeDelta::FromSeconds(30)));
  }
  return *ukm_time_aggregator_;
}

}  // namespace blink
