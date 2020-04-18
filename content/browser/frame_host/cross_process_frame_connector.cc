// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/cross_process_frame_connector.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/surfaces/surface.h"
#include "components/viz/service/surfaces/surface_hittest.h"
#include "content/browser/compositor/surface_utils.h"
#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/render_frame_host_delegate.h"
#include "content/browser/frame_host/render_frame_host_manager.h"
#include "content/browser/frame_host/render_frame_proxy_host.h"
#include "content/browser/renderer_host/cursor_manager.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_input_event_router.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/renderer_host/render_widget_host_view_child_frame.h"
#include "content/common/frame_messages.h"
#include "content/public/common/screen_info.h"
#include "content/public/common/use_zoom_for_dsf_policy.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches_util.h"
#include "ui/gfx/geometry/dip_util.h"

namespace content {

CrossProcessFrameConnector::CrossProcessFrameConnector(
    RenderFrameProxyHost* frame_proxy_in_parent_renderer)
    : FrameConnectorDelegate(IsUseZoomForDSFEnabled()),
      frame_proxy_in_parent_renderer_(frame_proxy_in_parent_renderer),
      is_scroll_bubbling_(false) {
  frame_proxy_in_parent_renderer->frame_tree_node()
      ->render_manager()
      ->current_frame_host()
      ->GetRenderWidgetHost()
      ->GetScreenInfo(&screen_info_);
}

CrossProcessFrameConnector::~CrossProcessFrameConnector() {
  if (!IsVisible()) {
    // MaybeLogCrash will check 1) if there was a crash or not and 2) if the
    // crash might have been already logged earlier as kCrashedWhileVisible or
    // kShownAfterCrashing.
    MaybeLogCrash(CrashVisibility::kNeverVisibleAfterCrash);
  }

  // Notify the view of this object being destroyed, if the view still exists.
  SetView(nullptr);
}

bool CrossProcessFrameConnector::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;

  IPC_BEGIN_MESSAGE_MAP(CrossProcessFrameConnector, msg)
    IPC_MESSAGE_HANDLER(FrameHostMsg_SynchronizeVisualProperties,
                        OnSynchronizeVisualProperties)
    IPC_MESSAGE_HANDLER(FrameHostMsg_UpdateViewportIntersection,
                        OnUpdateViewportIntersection)
    IPC_MESSAGE_HANDLER(FrameHostMsg_VisibilityChanged, OnVisibilityChanged)
    IPC_MESSAGE_HANDLER(FrameHostMsg_SetIsInert, OnSetIsInert)
    IPC_MESSAGE_HANDLER(FrameHostMsg_SetInheritedEffectiveTouchAction,
                        OnSetInheritedEffectiveTouchAction)
    IPC_MESSAGE_HANDLER(FrameHostMsg_UpdateRenderThrottlingStatus,
                        OnUpdateRenderThrottlingStatus)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void CrossProcessFrameConnector::SetView(RenderWidgetHostViewChildFrame* view) {
  // Detach ourselves from the previous |view_|.
  if (view_) {
    RenderWidgetHostViewBase* root_view = GetRootRenderWidgetHostView();
    if (root_view && root_view->GetCursorManager())
      root_view->GetCursorManager()->ViewBeingDestroyed(view_);

    // The RenderWidgetHostDelegate needs to be checked because SetView() can
    // be called during nested WebContents destruction. See
    // https://crbug.com/644306.
    if (is_scroll_bubbling_ && GetParentRenderWidgetHostView() &&
        GetParentRenderWidgetHostView()->host()->delegate()) {
      GetParentRenderWidgetHostView()
          ->host()
          ->delegate()
          ->GetInputEventRouter()
          ->CancelScrollBubbling(view_);
      is_scroll_bubbling_ = false;
    }
    view_->SetFrameConnectorDelegate(nullptr);
  }

  ResetScreenSpaceRect();
  view_ = view;

  // Attach ourselves to the new view and size it appropriately. Also update
  // visibility in case the frame owner is hidden in parent process. We should
  // try to move these updates to a single IPC (see https://crbug.com/750179).
  if (view_) {
    if (has_crashed_ && !IsVisible()) {
      // MaybeLogCrash will check 1) if there was a crash or not and 2) if the
      // crash might have been already logged earlier as kCrashedWhileVisible or
      // kShownAfterCrashing.
      MaybeLogCrash(CrashVisibility::kNeverVisibleAfterCrash);
    }
    is_crash_already_logged_ = has_crashed_ = false;

    view_->SetFrameConnectorDelegate(this);
    if (is_hidden_)
      OnVisibilityChanged(false);
    FrameMsg_ViewChanged_Params params;
    if (!base::FeatureList::IsEnabled(::features::kMash))
      params.frame_sink_id = view_->GetFrameSinkId();
    frame_proxy_in_parent_renderer_->Send(new FrameMsg_ViewChanged(
        frame_proxy_in_parent_renderer_->GetRoutingID(), params));
  }
}

void CrossProcessFrameConnector::RenderProcessGone() {
  has_crashed_ = true;

  FrameTreeNode* node = frame_proxy_in_parent_renderer_->frame_tree_node();
  int process_id = node->current_frame_host()->GetProcess()->GetID();
  for (node = node->parent(); node; node = node->parent()) {
    if (node->current_frame_host()->GetProcess()->GetID() == process_id) {
      // The crash will be already logged by the ancestor - ignore this crash in
      // the current instance of the CrossProcessFrameConnector.
      is_crash_already_logged_ = true;
    }
  }

  if (IsVisible())
    MaybeLogCrash(CrashVisibility::kCrashedWhileVisible);

  frame_proxy_in_parent_renderer_->Send(new FrameMsg_ChildFrameProcessGone(
      frame_proxy_in_parent_renderer_->GetRoutingID()));
}

void CrossProcessFrameConnector::SetChildFrameSurface(
    const viz::SurfaceInfo& surface_info) {
  frame_proxy_in_parent_renderer_->Send(new FrameMsg_SetChildFrameSurface(
      frame_proxy_in_parent_renderer_->GetRoutingID(), surface_info));
}

void CrossProcessFrameConnector::SendIntrinsicSizingInfoToParent(
    const blink::WebIntrinsicSizingInfo& sizing_info) {
  frame_proxy_in_parent_renderer_->Send(
      new FrameMsg_IntrinsicSizingInfoOfChildChanged(
          frame_proxy_in_parent_renderer_->GetRoutingID(), sizing_info));
}

void CrossProcessFrameConnector::UpdateCursor(const WebCursor& cursor) {
  RenderWidgetHostViewBase* root_view = GetRootRenderWidgetHostView();
  // UpdateCursor messages are ignored if the root view does not support
  // cursors.
  if (root_view && root_view->GetCursorManager())
    root_view->GetCursorManager()->UpdateCursor(view_, cursor);
}

gfx::PointF CrossProcessFrameConnector::TransformPointToRootCoordSpace(
    const gfx::PointF& point,
    const viz::SurfaceId& surface_id) {
  gfx::PointF transformed_point;
  TransformPointToCoordSpaceForView(point, GetRootRenderWidgetHostView(),
                                    surface_id, &transformed_point);
  return transformed_point;
}

bool CrossProcessFrameConnector::TransformPointToLocalCoordSpaceLegacy(
    const gfx::PointF& point,
    const viz::SurfaceId& original_surface,
    const viz::SurfaceId& local_surface_id,
    gfx::PointF* transformed_point) {
  if (original_surface == local_surface_id) {
    *transformed_point = point;
    return true;
  }

  // Transformations use physical pixels rather than DIP, so conversion
  // is necessary.
  *transformed_point =
      gfx::ConvertPointToPixel(view_->current_surface_scale_factor(), point);
  viz::SurfaceHittest hittest(nullptr,
                              GetFrameSinkManager()->surface_manager());
  if (!hittest.TransformPointToTargetSurface(original_surface, local_surface_id,
                                             transformed_point))
    return false;

  *transformed_point = gfx::ConvertPointToDIP(
      view_->current_surface_scale_factor(), *transformed_point);
  return true;
}

bool CrossProcessFrameConnector::TransformPointToCoordSpaceForView(
    const gfx::PointF& point,
    RenderWidgetHostViewBase* target_view,
    const viz::SurfaceId& local_surface_id,
    gfx::PointF* transformed_point,
    viz::EventSource source) {
  RenderWidgetHostViewBase* root_view = GetRootRenderWidgetHostView();
  if (!root_view)
    return false;

  // It is possible that neither the original surface or target surface is an
  // ancestor of the other in the RenderWidgetHostView tree (e.g. they could
  // be siblings). To account for this, the point is first transformed into the
  // root coordinate space and then the root is asked to perform the conversion.
  if (!root_view->TransformPointToLocalCoordSpace(point, local_surface_id,
                                                  transformed_point, source))
    return false;

  if (target_view == root_view)
    return true;

  return root_view->TransformPointToCoordSpaceForView(
      *transformed_point, target_view, transformed_point, source);
}

void CrossProcessFrameConnector::ForwardProcessAckedTouchEvent(
    const TouchEventWithLatencyInfo& touch,
    InputEventAckState ack_result) {
  auto* main_view = GetRootRenderWidgetHostView();
  if (main_view)
    main_view->ProcessAckedTouchEvent(touch, ack_result);
}

void CrossProcessFrameConnector::BubbleScrollEvent(
    const blink::WebGestureEvent& event) {
  DCHECK((view_->wheel_scroll_latching_enabled() &&
          event.GetType() == blink::WebInputEvent::kGestureScrollBegin) ||
         event.GetType() == blink::WebInputEvent::kGestureScrollUpdate ||
         event.GetType() == blink::WebInputEvent::kGestureScrollEnd ||
         event.GetType() == blink::WebInputEvent::kGestureFlingStart);
  auto* parent_view = GetParentRenderWidgetHostView();

  if (!parent_view)
    return;

  auto* event_router = parent_view->host()->delegate()->GetInputEventRouter();

  // We will only convert the coordinates back to the root here. The
  // RenderWidgetHostInputEventRouter will determine which ancestor view will
  // receive a resent gesture event, so it will be responsible for converting to
  // the coordinates of the target view.
  blink::WebGestureEvent resent_gesture_event(event);
  const gfx::PointF root_point =
      view_->TransformPointToRootCoordSpaceF(event.PositionInWidget());
  resent_gesture_event.SetPositionInWidget(root_point);

  if (view_->wheel_scroll_latching_enabled()) {
    if (event.GetType() == blink::WebInputEvent::kGestureScrollBegin) {
      event_router->BubbleScrollEvent(parent_view, resent_gesture_event, view_);
      is_scroll_bubbling_ = true;
    } else if (is_scroll_bubbling_) {
      event_router->BubbleScrollEvent(parent_view, resent_gesture_event, view_);
    }
    if (event.GetType() == blink::WebInputEvent::kGestureScrollEnd ||
        event.GetType() == blink::WebInputEvent::kGestureFlingStart) {
      is_scroll_bubbling_ = false;
    }
  } else {  // !view_->wheel_scroll_latching_enabled()
    if (event.GetType() == blink::WebInputEvent::kGestureScrollUpdate) {
      event_router->BubbleScrollEvent(parent_view, resent_gesture_event, view_);
      is_scroll_bubbling_ = true;
    } else if ((event.GetType() == blink::WebInputEvent::kGestureScrollEnd ||
                event.GetType() == blink::WebInputEvent::kGestureFlingStart) &&
               is_scroll_bubbling_) {
      event_router->BubbleScrollEvent(parent_view, resent_gesture_event, view_);
      is_scroll_bubbling_ = false;
    }
  }
}

bool CrossProcessFrameConnector::HasFocus() {
  RenderWidgetHostViewBase* root_view = GetRootRenderWidgetHostView();
  if (root_view)
    return root_view->HasFocus();
  return false;
}

void CrossProcessFrameConnector::FocusRootView() {
  RenderWidgetHostViewBase* root_view = GetRootRenderWidgetHostView();
  if (root_view)
    root_view->Focus();
}

bool CrossProcessFrameConnector::LockMouse() {
  RenderWidgetHostViewBase* root_view = GetRootRenderWidgetHostView();
  if (root_view)
    return root_view->LockMouse();
  return false;
}

void CrossProcessFrameConnector::UnlockMouse() {
  RenderWidgetHostViewBase* root_view = GetRootRenderWidgetHostView();
  if (root_view)
    root_view->UnlockMouse();
}

void CrossProcessFrameConnector::OnSynchronizeVisualProperties(
    const viz::SurfaceId& surface_id,
    const FrameVisualProperties& visual_properties) {
  // If the |screen_space_rect| or |screen_info| of the frame has changed, then
  // the viz::LocalSurfaceId must also change.
  if ((last_received_local_frame_size_ != visual_properties.local_frame_size ||
       screen_info_ != visual_properties.screen_info ||
       capture_sequence_number() !=
           visual_properties.capture_sequence_number) &&
      local_surface_id_ == surface_id.local_surface_id()) {
    bad_message::ReceivedBadMessage(
        frame_proxy_in_parent_renderer_->GetProcess(),
        bad_message::CPFC_RESIZE_PARAMS_CHANGED_LOCAL_SURFACE_ID_UNCHANGED);
    return;
  }

  last_received_local_frame_size_ = visual_properties.local_frame_size;
  SynchronizeVisualProperties(surface_id, visual_properties);
}

void CrossProcessFrameConnector::OnUpdateViewportIntersection(
    const gfx::Rect& viewport_intersection,
    const gfx::Rect& compositor_visible_rect) {
  viewport_intersection_rect_ = viewport_intersection;
  compositor_visible_rect_ = compositor_visible_rect;
  if (view_)
    view_->UpdateViewportIntersection(viewport_intersection,
                                      compositor_visible_rect);

  if (IsVisible()) {
    // MaybeLogCrash will check 1) if there was a crash or not and 2) if the
    // crash might have been already logged earlier as kCrashedWhileVisible or
    // kShownAfterCrashing.
    MaybeLogCrash(CrashVisibility::kShownAfterCrashing);
  }
}

void CrossProcessFrameConnector::OnVisibilityChanged(bool visible) {
  is_hidden_ = !visible;
  if (IsVisible()) {
    // MaybeLogCrash will check 1) if there was a crash or not and 2) if the
    // crash might have been already logged earlier as kCrashedWhileVisible or
    // kShownAfterCrashing.
    MaybeLogCrash(CrashVisibility::kShownAfterCrashing);
  }
  if (!view_)
    return;

  // If there is an inner WebContents, it should be notified of the change in
  // the visibility. The Show/Hide methods will not be called if an inner
  // WebContents exists since the corresponding WebContents will itself call
  // Show/Hide on all the RenderWidgetHostViews (including this) one.
  if (frame_proxy_in_parent_renderer_->frame_tree_node()
          ->render_manager()
          ->ForInnerDelegate()) {
    view_->host()->delegate()->OnRenderFrameProxyVisibilityChanged(visible);
    return;
  }

  if (visible && !view_->host()->delegate()->IsHidden()) {
    view_->Show();
  } else if (!visible) {
    view_->Hide();
  }
}

void CrossProcessFrameConnector::OnSetIsInert(bool inert) {
  is_inert_ = inert;
  if (view_)
    view_->SetIsInert();
}

void CrossProcessFrameConnector::OnSetInheritedEffectiveTouchAction(
    cc::TouchAction touch_action) {
  inherited_effective_touch_action_ = touch_action;
  if (view_)
    view_->UpdateInheritedEffectiveTouchAction();
}

RenderWidgetHostViewBase*
CrossProcessFrameConnector::GetRootRenderWidgetHostView() {
  // Tests may not have frame_proxy_in_parent_renderer_ set.
  if (!frame_proxy_in_parent_renderer_)
    return nullptr;

  RenderFrameHostImpl* top_host = frame_proxy_in_parent_renderer_->
      frame_tree_node()->frame_tree()->root()->current_frame_host();

  // This method should return the root RWHV from the top-level WebContents,
  // in the case of nested WebContents.
  while (top_host->frame_tree_node()->render_manager()->ForInnerDelegate()) {
    top_host = top_host->frame_tree_node()->render_manager()->
        GetOuterDelegateNode()->frame_tree()->root()->current_frame_host();
  }

  return static_cast<RenderWidgetHostViewBase*>(top_host->GetView());
}

RenderWidgetHostViewBase*
CrossProcessFrameConnector::GetParentRenderWidgetHostView() {
  FrameTreeNode* parent =
      frame_proxy_in_parent_renderer_->frame_tree_node()->parent();

  if (!parent &&
      frame_proxy_in_parent_renderer_->frame_tree_node()
          ->render_manager()
          ->GetOuterDelegateNode()) {
    parent = frame_proxy_in_parent_renderer_->frame_tree_node()
                 ->render_manager()
                 ->GetOuterDelegateNode()
                 ->parent();
  }

  if (parent) {
    return static_cast<RenderWidgetHostViewBase*>(
        parent->current_frame_host()->GetView());
  }

  return nullptr;
}

void CrossProcessFrameConnector::EnableAutoResize(const gfx::Size& min_size,
                                                  const gfx::Size& max_size) {
  frame_proxy_in_parent_renderer_->Send(new FrameMsg_EnableAutoResize(
      frame_proxy_in_parent_renderer_->GetRoutingID(), min_size, max_size));
}

void CrossProcessFrameConnector::DisableAutoResize() {
  frame_proxy_in_parent_renderer_->Send(new FrameMsg_DisableAutoResize(
      frame_proxy_in_parent_renderer_->GetRoutingID()));
}

bool CrossProcessFrameConnector::IsInert() const {
  return is_inert_;
}

cc::TouchAction CrossProcessFrameConnector::InheritedEffectiveTouchAction()
    const {
  return inherited_effective_touch_action_;
}

bool CrossProcessFrameConnector::IsHidden() const {
  return is_hidden_;
}

#if defined(USE_AURA)
void CrossProcessFrameConnector::EmbedRendererWindowTreeClientInParent(
    ui::mojom::WindowTreeClientPtr window_tree_client) {
  RenderWidgetHostViewBase* root = GetRootRenderWidgetHostView();
  RenderWidgetHostViewBase* parent = GetParentRenderWidgetHostView();
  if (!parent || !root)
    return;
  const int frame_routing_id = frame_proxy_in_parent_renderer_->GetRoutingID();
  parent->EmbedChildFrameRendererWindowTreeClient(
      root, frame_routing_id, std::move(window_tree_client));
  frame_proxy_in_parent_renderer_->SetDestructionCallback(
      base::BindOnce(&RenderWidgetHostViewBase::OnChildFrameDestroyed,
                     parent->GetWeakPtr(), frame_routing_id));
}
#endif

void CrossProcessFrameConnector::DidUpdateVisualProperties(
    const cc::RenderFrameMetadata& metadata) {
  frame_proxy_in_parent_renderer_->Send(new FrameMsg_DidUpdateVisualProperties(
      frame_proxy_in_parent_renderer_->GetRoutingID(), metadata));
}

void CrossProcessFrameConnector::SetVisibilityForChildViews(
    bool visible) const {
  frame_proxy_in_parent_renderer_->frame_tree_node()
      ->current_frame_host()
      ->SetVisibilityForChildViews(visible);
}

void CrossProcessFrameConnector::SetScreenSpaceRect(
    const gfx::Rect& screen_space_rect) {
  gfx::Rect old_rect = screen_space_rect;
  FrameConnectorDelegate::SetScreenSpaceRect(screen_space_rect);

  if (view_) {
    view_->SetBounds(screen_space_rect_in_dip_);

    // Other local root frames nested underneath this one implicitly have their
    // view rects changed when their ancestor is repositioned, and therefore
    // need to have their screen rects updated.
    FrameTreeNode* proxy_node =
        frame_proxy_in_parent_renderer_->frame_tree_node();
    if (old_rect.x() != screen_space_rect_in_pixels_.x() ||
        old_rect.y() != screen_space_rect_in_pixels_.y()) {
      for (FrameTreeNode* node :
           proxy_node->frame_tree()->SubtreeNodes(proxy_node)) {
        if (node != proxy_node && node->current_frame_host()->is_local_root())
          node->current_frame_host()->GetRenderWidgetHost()->SendScreenRects();
      }
    }
  }
}

void CrossProcessFrameConnector::ResetScreenSpaceRect() {
  local_surface_id_ = viz::LocalSurfaceId();
  // TODO(lfg): Why do we need to reset the screen_space_rect_ that comes from
  // the parent when setting the child? https://crbug.com/809275
  screen_space_rect_in_pixels_ = gfx::Rect();
  screen_space_rect_in_dip_ = gfx::Rect();
  last_received_local_frame_size_ = gfx::Size();
}

void CrossProcessFrameConnector::OnUpdateRenderThrottlingStatus(
    bool is_throttled,
    bool subtree_throttled) {
  if (is_throttled != is_throttled_ ||
      subtree_throttled != subtree_throttled_) {
    is_throttled_ = is_throttled;
    subtree_throttled_ = subtree_throttled;
    if (view_)
      view_->UpdateRenderThrottlingStatus();
  }
}

bool CrossProcessFrameConnector::IsThrottled() const {
  return is_throttled_;
}

bool CrossProcessFrameConnector::IsSubtreeThrottled() const {
  return subtree_throttled_;
}

void CrossProcessFrameConnector::MaybeLogCrash(CrashVisibility visibility) {
  if (!has_crashed_)
    return;

  // Only log once per renderer crash.
  if (is_crash_already_logged_)
    return;
  is_crash_already_logged_ = true;

  // Actually log the UMA.
  UMA_HISTOGRAM_ENUMERATION("Stability.ChildFrameCrash.Visibility", visibility);
}

bool CrossProcessFrameConnector::IsVisible() {
  if (is_hidden_)
    return false;
  if (viewport_intersection_rect().IsEmpty())
    return false;

  Visibility embedder_visibility =
      frame_proxy_in_parent_renderer_->frame_tree_node()
          ->current_frame_host()
          ->delegate()
          ->GetVisibility();
  if (embedder_visibility != Visibility::VISIBLE)
    return false;

  return true;
}

}  // namespace content
