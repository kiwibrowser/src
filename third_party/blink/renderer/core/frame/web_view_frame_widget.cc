// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#include "third_party/blink/renderer/core/frame/web_view_frame_widget.h"

#include "third_party/blink/public/mojom/page/page_visibility_state.mojom-blink.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/platform/exported/web_active_gesture_animation.h"

namespace blink {

WebViewFrameWidget::WebViewFrameWidget(WebWidgetClient& client,
                                       WebViewImpl& web_view)
    : WebFrameWidgetBase(client),
      web_view_(&web_view),
      self_keep_alive_(this) {}

WebViewFrameWidget::~WebViewFrameWidget() = default;

void WebViewFrameWidget::Close() {
  // Note: it's important to use the captured main frame pointer here. During
  // a frame swap, the swapped frame is detached *after* the frame tree is
  // updated. If the main frame is being swapped, then
  // m_webView()->mainFrameImpl() will no longer point to the original frame.
  web_view_->SetCompositorVisibility(false);
  web_view_ = nullptr;

  WebFrameWidgetBase::Close();

  // Note: this intentionally does not forward to WebView::close(), to make it
  // easier to untangle the cleanup logic later.
  self_keep_alive_.Clear();
}

WebSize WebViewFrameWidget::Size() {
  return web_view_->Size();
}

void WebViewFrameWidget::Resize(const WebSize& size) {
  return web_view_->Resize(size);
}

void WebViewFrameWidget::ResizeVisualViewport(const WebSize& size) {
  return web_view_->ResizeVisualViewport(size);
}

void WebViewFrameWidget::DidEnterFullscreen() {
  return web_view_->DidEnterFullscreen();
}

void WebViewFrameWidget::DidExitFullscreen() {
  return web_view_->DidExitFullscreen();
}

void WebViewFrameWidget::SetSuppressFrameRequestsWorkaroundFor704763Only(
    bool suppress_frame_requests) {
  return web_view_->SetSuppressFrameRequestsWorkaroundFor704763Only(
      suppress_frame_requests);
}
void WebViewFrameWidget::BeginFrame(base::TimeTicks last_frame_time) {
  return web_view_->BeginFrame(last_frame_time);
}

void WebViewFrameWidget::UpdateLifecycle(LifecycleUpdate requested_update) {
  return web_view_->UpdateLifecycle(requested_update);
}

void WebViewFrameWidget::UpdateAllLifecyclePhasesAndCompositeForTesting() {
  web_view_->UpdateAllLifecyclePhasesAndCompositeForTesting();
}

void WebViewFrameWidget::Paint(WebCanvas* canvas, const WebRect& view_port) {
  return web_view_->Paint(canvas, view_port);
}

void WebViewFrameWidget::LayoutAndPaintAsync(base::OnceClosure callback) {
  return web_view_->LayoutAndPaintAsync(std::move(callback));
}

void WebViewFrameWidget::CompositeAndReadbackAsync(
    base::OnceCallback<void(const SkBitmap&)> callback) {
  return web_view_->CompositeAndReadbackAsync(std::move(callback));
}

void WebViewFrameWidget::ThemeChanged() {
  return web_view_->ThemeChanged();
}

WebInputEventResult WebViewFrameWidget::HandleInputEvent(
    const WebCoalescedInputEvent& event) {
  return web_view_->HandleInputEvent(event);
}

WebInputEventResult WebViewFrameWidget::DispatchBufferedTouchEvents() {
  return web_view_->DispatchBufferedTouchEvents();
}

void WebViewFrameWidget::SetCursorVisibilityState(bool is_visible) {
  return web_view_->SetCursorVisibilityState(is_visible);
}

void WebViewFrameWidget::ApplyViewportDeltas(
    const WebFloatSize& visual_viewport_delta,
    const WebFloatSize& layout_viewport_delta,
    const WebFloatSize& elastic_overscroll_delta,
    float scale_factor,
    float browser_controls_shown_ratio_delta) {
  return web_view_->ApplyViewportDeltas(
      visual_viewport_delta, layout_viewport_delta, elastic_overscroll_delta,
      scale_factor, browser_controls_shown_ratio_delta);
}

void WebViewFrameWidget::RecordWheelAndTouchScrollingCount(
    bool has_scrolled_by_wheel,
    bool has_scrolled_by_touch) {
  return web_view_->RecordWheelAndTouchScrollingCount(has_scrolled_by_wheel,
                                                      has_scrolled_by_touch);
}

void WebViewFrameWidget::MouseCaptureLost() {
  return web_view_->MouseCaptureLost();
}

void WebViewFrameWidget::SetFocus(bool enable) {
  return web_view_->SetFocus(enable);
}

bool WebViewFrameWidget::SelectionBounds(WebRect& anchor,
                                         WebRect& focus) const {
  return web_view_->SelectionBounds(anchor, focus);
}

bool WebViewFrameWidget::IsAcceleratedCompositingActive() const {
  return web_view_->IsAcceleratedCompositingActive();
}

void WebViewFrameWidget::WillCloseLayerTreeView() {
  return web_view_->WillCloseLayerTreeView();
}

SkColor WebViewFrameWidget::BackgroundColor() const {
  return web_view_->BackgroundColor();
}

WebPagePopup* WebViewFrameWidget::GetPagePopup() const {
  return web_view_->GetPagePopup();
}

void WebViewFrameWidget::UpdateBrowserControlsState(
    WebBrowserControlsState constraints,
    WebBrowserControlsState current,
    bool animate) {
  return web_view_->UpdateBrowserControlsState(constraints, current, animate);
}

void WebViewFrameWidget::SetVisibilityState(
    mojom::PageVisibilityState visibility_state) {
  return web_view_->SetVisibilityState(visibility_state, false);
}

void WebViewFrameWidget::SetBackgroundColorOverride(SkColor color) {
  web_view_->SetBackgroundColorOverride(color);
}

void WebViewFrameWidget::ClearBackgroundColorOverride() {
  return web_view_->ClearBackgroundColorOverride();
}

void WebViewFrameWidget::SetBaseBackgroundColorOverride(SkColor color) {
  web_view_->SetBaseBackgroundColorOverride(color);
}

void WebViewFrameWidget::ClearBaseBackgroundColorOverride() {
  return web_view_->ClearBaseBackgroundColorOverride();
}

void WebViewFrameWidget::SetBaseBackgroundColor(SkColor color) {
  web_view_->SetBaseBackgroundColor(color);
}

WebInputMethodController*
WebViewFrameWidget::GetActiveWebInputMethodController() const {
  return web_view_->GetActiveWebInputMethodController();
}

bool WebViewFrameWidget::ScrollFocusedEditableElementIntoView() {
  return web_view_->ScrollFocusedEditableElementIntoView();
}

void WebViewFrameWidget::Initialize() {
  web_view_->SetCompositorVisibility(true);
}

void WebViewFrameWidget::ScheduleAnimation() {
  web_view_->ScheduleAnimationForWidget();
}

base::WeakPtr<CompositorMutatorImpl>
WebViewFrameWidget::EnsureCompositorMutator(
    scoped_refptr<base::SingleThreadTaskRunner>* mutator_task_runner) {
  return web_view_->EnsureCompositorMutator(mutator_task_runner);
}

void WebViewFrameWidget::SetRootGraphicsLayer(GraphicsLayer* layer) {
  web_view_->SetRootGraphicsLayer(layer);
}

GraphicsLayer* WebViewFrameWidget::RootGraphicsLayer() const {
  return web_view_->RootGraphicsLayer();
}

void WebViewFrameWidget::SetRootLayer(scoped_refptr<cc::Layer> layer) {
  web_view_->SetRootLayer(layer);
}

WebLayerTreeView* WebViewFrameWidget::GetLayerTreeView() const {
  return web_view_->LayerTreeView();
}

CompositorAnimationHost* WebViewFrameWidget::AnimationHost() const {
  return web_view_->AnimationHost();
}

WebHitTestResult WebViewFrameWidget::HitTestResultAt(const WebPoint& point) {
  return web_view_->HitTestResultAt(point);
}

HitTestResult WebViewFrameWidget::CoreHitTestResultAt(const WebPoint& point) {
  return web_view_->CoreHitTestResultAt(point);
}

void WebViewFrameWidget::Trace(blink::Visitor* visitor) {
  WebFrameWidgetBase::Trace(visitor);
}

PageWidgetEventHandler* WebViewFrameWidget::GetPageWidgetEventHandler() {
  return web_view_.get();
}

}  // namespace blink
