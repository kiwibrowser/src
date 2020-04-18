// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/device_orientation/device_orientation_inspector_agent.h"

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/modules/device_orientation/device_orientation_controller.h"
#include "third_party/blink/renderer/modules/device_orientation/device_orientation_data.h"
#include "third_party/blink/renderer/modules/sensor/sensor_inspector_agent.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

using protocol::Response;

namespace DeviceOrientationInspectorAgentState {
static const char kAlpha[] = "alpha";
static const char kBeta[] = "beta";
static const char kGamma[] = "gamma";
static const char kOverrideEnabled[] = "overrideEnabled";
}

DeviceOrientationInspectorAgent::~DeviceOrientationInspectorAgent() = default;

DeviceOrientationInspectorAgent::DeviceOrientationInspectorAgent(
    InspectedFrames* inspected_frames)
    : inspected_frames_(inspected_frames),
      sensor_agent_(new SensorInspectorAgent(inspected_frames->Root())) {}

void DeviceOrientationInspectorAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(inspected_frames_);
  visitor->Trace(sensor_agent_);
  InspectorBaseAgent::Trace(visitor);
}

DeviceOrientationController* DeviceOrientationInspectorAgent::Controller() {
  Document* document = inspected_frames_->Root()->GetDocument();
  return document ? &DeviceOrientationController::From(*document) : nullptr;
}

Response DeviceOrientationInspectorAgent::setDeviceOrientationOverride(
    double alpha,
    double beta,
    double gamma) {
  state_->setBoolean(DeviceOrientationInspectorAgentState::kOverrideEnabled,
                     true);
  state_->setDouble(DeviceOrientationInspectorAgentState::kAlpha, alpha);
  state_->setDouble(DeviceOrientationInspectorAgentState::kBeta, beta);
  state_->setDouble(DeviceOrientationInspectorAgentState::kGamma, gamma);
  if (Controller()) {
    Controller()->SetOverride(
        DeviceOrientationData::Create(alpha, beta, gamma, false));
  }
  sensor_agent_->SetOrientationSensorOverride(alpha, beta, gamma);
  return Response::OK();
}

Response DeviceOrientationInspectorAgent::clearDeviceOrientationOverride() {
  state_->setBoolean(DeviceOrientationInspectorAgentState::kOverrideEnabled,
                     false);
  if (Controller())
    Controller()->ClearOverride();
  sensor_agent_->Disable();
  return Response::OK();
}

Response DeviceOrientationInspectorAgent::disable() {
  state_->setBoolean(DeviceOrientationInspectorAgentState::kOverrideEnabled,
                     false);
  if (Controller())
    Controller()->ClearOverride();
  sensor_agent_->Disable();
  return Response::OK();
}

void DeviceOrientationInspectorAgent::Restore() {
  if (!Controller())
    return;
  if (state_->booleanProperty(
          DeviceOrientationInspectorAgentState::kOverrideEnabled, false)) {
    double alpha = 0;
    state_->getDouble(DeviceOrientationInspectorAgentState::kAlpha, &alpha);
    double beta = 0;
    state_->getDouble(DeviceOrientationInspectorAgentState::kBeta, &beta);
    double gamma = 0;
    state_->getDouble(DeviceOrientationInspectorAgentState::kGamma, &gamma);
    Controller()->SetOverride(
        DeviceOrientationData::Create(alpha, beta, gamma, false));
    sensor_agent_->SetOrientationSensorOverride(alpha, beta, gamma);
  }
}

void DeviceOrientationInspectorAgent::DidCommitLoadForLocalFrame(
    LocalFrame* frame) {
  if (frame == inspected_frames_->Root()) {
    // New document in main frame - apply override there.
    // No need to cleanup previous one, as it's already gone.
    Restore();
  }
}

}  // namespace blink
