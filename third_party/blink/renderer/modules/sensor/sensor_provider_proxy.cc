// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/sensor/sensor_provider_proxy.h"

#include "services/device/public/mojom/constants.mojom-blink.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/renderer/modules/sensor/sensor_proxy_impl.h"
#include "third_party/blink/renderer/modules/sensor/sensor_proxy_inspector_impl.h"
#include "third_party/blink/renderer/platform/mojo/mojo_helper.h"

namespace blink {

// SensorProviderProxy
SensorProviderProxy::SensorProviderProxy(LocalFrame& frame)
    : Supplement<LocalFrame>(frame), inspector_mode_(false) {}

void SensorProviderProxy::InitializeIfNeeded() {
  if (IsInitialized())
    return;

  GetSupplementable()->GetInterfaceProvider().GetInterface(
      mojo::MakeRequest(&sensor_provider_));
  sensor_provider_.set_connection_error_handler(
      WTF::Bind(&SensorProviderProxy::OnSensorProviderConnectionError,
                WrapWeakPersistent(this)));
}

// static
const char SensorProviderProxy::kSupplementName[] = "SensorProvider";

// static
SensorProviderProxy* SensorProviderProxy::From(LocalFrame* frame) {
  DCHECK(frame);
  SensorProviderProxy* provider_proxy =
      Supplement<LocalFrame>::From<SensorProviderProxy>(*frame);
  if (!provider_proxy) {
    provider_proxy = new SensorProviderProxy(*frame);
    Supplement<LocalFrame>::ProvideTo(*frame, provider_proxy);
  }
  provider_proxy->InitializeIfNeeded();
  return provider_proxy;
}

SensorProviderProxy::~SensorProviderProxy() = default;

void SensorProviderProxy::Trace(blink::Visitor* visitor) {
  visitor->Trace(sensor_proxies_);
  Supplement<LocalFrame>::Trace(visitor);
}

SensorProxy* SensorProviderProxy::CreateSensorProxy(
    device::mojom::blink::SensorType type,
    Page* page) {
  DCHECK(!GetSensorProxy(type));

  SensorProxy* sensor =
      inspector_mode_
          ? static_cast<SensorProxy*>(
                new SensorProxyInspectorImpl(type, this, page))
          : static_cast<SensorProxy*>(new SensorProxyImpl(type, this, page));
  sensor_proxies_.insert(sensor);

  return sensor;
}

SensorProxy* SensorProviderProxy::GetSensorProxy(
    device::mojom::blink::SensorType type) {
  for (SensorProxy* sensor : sensor_proxies_) {
    // TODO(Mikhail) : Hash sensors by type for efficiency.
    if (sensor->type() == type)
      return sensor;
  }

  return nullptr;
}

void SensorProviderProxy::OnSensorProviderConnectionError() {
  sensor_provider_.reset();
  for (SensorProxy* sensor : sensor_proxies_) {
    sensor->ReportError(kNotReadableError,
                        SensorProxy::kDefaultErrorDescription);
  }
}

void SensorProviderProxy::RemoveSensorProxy(SensorProxy* proxy) {
  DCHECK(sensor_proxies_.Contains(proxy));
  sensor_proxies_.erase(proxy);
}

}  // namespace blink
