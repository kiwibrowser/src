// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/generic_sensor/sensor_permission_context.h"

#include "base/feature_list.h"
#include "chrome/browser/content_settings/tab_specific_content_settings.h"
#include "chrome/browser/permissions/permission_request_id.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "services/device/public/cpp/device_features.h"
#include "third_party/blink/public/mojom/feature_policy/feature_policy.mojom.h"
#include "url/gurl.h"

SensorPermissionContext::SensorPermissionContext(Profile* profile)
    : PermissionContextBase(profile,
                            CONTENT_SETTINGS_TYPE_SENSORS,
                            blink::mojom::FeaturePolicyFeature::kNotFound) {}

SensorPermissionContext::~SensorPermissionContext() {}

void SensorPermissionContext::UpdateTabContext(const PermissionRequestID& id,
                                               const GURL& requesting_frame,
                                               bool allowed) {
  // Show location bar indicator only when features::kGenericSensorExtraClasses
  // feature is enabled.
  if (!base::FeatureList::IsEnabled(features::kGenericSensorExtraClasses))
    return;

  auto* content_settings = TabSpecificContentSettings::GetForFrame(
      id.render_process_id(), id.render_frame_id());
  if (!content_settings)
    return;

  if (allowed)
    content_settings->OnContentAllowed(CONTENT_SETTINGS_TYPE_SENSORS);
  else
    content_settings->OnContentBlocked(CONTENT_SETTINGS_TYPE_SENSORS);
}

ContentSetting SensorPermissionContext::GetPermissionStatusInternal(
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  // TODO(juncai): We may need to add cross-origin iframes check here when we
  // can grant permission for certain sensor types. Currently this function
  // doesn't have any information of which sensor type requests permission.
  // The Generic Sensor API is not allowed in cross-origin iframes and
  // this is enforced by the renderer.
  // https://crbug.com/787019

  // This is to allow DeviceMotion and DeviceOrientation Event to be
  // able to access sensors (which are provided by generic sensor) in
  // cross-origin iframes. The Generic Sensor API is not allowed in
  // cross-origin iframes and this is enforced by the renderer.

  // Sensors are allowed by default in content_settings_registry.cc.
  // If cross-origin access check is required, comparison between requesting
  // and embedding origin must be performed.
  return PermissionContextBase::GetPermissionStatusInternal(
      render_frame_host, requesting_origin, embedding_origin);
}

bool SensorPermissionContext::IsRestrictedToSecureOrigins() const {
  // This is to allow non-secure origins that use DeviceMotion and
  // DeviceOrientation Event to be able to access sensors that are provided
  // by generic_sensor. The Generic Sensor API is not allowed in non-secure
  // origins and this is enforced by the renderer.
  return false;
}
