// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/generic_sensor/sensor_provider_proxy_impl.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/blink/public/mojom/feature_policy/feature_policy.mojom.h"

using device::mojom::SensorType;

using device::mojom::SensorCreationResult;

namespace content {

SensorProviderProxyImpl::SensorProviderProxyImpl(
    PermissionManager* permission_manager,
    RenderFrameHost* render_frame_host)
    : permission_manager_(permission_manager),
      render_frame_host_(render_frame_host) {
  DCHECK(permission_manager);
  DCHECK(render_frame_host);
}

SensorProviderProxyImpl::~SensorProviderProxyImpl() = default;

void SensorProviderProxyImpl::Bind(
    device::mojom::SensorProviderRequest request) {
  binding_set_.AddBinding(this, std::move(request));
}

void SensorProviderProxyImpl::GetSensor(SensorType type,
                                        GetSensorCallback callback) {
  ServiceManagerConnection* connection =
      ServiceManagerConnection::GetForProcess();

  if (!connection) {
    std::move(callback).Run(SensorCreationResult::ERROR_NOT_AVAILABLE, nullptr);
    return;
  }

  if (!CheckFeaturePolicies(type) || !CheckPermission()) {
    std::move(callback).Run(SensorCreationResult::ERROR_NOT_ALLOWED, nullptr);
    return;
  }

  if (!sensor_provider_) {
    connection->GetConnector()->BindInterface(
        device::mojom::kServiceName, mojo::MakeRequest(&sensor_provider_));
    sensor_provider_.set_connection_error_handler(base::BindOnce(
        &SensorProviderProxyImpl::OnConnectionError, base::Unretained(this)));
  }
  sensor_provider_->GetSensor(type, std::move(callback));
}

bool SensorProviderProxyImpl::CheckPermission() const {
  const GURL& requesting_origin =
      render_frame_host_->GetLastCommittedURL().GetOrigin();

  blink::mojom::PermissionStatus permission_status =
      permission_manager_->GetPermissionStatusForFrame(
          PermissionType::SENSORS, render_frame_host_, requesting_origin);
  return permission_status == blink::mojom::PermissionStatus::GRANTED;
}

namespace {

std::vector<blink::mojom::FeaturePolicyFeature>
SensorTypeToFeaturePolicyFeatures(SensorType type) {
  switch (type) {
    case SensorType::AMBIENT_LIGHT:
      return {blink::mojom::FeaturePolicyFeature::kAmbientLightSensor};
    case SensorType::ACCELEROMETER:
    case SensorType::LINEAR_ACCELERATION:
      return {blink::mojom::FeaturePolicyFeature::kAccelerometer};
    case SensorType::GYROSCOPE:
      return {blink::mojom::FeaturePolicyFeature::kGyroscope};
    case SensorType::MAGNETOMETER:
      return {blink::mojom::FeaturePolicyFeature::kMagnetometer};
    case SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES:
    case SensorType::ABSOLUTE_ORIENTATION_QUATERNION:
      return {blink::mojom::FeaturePolicyFeature::kAccelerometer,
              blink::mojom::FeaturePolicyFeature::kGyroscope,
              blink::mojom::FeaturePolicyFeature::kMagnetometer};
    case SensorType::RELATIVE_ORIENTATION_EULER_ANGLES:
    case SensorType::RELATIVE_ORIENTATION_QUATERNION:
      return {blink::mojom::FeaturePolicyFeature::kAccelerometer,
              blink::mojom::FeaturePolicyFeature::kGyroscope};
    default:
      NOTREACHED() << "Unknown sensor type " << type;
      return {};
  }
}

}  // namespace

bool SensorProviderProxyImpl::CheckFeaturePolicies(SensorType type) const {
  const std::vector<blink::mojom::FeaturePolicyFeature>& features =
      SensorTypeToFeaturePolicyFeatures(type);
  return std::all_of(features.begin(), features.end(),
                     [this](blink::mojom::FeaturePolicyFeature feature) {
                       return render_frame_host_->IsFeatureEnabled(feature);
                     });
}

void SensorProviderProxyImpl::OnConnectionError() {
  // Close all the upstream bindings to notify them of this failure as the
  // GetSensorCallbacks will never be called.
  binding_set_.CloseAllBindings();
  sensor_provider_.reset();
}

}  // namespace content
