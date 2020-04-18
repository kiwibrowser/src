// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_session.h"

#include "base/auto_reset.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/bindings/modules/v8/v8_xr_frame_request_callback.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/resize_observer/resize_observer.h"
#include "third_party/blink/renderer/core/resize_observer/resize_observer_entry.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/screen_orientation/screen_orientation.h"
#include "third_party/blink/renderer/modules/xr/xr.h"
#include "third_party/blink/renderer/modules/xr/xr_canvas_input_provider.h"
#include "third_party/blink/renderer/modules/xr/xr_device.h"
#include "third_party/blink/renderer/modules/xr/xr_frame_of_reference.h"
#include "third_party/blink/renderer/modules/xr/xr_frame_of_reference_options.h"
#include "third_party/blink/renderer/modules/xr/xr_frame_provider.h"
#include "third_party/blink/renderer/modules/xr/xr_input_source_event.h"
#include "third_party/blink/renderer/modules/xr/xr_layer.h"
#include "third_party/blink/renderer/modules/xr/xr_presentation_context.h"
#include "third_party/blink/renderer/modules/xr/xr_presentation_frame.h"
#include "third_party/blink/renderer/modules/xr/xr_session_event.h"
#include "third_party/blink/renderer/modules/xr/xr_view.h"

namespace blink {

namespace {

const char kSessionEnded[] = "XRSession has already ended.";

const char kUnknownFrameOfReference[] = "Unknown frame of reference type.";

const char kNonEmulatedStageNotSupported[] =
    "This device does not support a non-emulated 'stage' frame of reference.";

const double kDegToRad = M_PI / 180.0;

// TODO(bajones): This is something that we probably want to make configurable.
const double kMagicWindowVerticalFieldOfView = 75.0f * M_PI / 180.0f;

void UpdateViewFromEyeParameters(
    XRView* view,
    const device::mojom::blink::VREyeParametersPtr& eye,
    double depth_near,
    double depth_far) {
  const device::mojom::blink::VRFieldOfViewPtr& fov = eye->fieldOfView;

  view->UpdateProjectionMatrixFromFoV(
      fov->upDegrees * kDegToRad, fov->downDegrees * kDegToRad,
      fov->leftDegrees * kDegToRad, fov->rightDegrees * kDegToRad, depth_near,
      depth_far);

  view->UpdateOffset(eye->offset[0], eye->offset[1], eye->offset[2]);
}

}  // namespace

class XRSession::XRSessionResizeObserverDelegate final
    : public ResizeObserver::Delegate {
 public:
  explicit XRSessionResizeObserverDelegate(XRSession* session)
      : session_(session) {
    DCHECK(session);
  }
  ~XRSessionResizeObserverDelegate() override = default;

  void OnResize(
      const HeapVector<Member<ResizeObserverEntry>>& entries) override {
    DCHECK_EQ(1u, entries.size());
    session_->UpdateCanvasDimensions(entries[0]->target());
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(session_);
    ResizeObserver::Delegate::Trace(visitor);
  }

 private:
  Member<XRSession> session_;
};

XRSession::XRSession(XRDevice* device,
                     bool exclusive,
                     XRPresentationContext* output_context)
    : device_(device),
      exclusive_(exclusive),
      output_context_(output_context),
      callback_collection_(device->GetExecutionContext()) {
  blurred_ = !HasAppropriateFocus();

  // When an output context is provided, monitor it for resize events.
  if (output_context_) {
    HTMLCanvasElement* canvas = outputContext()->canvas();
    if (canvas) {
      resize_observer_ = ResizeObserver::Create(
          canvas->GetDocument(), new XRSessionResizeObserverDelegate(this));
      resize_observer_->observe(canvas);

      // Begin processing input events on the output context's canvas.
      if (!exclusive_) {
        canvas_input_provider_ = new XRCanvasInputProvider(this, canvas);
      }

      // Get the initial canvas dimensions
      UpdateCanvasDimensions(canvas);
    }
  }
}

void XRSession::setDepthNear(double value) {
  if (depth_near_ != value) {
    update_views_next_frame_ = true;
    depth_near_ = value;
  }
}

void XRSession::setDepthFar(double value) {
  if (depth_far_ != value) {
    update_views_next_frame_ = true;
    depth_far_ = value;
  }
}

void XRSession::setBaseLayer(XRLayer* value) {
  base_layer_ = value;
  // Make sure that the layer's drawing buffer is updated to the right size
  // if this is a non-exclusive session.
  if (!exclusive_ && base_layer_) {
    base_layer_->OnResize();
  }
}

void XRSession::SetNonExclusiveProjectionMatrix(
    const WTF::Vector<float>& projection_matrix) {
  DCHECK_EQ(projection_matrix.size(), 16lu);

  non_exclusive_projection_matrix_ = projection_matrix;
  // It is about as expensive to check equality as to just
  // update the views, so just update.
  update_views_next_frame_ = true;
}

ExecutionContext* XRSession::GetExecutionContext() const {
  return device_->GetExecutionContext();
}

const AtomicString& XRSession::InterfaceName() const {
  return EventTargetNames::XRSession;
}

ScriptPromise XRSession::requestFrameOfReference(
    ScriptState* script_state,
    const String& type,
    const XRFrameOfReferenceOptions& options) {
  if (ended_) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kInvalidStateError, kSessionEnded));
  }

  XRFrameOfReference* frameOfRef = nullptr;
  if (type == "headModel") {
    frameOfRef =
        new XRFrameOfReference(this, XRFrameOfReference::kTypeHeadModel);
  } else if (type == "eyeLevel") {
    frameOfRef =
        new XRFrameOfReference(this, XRFrameOfReference::kTypeEyeLevel);
  } else if (type == "stage") {
    if (!options.disableStageEmulation()) {
      frameOfRef = new XRFrameOfReference(this, XRFrameOfReference::kTypeStage);
      frameOfRef->UseEmulatedHeight(options.stageEmulationHeight());
    } else if (device_->xrDisplayInfoPtr()->stageParameters) {
      frameOfRef = new XRFrameOfReference(this, XRFrameOfReference::kTypeStage);
    } else {
      return ScriptPromise::RejectWithDOMException(
          script_state, DOMException::Create(kNotSupportedError,
                                             kNonEmulatedStageNotSupported));
    }
  }

  if (!frameOfRef) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kNotSupportedError, kUnknownFrameOfReference));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  resolver->Resolve(frameOfRef);

  return promise;
}

int XRSession::requestAnimationFrame(V8XRFrameRequestCallback* callback) {
  TRACE_EVENT0("gpu", __FUNCTION__);
  // Don't allow any new frame requests once the session is ended.
  if (ended_)
    return 0;

  // Don't allow frames to be scheduled if there's no layers attached to the
  // session. That would allow tracking with no associated visuals.
  if (!base_layer_)
    return 0;

  int id = callback_collection_.RegisterCallback(callback);
  if (!pending_frame_) {
    // Kick off a request for a new XR frame.
    device_->frameProvider()->RequestFrame(this);
    pending_frame_ = true;
  }
  return id;
}

void XRSession::cancelAnimationFrame(int id) {
  callback_collection_.CancelCallback(id);
}

HeapVector<Member<XRInputSource>> XRSession::getInputSources() const {
  Document* doc = ToDocumentOrNull(GetExecutionContext());
  if (!did_log_getInputSources_ && doc) {
    ukm::builders::XR_WebXR(device_->GetSourceId())
        .SetDidGetXRInputSources(1)
        .Record(doc->UkmRecorder());
    did_log_getInputSources_ = true;
  }

  HeapVector<Member<XRInputSource>> source_array;
  for (const auto& input_source : input_sources_.Values()) {
    source_array.push_back(input_source);
  }

  if (canvas_input_provider_) {
    XRInputSource* input_source = canvas_input_provider_->GetInputSource();
    if (input_source) {
      source_array.push_back(input_source);
    }
  }

  return source_array;
}

ScriptPromise XRSession::end(ScriptState* script_state) {
  // Don't allow a session to end twice.
  if (ended_) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kInvalidStateError, kSessionEnded));
  }

  ForceEnd();

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // TODO(bajones): If there's any work that needs to be done asynchronously on
  // session end it should be completed before this promise is resolved.

  resolver->Resolve();
  return promise;
}

void XRSession::ForceEnd() {
  // Detach this session from the device.
  ended_ = true;
  pending_frame_ = false;

  if (canvas_input_provider_) {
    canvas_input_provider_->Stop();
    canvas_input_provider_ = nullptr;
  }

  // If this session is the active exclusive session for the device, notify the
  // frameProvider that it's ended.
  if (device_->frameProvider()->exclusive_session() == this) {
    device_->frameProvider()->OnExclusiveSessionEnded();
  }

  DispatchEvent(XRSessionEvent::Create(EventTypeNames::end, this));
}

double XRSession::DefaultFramebufferScale() const {
  if (exclusive_)
    return device_->xrDisplayInfoPtr()->webxr_default_framebuffer_scale;
  return 1.0;
}

DoubleSize XRSession::IdealFramebufferSize() const {
  if (!exclusive_) {
    return OutputCanvasSize();
  }

  double width = device_->xrDisplayInfoPtr()->leftEye->renderWidth +
                 device_->xrDisplayInfoPtr()->rightEye->renderWidth;
  double height = std::max(device_->xrDisplayInfoPtr()->leftEye->renderHeight,
                           device_->xrDisplayInfoPtr()->rightEye->renderHeight);
  return DoubleSize(width, height);
}

DoubleSize XRSession::OutputCanvasSize() const {
  if (!output_context_) {
    return DoubleSize();
  }

  return DoubleSize(output_width_, output_height_);
}

int XRSession::OutputCanvasAngle() const {
  return output_angle_;
}

void XRSession::OnFocus() {
  if (!blurred_)
    return;

  blurred_ = false;
  DispatchEvent(XRSessionEvent::Create(EventTypeNames::focus, this));
}

void XRSession::OnBlur() {
  if (blurred_)
    return;

  blurred_ = true;
  DispatchEvent(XRSessionEvent::Create(EventTypeNames::blur, this));
}

// Exclusive sessions may still not be blurred in headset even if the page isn't
// focused.  This prevents the in-headset experience from freezing on an
// external display headset when the user clicks on another tab.
bool XRSession::HasAppropriateFocus() {
  return exclusive_ ? device_->HasDeviceFocus()
                    : device_->HasDeviceAndFrameFocus();
}

void XRSession::OnFocusChanged() {
  if (HasAppropriateFocus()) {
    OnFocus();
  } else {
    OnBlur();
  }
}

void XRSession::OnFrame(
    std::unique_ptr<TransformationMatrix> base_pose_matrix,
    const base::Optional<gpu::MailboxHolder>& buffer_mailbox_holder) {
  TRACE_EVENT0("gpu", __FUNCTION__);
  DVLOG(2) << __FUNCTION__;
  // Don't process any outstanding frames once the session is ended.
  if (ended_)
    return;

  base_pose_matrix_ = std::move(base_pose_matrix);

  // Don't allow frames to be processed if there's no layers attached to the
  // session. That would allow tracking with no associated visuals.
  if (!base_layer_)
    return;

  XRPresentationFrame* presentation_frame = CreatePresentationFrame();

  if (pending_frame_) {
    pending_frame_ = false;

    // Make sure that any frame-bounded changed to the views array take effect.
    if (update_views_next_frame_) {
      views_dirty_ = true;
      update_views_next_frame_ = false;
    }

    // Cache the base layer, since it could change during the frame callback.
    XRLayer* frame_base_layer = base_layer_;
    frame_base_layer->OnFrameStart(buffer_mailbox_holder);

    // Resolve the queued requestAnimationFrame callbacks. All XR rendering will
    // happen within these calls. resolving_frame_ will be true for the duration
    // of the callbacks.
    base::AutoReset<bool> resolving(&resolving_frame_, true);
    callback_collection_.ExecuteCallbacks(this, presentation_frame);

    // The session might have ended in the middle of the frame. Only call
    // OnFrameEnd if it's still valid.
    if (!ended_)
      frame_base_layer->OnFrameEnd();
  }
}

void XRSession::LogGetPose() const {
  Document* doc = ToDocumentOrNull(GetExecutionContext());
  if (!did_log_getDevicePose_ && doc) {
    did_log_getDevicePose_ = true;

    ukm::builders::XR_WebXR(device_->GetSourceId())
        .SetDidRequestPose(1)
        .Record(doc->UkmRecorder());
  }
}

XRPresentationFrame* XRSession::CreatePresentationFrame() {
  XRPresentationFrame* presentation_frame = new XRPresentationFrame(this);
  if (base_pose_matrix_) {
    presentation_frame->SetBasePoseMatrix(*base_pose_matrix_);
  }
  return presentation_frame;
}

// Called when the canvas element for this session's output context is resized.
void XRSession::UpdateCanvasDimensions(Element* element) {
  DCHECK(element);

  double devicePixelRatio = 1.0;
  LocalFrame* frame = device_->xr()->GetFrame();
  if (frame) {
    devicePixelRatio = frame->DevicePixelRatio();
  }

  update_views_next_frame_ = true;
  output_width_ = element->OffsetWidth() * devicePixelRatio;
  output_height_ = element->OffsetHeight() * devicePixelRatio;

  // TODO(crbug.com/836948): handle square canvases.
  // TODO(crbug.com/840346): we should not need to use ScreenOrientation here.
  ScreenOrientation* orientation = ScreenOrientation::Create(frame);
  if (orientation) {
    output_angle_ = orientation->angle();
    DVLOG(2) << __FUNCTION__ << ": got angle=" << output_angle_;
  }

  if (base_layer_) {
    base_layer_->OnResize();
  }
}

void XRSession::OnInputStateChange(
    int16_t frame_id,
    const WTF::Vector<device::mojom::blink::XRInputSourceStatePtr>&
        input_states) {
  bool devices_changed = false;

  // Update any input sources with new state information. Any updated input
  // sources are marked as active.
  for (const auto& input_state : input_states) {
    XRInputSource* input_source = input_sources_.at(input_state->source_id);
    if (!input_source) {
      input_source = new XRInputSource(this, input_state->source_id);
      input_sources_.Set(input_state->source_id, input_source);
      devices_changed = true;
    }
    input_source->active_frame_id = frame_id;
    UpdateInputSourceState(input_source, input_state);
  }

  // Remove any input sources that are inactive..
  std::vector<uint32_t> inactive_sources;
  for (const auto& input_source : input_sources_.Values()) {
    if (input_source->active_frame_id != frame_id) {
      inactive_sources.push_back(input_source->source_id());
      devices_changed = true;
    }
  }

  if (inactive_sources.size()) {
    for (uint32_t source_id : inactive_sources) {
      input_sources_.erase(source_id);
    }
  }

  if (devices_changed) {
    DispatchEvent(
        XRSessionEvent::Create(EventTypeNames::inputsourceschange, this));
  }
}

void XRSession::OnSelectStart(XRInputSource* input_source) {
  // Discard duplicate events
  if (input_source->primary_input_pressed)
    return;

  input_source->primary_input_pressed = true;
  input_source->selection_cancelled = false;

  XRInputSourceEvent* event =
      CreateInputSourceEvent(EventTypeNames::selectstart, input_source);
  DispatchEvent(event);

  if (event->defaultPrevented())
    input_source->selection_cancelled = true;
}

void XRSession::OnSelectEnd(XRInputSource* input_source) {
  // Discard duplicate events
  if (!input_source->primary_input_pressed)
    return;

  input_source->primary_input_pressed = false;

  LocalFrame* frame = device_->xr()->GetFrame();
  if (!frame)
    return;

  std::unique_ptr<UserGestureIndicator> gesture_indicator =
      Frame::NotifyUserActivation(frame);

  XRInputSourceEvent* event =
      CreateInputSourceEvent(EventTypeNames::selectend, input_source);
  DispatchEvent(event);

  if (event->defaultPrevented())
    input_source->selection_cancelled = true;
}

void XRSession::OnSelect(XRInputSource* input_source) {
  // If a select was fired but we had not previously started the selection it
  // indictes a sub-frame or instantanous select event, and we should fire a
  // selectstart prior to the selectend.
  if (!input_source->primary_input_pressed) {
    OnSelectStart(input_source);
  }

  // Make sure we end the selection prior to firing the select event.
  OnSelectEnd(input_source);

  if (!input_source->selection_cancelled) {
    XRInputSourceEvent* event =
        CreateInputSourceEvent(EventTypeNames::select, input_source);
    DispatchEvent(event);
  }
}

void XRSession::OnPoseReset() {
  DispatchEvent(XRSessionEvent::Create(EventTypeNames::resetpose, this));
}

void XRSession::UpdateInputSourceState(
    XRInputSource* input_source,
    const device::mojom::blink::XRInputSourceStatePtr& state) {
  if (!input_source || !state)
    return;

  // Update the input source's description if this state update
  // includes them.
  if (state->description) {
    const device::mojom::blink::XRInputSourceDescriptionPtr& desc =
        state->description;

    input_source->SetPointerOrigin(
        static_cast<XRInputSource::PointerOrigin>(desc->pointer_origin));

    input_source->SetHandedness(
        static_cast<XRInputSource::Handedness>(desc->handedness));

    input_source->SetEmulatedPosition(desc->emulated_position);

    if (desc->pointer_offset && desc->pointer_offset->matrix.has_value()) {
      const WTF::Vector<float>& m = desc->pointer_offset->matrix.value();
      std::unique_ptr<TransformationMatrix> pointer_matrix =
          TransformationMatrix::Create(m[0], m[1], m[2], m[3], m[4], m[5], m[6],
                                       m[7], m[8], m[9], m[10], m[11], m[12],
                                       m[13], m[14], m[15]);
      input_source->SetPointerTransformMatrix(std::move(pointer_matrix));
    }
  }

  if (state->grip && state->grip->matrix.has_value()) {
    const Vector<float>& m = state->grip->matrix.value();
    std::unique_ptr<TransformationMatrix> grip_matrix =
        TransformationMatrix::Create(m[0], m[1], m[2], m[3], m[4], m[5], m[6],
                                     m[7], m[8], m[9], m[10], m[11], m[12],
                                     m[13], m[14], m[15]);
    input_source->SetBasePoseMatrix(std::move(grip_matrix));
  }

  // Handle state change of the primary input, which may fire events
  if (state->primary_input_clicked)
    OnSelect(input_source);

  if (state->primary_input_pressed) {
    OnSelectStart(input_source);
  } else if (input_source->primary_input_pressed) {
    // May get here if the input source was previously pressed but now isn't,
    // but the input source did not set primary_input_clicked to true. We will
    // treat this as a cancelled selection, firing the selectend event so the
    // page stays in sync with the controller state but won't fire the
    // usual select event.
    OnSelectEnd(input_source);
  }
}

XRInputSourceEvent* XRSession::CreateInputSourceEvent(
    const AtomicString& type,
    XRInputSource* input_source) {
  XRPresentationFrame* presentation_frame = CreatePresentationFrame();
  return XRInputSourceEvent::Create(type, presentation_frame, input_source);
}

const HeapVector<Member<XRView>>& XRSession::views() {
  // TODO(bajones): For now we assume that exclusive sessions render a stereo
  // pair of views and non-exclusive sessions render a single view. That doesn't
  // always hold true, however, so the view configuration should ultimately come
  // from the backing service.
  if (views_dirty_) {
    if (exclusive_) {
      // If we don't already have the views allocated, do so now.
      if (views_.IsEmpty()) {
        views_.push_back(new XRView(this, XRView::kEyeLeft));
        views_.push_back(new XRView(this, XRView::kEyeRight));
      }
      // In exclusive mode the projection and view matrices must be aligned with
      // the device's physical optics.
      UpdateViewFromEyeParameters(views_[XRView::kEyeLeft],
                                  device_->xrDisplayInfoPtr()->leftEye,
                                  depth_near_, depth_far_);
      UpdateViewFromEyeParameters(views_[XRView::kEyeRight],
                                  device_->xrDisplayInfoPtr()->rightEye,
                                  depth_near_, depth_far_);
    } else {
      if (views_.IsEmpty()) {
        views_.push_back(new XRView(this, XRView::kEyeLeft));
        views_[XRView::kEyeLeft]->UpdateOffset(0, 0, 0);
      }

      float aspect = 1.0f;
      if (output_width_ && output_height_) {
        aspect = static_cast<float>(output_width_) /
                 static_cast<float>(output_height_);
      }

      if (non_exclusive_projection_matrix_.size() > 0) {
        views_[XRView::kEyeLeft]->UpdateProjectionMatrixFromRawValues(
            non_exclusive_projection_matrix_, depth_near_, depth_far_);
      } else {
        // In non-exclusive mode, if there is no explicit projection matrix
        // provided, the projection matrix must be aligned with the
        // output canvas dimensions.
        views_[XRView::kEyeLeft]->UpdateProjectionMatrixFromAspect(
            kMagicWindowVerticalFieldOfView, aspect, depth_near_, depth_far_);
      }
    }

    views_dirty_ = false;
  } else {
    // TODO(https://crbug.com/836926): views_dirty_ is not working right for
    // AR mode, we're not picking up the change on the right frame. Remove this
    // fallback once that's sorted out.
    DVLOG(2) << __FUNCTION__ << ": FIXME, fallback proj matrix update";
    if (non_exclusive_projection_matrix_.size() > 0) {
      views_[XRView::kEyeLeft]->UpdateProjectionMatrixFromRawValues(
          non_exclusive_projection_matrix_, depth_near_, depth_far_);
    }
  }

  return views_;
}

void XRSession::Trace(blink::Visitor* visitor) {
  visitor->Trace(device_);
  visitor->Trace(output_context_);
  visitor->Trace(base_layer_);
  visitor->Trace(views_);
  visitor->Trace(input_sources_);
  visitor->Trace(resize_observer_);
  visitor->Trace(canvas_input_provider_);
  visitor->Trace(callback_collection_);
  EventTargetWithInlineData::Trace(visitor);
}

void XRSession::TraceWrappers(blink::ScriptWrappableVisitor* visitor) const {
  for (const auto& input_source : input_sources_.Values())
    visitor->TraceWrappers(input_source);

  visitor->TraceWrappers(callback_collection_);
  EventTargetWithInlineData::TraceWrappers(visitor);
}

}  // namespace blink
