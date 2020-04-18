// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_device_lister_impl.h"

#include <utility>

#include "chrome/browser/printing/cloud_print/privet_constants.h"

namespace cloud_print {

PrivetDeviceListerImpl::PrivetDeviceListerImpl(
    local_discovery::ServiceDiscoveryClient* service_discovery_client,
    PrivetDeviceLister::Delegate* delegate)
    : delegate_(delegate),
      device_lister_(local_discovery::ServiceDiscoveryDeviceLister::Create(
          this,
          service_discovery_client,
          kPrivetDefaultDeviceType)) {}

PrivetDeviceListerImpl::~PrivetDeviceListerImpl() {
}

void PrivetDeviceListerImpl::Start() {
  device_lister_->Start();
}

void PrivetDeviceListerImpl::DiscoverNewDevices() {
  device_lister_->DiscoverNewDevices();
}

void PrivetDeviceListerImpl::OnDeviceChanged(
    const std::string& service_type,
    bool added,
    const local_discovery::ServiceDescription& service_description) {
  if (!delegate_)
    return;

  delegate_->DeviceChanged(service_description.service_name,
                           DeviceDescription(service_description));
}

void PrivetDeviceListerImpl::OnDeviceRemoved(const std::string& service_type,
                                             const std::string& service_name) {
  if (delegate_)
    delegate_->DeviceRemoved(service_name);
}

void PrivetDeviceListerImpl::OnDeviceCacheFlushed(
    const std::string& service_type) {
  if (delegate_)
    delegate_->DeviceCacheFlushed();
}

}  // namespace cloud_print
