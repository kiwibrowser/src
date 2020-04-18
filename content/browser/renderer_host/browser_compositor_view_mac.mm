// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/browser_compositor_view_mac.h"

#import <Cocoa/Cocoa.h>
#include <stdint.h>
#include <utility>

#include "base/command_line.h"
#include "base/containers/circular_deque.h"
#include "base/lazy_instance.h"
#include "base/trace_event/trace_event.h"
#include "components/viz/common/features.h"
#include "content/browser/compositor/image_transport_factory.h"
#include "content/browser/renderer_host/display_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/context_factory.h"
#include "ui/accelerated_widget_mac/accelerated_widget_mac.h"
#include "ui/accelerated_widget_mac/window_resize_helper_mac.h"
#include "ui/base/layout.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/dip_util.h"

namespace content {

namespace {

// Weak pointers to all BrowserCompositorMac instances, used to
// - Determine if a spare RecyclableCompositorMac should be kept around (one
//   should be only if there exists at least one BrowserCompositorMac).
// - Force all ui::Compositors to be destroyed at shut-down (because the NSView
//   signals to shut down will come in very late, long after things that the
//   ui::Compositor depend on have been destroyed).
//   https://crbug.com/805726
base::LazyInstance<std::set<BrowserCompositorMac*>>::Leaky
    g_browser_compositors;

// A spare RecyclableCompositorMac kept around for recycling.
base::LazyInstance<base::circular_deque<
    std::unique_ptr<RecyclableCompositorMac>>>::DestructorAtExit
    g_spare_recyclable_compositors;

void ReleaseSpareCompositors() {
  // Allow at most one spare recyclable compositor.
  while (g_spare_recyclable_compositors.Get().size() > 1)
    g_spare_recyclable_compositors.Get().pop_front();

  if (g_browser_compositors.Get().empty())
    g_spare_recyclable_compositors.Get().clear();
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// RecyclableCompositorMac

// A ui::Compositor and a gfx::AcceleratedWidget (and helper) that it draws
// into. This structure is used to efficiently recycle these structures across
// tabs (because creating a new ui::Compositor for each tab would be expensive
// in terms of time and resources).
class RecyclableCompositorMac : public ui::CompositorObserver {
 public:
  ~RecyclableCompositorMac() override;

  // Create a compositor, or recycle a preexisting one.
  static std::unique_ptr<RecyclableCompositorMac> Create();

  // Delete a compositor, or allow it to be recycled.
  static void Recycle(std::unique_ptr<RecyclableCompositorMac> compositor);

  ui::Compositor* compositor() { return &compositor_; }
  ui::AcceleratedWidgetMac* accelerated_widget_mac() {
    return accelerated_widget_mac_.get();
  }
  const gfx::Size pixel_size() const { return size_pixels_; }
  float scale_factor() const { return scale_factor_; }

  // Suspend will prevent the compositor from producing new frames. This should
  // be called to avoid creating spurious frames while changing state.
  // Compositors are created as suspended.
  void Suspend();
  void Unsuspend();

  // Update the compositor's surface information, if needed.
  void UpdateSurface(const gfx::Size& size_pixels, float scale_factor);
  // Invalidate the compositor's surface information.
  void InvalidateSurface();

  // The viz::ParentLocalSurfaceIdAllocator for the ui::Compositor dispenses
  // viz::LocalSurfaceIds that are renderered into by the ui::Compositor.
  viz::ParentLocalSurfaceIdAllocator local_surface_id_allocator_;
  gfx::Size size_pixels_;
  float scale_factor_ = 1.f;

 private:
  RecyclableCompositorMac();

  // ui::CompositorObserver implementation:
  void OnCompositingDidCommit(ui::Compositor* compositor) override;
  void OnCompositingStarted(ui::Compositor* compositor,
                            base::TimeTicks start_time) override {}
  void OnCompositingEnded(ui::Compositor* compositor) override {}
  void OnCompositingLockStateChanged(ui::Compositor* compositor) override {}
  void OnCompositingChildResizing(ui::Compositor* compositor) override {}
  void OnCompositingShuttingDown(ui::Compositor* compositor) override {}

  std::unique_ptr<ui::AcceleratedWidgetMac> accelerated_widget_mac_;
  ui::Compositor compositor_;
  std::unique_ptr<ui::CompositorLock> compositor_suspended_lock_;

  DISALLOW_COPY_AND_ASSIGN(RecyclableCompositorMac);
};

RecyclableCompositorMac::RecyclableCompositorMac()
    : accelerated_widget_mac_(new ui::AcceleratedWidgetMac()),
      compositor_(content::GetContextFactoryPrivate()->AllocateFrameSinkId(),
                  content::GetContextFactory(),
                  content::GetContextFactoryPrivate(),
                  ui::WindowResizeHelperMac::Get()->task_runner(),
                  features::IsSurfaceSynchronizationEnabled(),
                  false /* enable_pixel_canvas */) {
  compositor_.SetAcceleratedWidget(
      accelerated_widget_mac_->accelerated_widget());
  Suspend();
  compositor_.AddObserver(this);
}

RecyclableCompositorMac::~RecyclableCompositorMac() {
  compositor_.RemoveObserver(this);
}

void RecyclableCompositorMac::Suspend() {
  // Requests a compositor lock without a timeout.
  compositor_suspended_lock_ =
      compositor_.GetCompositorLock(nullptr, base::TimeDelta());
}

void RecyclableCompositorMac::Unsuspend() {
  compositor_suspended_lock_ = nullptr;
}

void RecyclableCompositorMac::UpdateSurface(const gfx::Size& size_pixels,
                                            float scale_factor) {
  if (size_pixels != size_pixels_ || scale_factor != scale_factor_) {
    size_pixels_ = size_pixels;
    scale_factor_ = scale_factor;
    compositor()->SetScaleAndSize(scale_factor_, size_pixels_,
                                  local_surface_id_allocator_.GenerateId());
  }
}

void RecyclableCompositorMac::InvalidateSurface() {
  size_pixels_ = gfx::Size();
  scale_factor_ = 1.f;
  local_surface_id_allocator_.Invalidate();
  compositor()->SetScaleAndSize(
      scale_factor_, size_pixels_,
      local_surface_id_allocator_.GetCurrentLocalSurfaceId());
}

void RecyclableCompositorMac::OnCompositingDidCommit(
    ui::Compositor* compositor_that_did_commit) {
  DCHECK_EQ(compositor_that_did_commit, compositor());
  accelerated_widget_mac_->SetSuspended(false);
}

// static
std::unique_ptr<RecyclableCompositorMac> RecyclableCompositorMac::Create() {
  DCHECK(ui::WindowResizeHelperMac::Get()->task_runner());
  if (!g_spare_recyclable_compositors.Get().empty()) {
    std::unique_ptr<RecyclableCompositorMac> result;
    result = std::move(g_spare_recyclable_compositors.Get().back());
    g_spare_recyclable_compositors.Get().pop_back();
    return result;
  }
  return std::unique_ptr<RecyclableCompositorMac>(new RecyclableCompositorMac);
}

// static
void RecyclableCompositorMac::Recycle(
    std::unique_ptr<RecyclableCompositorMac> compositor) {
  compositor->accelerated_widget_mac_->SetSuspended(true);

  // Make this RecyclableCompositorMac recyclable for future instances.
  g_spare_recyclable_compositors.Get().push_back(std::move(compositor));

  // Post a task to free up the spare ui::Compositors when needed. Post this
  // to the browser main thread so that we won't free any compositors while
  // in a nested loop waiting to put up a new frame.
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE, base::Bind(&ReleaseSpareCompositors));
}

////////////////////////////////////////////////////////////////////////////////
// BrowserCompositorMac

BrowserCompositorMac::BrowserCompositorMac(
    ui::AcceleratedWidgetMacNSView* accelerated_widget_mac_ns_view,
    BrowserCompositorMacClient* client,
    bool render_widget_host_is_hidden,
    bool ns_view_attached_to_window,
    const display::Display& initial_display,
    const viz::FrameSinkId& frame_sink_id)
    : client_(client),
      accelerated_widget_mac_ns_view_(accelerated_widget_mac_ns_view),
      dfh_display_(initial_display),
      weak_factory_(this) {
  g_browser_compositors.Get().insert(this);

  root_layer_.reset(new ui::Layer(ui::LAYER_SOLID_COLOR));
  // Ensure that this layer draws nothing when it does not not have delegated
  // content (otherwise this solid color will be flashed during navigation).
  root_layer_->SetColor(SK_ColorTRANSPARENT);
  delegated_frame_host_.reset(new DelegatedFrameHost(
      frame_sink_id, this,
      base::FeatureList::IsEnabled(features::kVizDisplayCompositor),
      true /* should_register_frame_sink_id */));

  SetRenderWidgetHostIsHidden(render_widget_host_is_hidden);
  SetNSViewAttachedToWindow(ns_view_attached_to_window);
}

BrowserCompositorMac::~BrowserCompositorMac() {
  // Ensure that copy callbacks completed or cancelled during further tear-down
  // do not call back into this.
  weak_factory_.InvalidateWeakPtrs();

  TransitionToState(HasNoCompositor);
  delegated_frame_host_.reset();
  root_layer_.reset();

  size_t num_erased = g_browser_compositors.Get().erase(this);
  DCHECK_EQ(1u, num_erased);

  // If there are no compositors allocated, destroy the recyclable
  // RecyclableCompositorMac.
  if (g_browser_compositors.Get().empty())
    g_spare_recyclable_compositors.Get().clear();
}

gfx::AcceleratedWidget BrowserCompositorMac::GetAcceleratedWidget() {
  if (recyclable_compositor_) {
    return recyclable_compositor_->accelerated_widget_mac()
        ->accelerated_widget();
  }
  return gfx::kNullAcceleratedWidget;
}

DelegatedFrameHost* BrowserCompositorMac::GetDelegatedFrameHost() {
  DCHECK(delegated_frame_host_);
  return delegated_frame_host_.get();
}

void BrowserCompositorMac::ClearCompositorFrame() {
  // Make sure that we no longer hold a compositor lock by un-suspending the
  // compositor. This ensures that we are able to swap in a new blank frame to
  // replace any old content.
  // https://crbug.com/739621
  if (recyclable_compositor_)
    recyclable_compositor_->Unsuspend();
  if (delegated_frame_host_)
    delegated_frame_host_->ClearDelegatedFrame();
}

bool BrowserCompositorMac::RequestRepaintForTesting() {
  const viz::LocalSurfaceId& new_local_surface_id =
      dfh_local_surface_id_allocator_.GenerateId();
  delegated_frame_host_->SynchronizeVisualProperties(
      new_local_surface_id, dfh_size_dip_,
      cc::DeadlinePolicy::UseExistingDeadline());
  return client_->SynchronizeVisualProperties();
}

const gfx::CALayerParams* BrowserCompositorMac::GetLastCALayerParams() const {
  if (!recyclable_compositor_)
    return nullptr;
  return recyclable_compositor_->accelerated_widget_mac()->GetCALayerParams();
}

viz::FrameSinkId BrowserCompositorMac::GetRootFrameSinkId() {
  if (parent_ui_layer_)
    return parent_ui_layer_->GetCompositor()->frame_sink_id();
  if (recyclable_compositor_)
    return recyclable_compositor_->compositor()->frame_sink_id();
  return viz::FrameSinkId();
}

void BrowserCompositorMac::DidCreateNewRendererCompositorFrameSink(
    viz::mojom::CompositorFrameSinkClient* renderer_compositor_frame_sink) {
  renderer_compositor_frame_sink_ = renderer_compositor_frame_sink;
  delegated_frame_host_->DidCreateNewRendererCompositorFrameSink(
      renderer_compositor_frame_sink_);
}

void BrowserCompositorMac::OnDidNotProduceFrame(const viz::BeginFrameAck& ack) {
  delegated_frame_host_->DidNotProduceFrame(ack);
}

void BrowserCompositorMac::SetBackgroundColor(SkColor background_color) {
  background_color_ = background_color;
  if (recyclable_compositor_)
    recyclable_compositor_->compositor()->SetBackgroundColor(background_color_);
}

bool BrowserCompositorMac::UpdateNSViewAndDisplay(
    const gfx::Size& new_size_dip,
    const display::Display& new_display) {
  if (new_size_dip == dfh_size_dip_ && new_display == dfh_display_)
    return false;

  bool needs_new_surface_id =
      new_size_dip != dfh_size_dip_ ||
      new_display.device_scale_factor() != dfh_display_.device_scale_factor();

  dfh_display_ = new_display;
  dfh_size_dip_ = new_size_dip;
  dfh_size_pixels_ = gfx::ConvertSizeToPixel(dfh_display_.device_scale_factor(),
                                             dfh_size_dip_);
  root_layer_->SetBounds(gfx::Rect(dfh_size_dip_));

  if (needs_new_surface_id) {
    if (recyclable_compositor_)
      recyclable_compositor_->Suspend();
    GetDelegatedFrameHost()->SynchronizeVisualProperties(
        dfh_local_surface_id_allocator_.GenerateId(), dfh_size_dip_,
        GetDeadlinePolicy());
  }

  if (recyclable_compositor_) {
    recyclable_compositor_->compositor()->SetDisplayColorSpace(
        dfh_display_.color_space());
    recyclable_compositor_->UpdateSurface(dfh_size_pixels_,
                                          dfh_display_.device_scale_factor());
  }

  return true;
}

void BrowserCompositorMac::SynchronizeVisualProperties(
    const gfx::Size& new_size_in_pixels,
    const viz::LocalSurfaceId& child_allocated_local_surface_id) {
  if (dfh_local_surface_id_allocator_.UpdateFromChild(
          child_allocated_local_surface_id)) {
    dfh_size_dip_ = gfx::ConvertSizeToDIP(dfh_display_.device_scale_factor(),
                                          new_size_in_pixels);
    dfh_size_pixels_ = new_size_in_pixels;
    root_layer_->SetBounds(gfx::Rect(dfh_size_dip_));
    if (recyclable_compositor_) {
      recyclable_compositor_->UpdateSurface(dfh_size_pixels_,
                                            dfh_display_.device_scale_factor());
    }
    GetDelegatedFrameHost()->SynchronizeVisualProperties(
        dfh_local_surface_id_allocator_.GetCurrentLocalSurfaceId(),
        dfh_size_dip_, GetDeadlinePolicy());
  }
  client_->SynchronizeVisualProperties();
}

void BrowserCompositorMac::UpdateVSyncParameters(
    const base::TimeTicks& timebase,
    const base::TimeDelta& interval) {
  if (recyclable_compositor_) {
    recyclable_compositor_->compositor()->SetDisplayVSyncParameters(
        timebase, interval);
  }
}

void BrowserCompositorMac::SetRenderWidgetHostIsHidden(bool hidden) {
  render_widget_host_is_hidden_ = hidden;
  UpdateState();
}

void BrowserCompositorMac::SetNSViewAttachedToWindow(bool attached) {
  ns_view_attached_to_window_ = attached;
  UpdateState();
}

void BrowserCompositorMac::UpdateState() {
  // Always use the parent ui::Layer's ui::Compositor if available.
  if (parent_ui_layer_) {
    TransitionToState(UseParentLayerCompositor);
    return;
  }

  // If the host is visible and a compositor is required then create one.
  if (!render_widget_host_is_hidden_) {
    TransitionToState(HasAttachedCompositor);
    return;
  }

  // If the host is not visible but we are attached to a window then keep around
  // a compositor only if it already exists.
  if (ns_view_attached_to_window_ && state_ == HasAttachedCompositor) {
    TransitionToState(HasDetachedCompositor);
    return;
  }

  // Otherwise put the compositor up for recycling.
  TransitionToState(HasNoCompositor);
}

void BrowserCompositorMac::TransitionToState(State new_state) {
  // Note that the state enum values represent the other through which
  // transitions must be done (see comments in State definition).

  // Transition UseParentLayerCompositor -> HasNoCompositor.
  if (state_ == UseParentLayerCompositor &&
      new_state < UseParentLayerCompositor) {
    DCHECK(root_layer_->parent());
    root_layer_->parent()->RemoveObserver(this);
    root_layer_->parent()->Remove(root_layer_.get());
    delegated_frame_host_->WasHidden();
    delegated_frame_host_->ResetCompositor();
    state_ = HasNoCompositor;
  }

  // Transition HasNoCompositor -> HasDetachedCompositor.
  if (state_ == HasNoCompositor && new_state < HasNoCompositor) {
    recyclable_compositor_ = RecyclableCompositorMac::Create();
    recyclable_compositor_->UpdateSurface(dfh_size_pixels_,
                                          dfh_display_.device_scale_factor());
    recyclable_compositor_->compositor()->SetRootLayer(root_layer_.get());
    recyclable_compositor_->compositor()->SetBackgroundColor(background_color_);
    recyclable_compositor_->compositor()->SetDisplayColorSpace(
        dfh_display_.color_space());
    recyclable_compositor_->accelerated_widget_mac()->SetNSView(
        accelerated_widget_mac_ns_view_);
    state_ = HasDetachedCompositor;
  }

  // Transition HasDetachedCompositor -> HasAttachedCompositor.
  if (state_ == HasDetachedCompositor && new_state < HasDetachedCompositor) {
    delegated_frame_host_->SetCompositor(recyclable_compositor_->compositor());
    delegated_frame_host_->WasShown(GetRendererLocalSurfaceId(), dfh_size_dip_,
                                    ui::LatencyInfo());

    // If there exists a saved frame ready to display, unsuspend the compositor
    // now (if one is not ready, the compositor will unsuspend on first surface
    // activation).
    if (delegated_frame_host_->HasSavedFrame())
      recyclable_compositor_->Unsuspend();

    state_ = HasAttachedCompositor;
  }

  // Transition HasAttachedCompositor -> HasDetachedCompositor.
  if (state_ == HasAttachedCompositor && new_state > HasAttachedCompositor) {
    // Ensure that any changes made to the ui::Compositor do not result in new
    // frames being produced.
    recyclable_compositor_->Suspend();
    // Marking the DelegatedFrameHost as removed from the window hierarchy is
    // necessary to remove all connections to its old ui::Compositor.
    delegated_frame_host_->WasHidden();
    delegated_frame_host_->ResetCompositor();
    state_ = HasDetachedCompositor;
  }

  // Transition HasDetachedCompositor -> HasNoCompositor.
  if (state_ == HasDetachedCompositor && new_state > HasDetachedCompositor) {
    recyclable_compositor_->accelerated_widget_mac()->ResetNSView();
    recyclable_compositor_->compositor()->SetRootLayer(nullptr);
    recyclable_compositor_->InvalidateSurface();
    RecyclableCompositorMac::Recycle(std::move(recyclable_compositor_));
    state_ = HasNoCompositor;
  }

  // Transition HasNoCompositor -> UseParentLayerCompositor.
  if (state_ == HasNoCompositor && new_state > HasNoCompositor) {
    DCHECK(parent_ui_layer_);
    DCHECK(parent_ui_layer_->GetCompositor());
    DCHECK(!root_layer_->parent());
    delegated_frame_host_->SetCompositor(parent_ui_layer_->GetCompositor());
    delegated_frame_host_->WasShown(GetRendererLocalSurfaceId(), dfh_size_dip_,
                                    ui::LatencyInfo());
    parent_ui_layer_->Add(root_layer_.get());
    parent_ui_layer_->AddObserver(this);
    state_ = UseParentLayerCompositor;
  }
}

// static
void BrowserCompositorMac::DisableRecyclingForShutdown() {
  // Ensure that the client has destroyed its BrowserCompositorViewMac before
  // it dependencies are destroyed.
  // https://crbug.com/805726
  while (!g_browser_compositors.Get().empty()) {
    BrowserCompositorMac* browser_compositor =
        *g_browser_compositors.Get().begin();
    browser_compositor->client_->DestroyCompositorForShutdown();
  }
  g_spare_recyclable_compositors.Get().clear();
}

void BrowserCompositorMac::SetNeedsBeginFrames(bool needs_begin_frames) {
  delegated_frame_host_->SetNeedsBeginFrames(needs_begin_frames);
}

void BrowserCompositorMac::SetWantsAnimateOnlyBeginFrames() {
  delegated_frame_host_->SetWantsAnimateOnlyBeginFrames();
}

void BrowserCompositorMac::TakeFallbackContentFrom(
    BrowserCompositorMac* other) {
  delegated_frame_host_->TakeFallbackContentFrom(
      other->delegated_frame_host_.get());

  // We will have a flash if we can't recycle the compositor from |other|.
  if (other->state_ == HasDetachedCompositor && state_ == HasNoCompositor) {
    other->TransitionToState(HasNoCompositor);
    TransitionToState(HasAttachedCompositor);
  }
}

////////////////////////////////////////////////////////////////////////////////
// DelegatedFrameHost, public:

ui::Layer* BrowserCompositorMac::DelegatedFrameHostGetLayer() const {
  return root_layer_.get();
}

bool BrowserCompositorMac::DelegatedFrameHostIsVisible() const {
  return state_ != HasNoCompositor;
}

SkColor BrowserCompositorMac::DelegatedFrameHostGetGutterColor() const {
  return client_->BrowserCompositorMacGetGutterColor();
}

void BrowserCompositorMac::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {
  if (!recyclable_compositor_)
    return;

  recyclable_compositor_->Unsuspend();

  // Disable screen updates until the frame of the new size appears (because the
  // content is drawn in the GPU process, it may change before we want it to).
  if (repaint_state_ == RepaintState::Paused) {
    bool compositor_is_nsview_size =
        recyclable_compositor_->pixel_size() == dfh_size_pixels_ &&
        recyclable_compositor_->scale_factor() ==
            dfh_display_.device_scale_factor();
    if (compositor_is_nsview_size || repaint_auto_resize_enabled_) {
      NSDisableScreenUpdates();
      repaint_state_ = RepaintState::ScreenUpdatesDisabled;
    }
  }
}

void BrowserCompositorMac::OnBeginFrame(base::TimeTicks frame_time) {
  client_->BrowserCompositorMacOnBeginFrame(frame_time);
}

void BrowserCompositorMac::OnFrameTokenChanged(uint32_t frame_token) {
  client_->OnFrameTokenChanged(frame_token);
}

void BrowserCompositorMac::DidNavigate() {
  // The first navigation does not need a new LocalSurfaceID. The renderer can
  // use the ID that was already provided.
  if (!is_first_navigation_)
    dfh_local_surface_id_allocator_.GenerateId();
  const viz::LocalSurfaceId& local_surface_id =
      dfh_local_surface_id_allocator_.GetCurrentLocalSurfaceId();
  delegated_frame_host_->SynchronizeVisualProperties(
      local_surface_id, dfh_size_dip_,
      cc::DeadlinePolicy::UseExistingDeadline());
  client_->SynchronizeVisualProperties();
  delegated_frame_host_->DidNavigate();
  is_first_navigation_ = false;
}

void BrowserCompositorMac::DidReceiveFirstFrameAfterNavigation() {
  client_->DidReceiveFirstFrameAfterNavigation();
}

void BrowserCompositorMac::BeginPauseForFrame(bool auto_resize_enabled) {
  repaint_auto_resize_enabled_ = auto_resize_enabled;
  repaint_state_ = RepaintState::Paused;
}

void BrowserCompositorMac::EndPauseForFrame() {
  if (repaint_state_ == RepaintState::ScreenUpdatesDisabled)
    NSEnableScreenUpdates();
  repaint_state_ = RepaintState::None;
}

bool BrowserCompositorMac::ShouldContinueToPauseForFrame() const {
  if (state_ == UseParentLayerCompositor)
    return false;

  // The renderer won't produce a frame if its frame sink hasn't been created
  // yet.
  if (!renderer_compositor_frame_sink_)
    return false;

  if (!recyclable_compositor_)
    return false;

  return !recyclable_compositor_->accelerated_widget_mac()->HasFrameOfSize(
      dfh_size_dip_);
}

void BrowserCompositorMac::SetParentUiLayer(ui::Layer* new_parent_ui_layer) {
  if (new_parent_ui_layer) {
    DCHECK(new_parent_ui_layer->GetCompositor());
    DCHECK(!parent_ui_layer_);
    parent_ui_layer_ = new_parent_ui_layer;
  } else if (parent_ui_layer_) {
    DCHECK(root_layer_->parent());
    DCHECK_EQ(root_layer_->parent(), parent_ui_layer_);
    parent_ui_layer_ = nullptr;
  }
  UpdateState();
}

bool BrowserCompositorMac::ForceNewSurfaceForTesting() {
  display::Display new_display(dfh_display_);
  new_display.set_device_scale_factor(new_display.device_scale_factor() * 2.0f);
  return UpdateNSViewAndDisplay(dfh_size_dip_, new_display);
}

void BrowserCompositorMac::GetRendererScreenInfo(
    ScreenInfo* screen_info) const {
  DisplayUtil::DisplayToScreenInfo(screen_info, dfh_display_);
}

viz::ScopedSurfaceIdAllocator
BrowserCompositorMac::GetScopedRendererSurfaceIdAllocator(
    base::OnceCallback<void()> allocation_task) {
  return viz::ScopedSurfaceIdAllocator(&dfh_local_surface_id_allocator_,
                                       std::move(allocation_task));
}

const viz::LocalSurfaceId& BrowserCompositorMac::GetRendererLocalSurfaceId() {
  if (dfh_local_surface_id_allocator_.GetCurrentLocalSurfaceId().is_valid())
    return dfh_local_surface_id_allocator_.GetCurrentLocalSurfaceId();

  return dfh_local_surface_id_allocator_.GenerateId();
}

bool BrowserCompositorMac::UpdateRendererLocalSurfaceIdFromChild(
    const viz::LocalSurfaceId& child_allocated_local_surface_id) {
  return dfh_local_surface_id_allocator_.UpdateFromChild(
      child_allocated_local_surface_id);
}

void BrowserCompositorMac::LayerDestroyed(ui::Layer* layer) {
  DCHECK_EQ(layer, parent_ui_layer_);
  SetParentUiLayer(nullptr);
}

ui::Compositor* BrowserCompositorMac::GetCompositorForTesting() const {
  if (recyclable_compositor_)
    return recyclable_compositor_->compositor();
  return nullptr;
}

cc::DeadlinePolicy BrowserCompositorMac::GetDeadlinePolicy() const {
  // Determined empirically for smoothness.
  uint32_t frames_to_wait = 8;

  // When using the RecyclableCompositor, never wait for frames to arrive
  // (surface sync is managed by the Suspend/Unsuspend lock).
  if (recyclable_compositor_)
    frames_to_wait = 0;

  return cc::DeadlinePolicy::UseSpecifiedDeadline(frames_to_wait);
}

}  // namespace content
