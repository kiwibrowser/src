// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/mediacapturefromelement/canvas_capture_media_stream_track.h"

#include <memory>
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/modules/mediacapturefromelement/auto_canvas_draw_listener.h"
#include "third_party/blink/renderer/modules/mediacapturefromelement/on_request_canvas_draw_listener.h"
#include "third_party/blink/renderer/modules/mediacapturefromelement/timed_canvas_draw_listener.h"
#include "third_party/blink/renderer/platform/mediastream/media_stream_center.h"

namespace blink {

CanvasCaptureMediaStreamTrack* CanvasCaptureMediaStreamTrack::Create(
    MediaStreamComponent* component,
    HTMLCanvasElement* element,
    ExecutionContext* context,
    std::unique_ptr<WebCanvasCaptureHandler> handler) {
  return new CanvasCaptureMediaStreamTrack(component, element, context,
                                           std::move(handler));
}

CanvasCaptureMediaStreamTrack* CanvasCaptureMediaStreamTrack::Create(
    MediaStreamComponent* component,
    HTMLCanvasElement* element,
    ExecutionContext* context,
    std::unique_ptr<WebCanvasCaptureHandler> handler,
    double frame_rate) {
  return new CanvasCaptureMediaStreamTrack(component, element, context,
                                           std::move(handler), frame_rate);
}

HTMLCanvasElement* CanvasCaptureMediaStreamTrack::canvas() const {
  return canvas_element_.Get();
}

void CanvasCaptureMediaStreamTrack::requestFrame() {
  draw_listener_->RequestFrame();
}

CanvasCaptureMediaStreamTrack* CanvasCaptureMediaStreamTrack::clone(
    ScriptState* script_state) {
  MediaStreamComponent* cloned_component = Component()->Clone();
  CanvasCaptureMediaStreamTrack* cloned_track =
      new CanvasCaptureMediaStreamTrack(*this, cloned_component);
  MediaStreamCenter::Instance().DidCreateMediaStreamTrack(cloned_component);
  return cloned_track;
}

void CanvasCaptureMediaStreamTrack::Trace(blink::Visitor* visitor) {
  visitor->Trace(canvas_element_);
  visitor->Trace(draw_listener_);
  MediaStreamTrack::Trace(visitor);
}

CanvasCaptureMediaStreamTrack::CanvasCaptureMediaStreamTrack(
    const CanvasCaptureMediaStreamTrack& track,
    MediaStreamComponent* component)
    : MediaStreamTrack(track.canvas_element_->GetExecutionContext(), component),
      canvas_element_(track.canvas_element_),
      draw_listener_(track.draw_listener_) {
  canvas_element_->AddListener(draw_listener_.Get());
}

CanvasCaptureMediaStreamTrack::CanvasCaptureMediaStreamTrack(
    MediaStreamComponent* component,
    HTMLCanvasElement* element,
    ExecutionContext* context,
    std::unique_ptr<WebCanvasCaptureHandler> handler)
    : MediaStreamTrack(context, component), canvas_element_(element) {
  draw_listener_ = AutoCanvasDrawListener::Create(std::move(handler));
  canvas_element_->AddListener(draw_listener_.Get());
}

CanvasCaptureMediaStreamTrack::CanvasCaptureMediaStreamTrack(
    MediaStreamComponent* component,
    HTMLCanvasElement* element,
    ExecutionContext* context,
    std::unique_ptr<WebCanvasCaptureHandler> handler,
    double frame_rate)
    : MediaStreamTrack(context, component), canvas_element_(element) {
  if (frame_rate == 0) {
    draw_listener_ = OnRequestCanvasDrawListener::Create(std::move(handler));
  } else {
    draw_listener_ = TimedCanvasDrawListener::Create(std::move(handler),
                                                     frame_rate, context);
  }
  canvas_element_->AddListener(draw_listener_.Get());
}

}  // namespace blink
