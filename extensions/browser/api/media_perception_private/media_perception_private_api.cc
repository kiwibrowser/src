// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/media_perception_private/media_perception_private_api.h"

namespace media_perception = extensions::api::media_perception_private;

namespace extensions {

MediaPerceptionPrivateGetStateFunction ::
    MediaPerceptionPrivateGetStateFunction() {}

MediaPerceptionPrivateGetStateFunction ::
    ~MediaPerceptionPrivateGetStateFunction() {}

ExtensionFunction::ResponseAction
MediaPerceptionPrivateGetStateFunction::Run() {
  MediaPerceptionAPIManager* manager =
      MediaPerceptionAPIManager::Get(browser_context());
  manager->GetState(base::Bind(
      &MediaPerceptionPrivateGetStateFunction::GetStateCallback, this));
  return RespondLater();
}

void MediaPerceptionPrivateGetStateFunction::GetStateCallback(
    media_perception::State state) {
  Respond(OneArgument(state.ToValue()));
}

MediaPerceptionPrivateSetStateFunction ::
    MediaPerceptionPrivateSetStateFunction() {}

MediaPerceptionPrivateSetStateFunction ::
    ~MediaPerceptionPrivateSetStateFunction() {}

ExtensionFunction::ResponseAction
MediaPerceptionPrivateSetStateFunction::Run() {
  std::unique_ptr<media_perception::SetState::Params> params =
      media_perception::SetState::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params.get());
  if (params->state.status != media_perception::STATUS_RUNNING &&
      params->state.status != media_perception::STATUS_SUSPENDED &&
      params->state.status != media_perception::STATUS_RESTARTING &&
      params->state.status != media_perception::STATUS_STOPPED) {
    return RespondNow(
        Error("Status can only be set to RUNNING, SUSPENDED, RESTARTING, or "
              "STOPPED."));
  }

  // Check that device context is only provided with SetState RUNNING.
  if (params->state.status != media_perception::STATUS_RUNNING &&
      params->state.device_context.get() != nullptr) {
    return RespondNow(
        Error("Only provide deviceContext with SetState RUNNING."));
  }

  // Check that video stream parameters are only provided with SetState RUNNING.
  if (params->state.status != media_perception::STATUS_RUNNING &&
      params->state.video_stream_param.get() != nullptr) {
    return RespondNow(
        Error("SetState: status must be RUNNING to set videoStreamParam."));
  }

  // Check that configuration is only provided with SetState RUNNING.
  if (params->state.configuration &&
      params->state.status != media_perception::STATUS_RUNNING) {
    return RespondNow(Error("Status must be RUNNING to set configuration."));
  }

  MediaPerceptionAPIManager* manager =
      MediaPerceptionAPIManager::Get(browser_context());
  manager->SetState(
      params->state,
      base::Bind(&MediaPerceptionPrivateSetStateFunction::SetStateCallback,
                 this));
  return RespondLater();
}

void MediaPerceptionPrivateSetStateFunction::SetStateCallback(
    media_perception::State state) {
  Respond(OneArgument(state.ToValue()));
}

MediaPerceptionPrivateGetDiagnosticsFunction ::
    MediaPerceptionPrivateGetDiagnosticsFunction() {}

MediaPerceptionPrivateGetDiagnosticsFunction ::
    ~MediaPerceptionPrivateGetDiagnosticsFunction() {}

ExtensionFunction::ResponseAction
MediaPerceptionPrivateGetDiagnosticsFunction::Run() {
  MediaPerceptionAPIManager* manager =
      MediaPerceptionAPIManager::Get(browser_context());
  manager->GetDiagnostics(base::Bind(
      &MediaPerceptionPrivateGetDiagnosticsFunction::GetDiagnosticsCallback,
      this));
  return RespondLater();
}

void MediaPerceptionPrivateGetDiagnosticsFunction::GetDiagnosticsCallback(
    media_perception::Diagnostics diagnostics) {
  Respond(OneArgument(diagnostics.ToValue()));
}

MediaPerceptionPrivateSetAnalyticsComponentFunction::
    MediaPerceptionPrivateSetAnalyticsComponentFunction() {}

MediaPerceptionPrivateSetAnalyticsComponentFunction::
    ~MediaPerceptionPrivateSetAnalyticsComponentFunction() {}

ExtensionFunction::ResponseAction
MediaPerceptionPrivateSetAnalyticsComponentFunction::Run() {
  std::unique_ptr<media_perception::SetAnalyticsComponent::Params> params =
      media_perception::SetAnalyticsComponent::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params.get());

  MediaPerceptionAPIManager* manager =
      MediaPerceptionAPIManager::Get(browser_context());
  manager->SetAnalyticsComponent(
      params->component,
      base::BindOnce(&MediaPerceptionPrivateSetAnalyticsComponentFunction::
                         OnAnalyticsComponentSet,
                     this));
  return RespondLater();
}

void MediaPerceptionPrivateSetAnalyticsComponentFunction::
    OnAnalyticsComponentSet(media_perception::ComponentState component_state) {
  Respond(OneArgument(component_state.ToValue()));
}

MediaPerceptionPrivateSetComponentProcessStateFunction::
    MediaPerceptionPrivateSetComponentProcessStateFunction() = default;

MediaPerceptionPrivateSetComponentProcessStateFunction::
    ~MediaPerceptionPrivateSetComponentProcessStateFunction() = default;

ExtensionFunction::ResponseAction
MediaPerceptionPrivateSetComponentProcessStateFunction::Run() {
  return RespondNow(Error("Not implemented."));
}

}  // namespace extensions
