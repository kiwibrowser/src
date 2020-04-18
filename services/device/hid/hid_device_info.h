// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_HID_HID_DEVICE_INFO_H_
#define SERVICES_DEVICE_HID_HID_DEVICE_INFO_H_

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "services/device/public/mojom/hid.mojom.h"

namespace device {

#if defined(OS_MACOSX)
typedef uint64_t HidPlatformDeviceId;
#else
typedef std::string HidPlatformDeviceId;
#endif

class HidDeviceInfo : public base::RefCountedThreadSafe<HidDeviceInfo> {
 public:
  HidDeviceInfo(const HidPlatformDeviceId& platform_device_id,
                uint16_t vendor_id,
                uint16_t product_id,
                const std::string& product_name,
                const std::string& serial_number,
                mojom::HidBusType bus_type,
                const std::vector<uint8_t> report_descriptor,
                std::string device_node = "");

  HidDeviceInfo(const HidPlatformDeviceId& platform_device_id,
                uint16_t vendor_id,
                uint16_t product_id,
                const std::string& product_name,
                const std::string& serial_number,
                mojom::HidBusType bus_type,
                mojom::HidCollectionInfoPtr collection,
                size_t max_input_report_size,
                size_t max_output_report_size,
                size_t max_feature_report_size);

  const mojom::HidDeviceInfoPtr& device() { return device_; }

  // Device identification.
  const std::string& device_guid() const { return device_->guid; }
  const HidPlatformDeviceId& platform_device_id() const {
    return platform_device_id_;
  }
  uint16_t vendor_id() const { return device_->vendor_id; }
  uint16_t product_id() const { return device_->product_id; }
  const std::string& product_name() const { return device_->product_name; }
  const std::string& serial_number() const { return device_->serial_number; }
  mojom::HidBusType bus_type() const { return device_->bus_type; }

  // Top-Level Collections information.
  const std::vector<mojom::HidCollectionInfoPtr>& collections() const {
    return device_->collections;
  }
  bool has_report_id() const { return device_->has_report_id; };
  size_t max_input_report_size() const {
    return device_->max_input_report_size;
  }
  size_t max_output_report_size() const {
    return device_->max_output_report_size;
  }
  size_t max_feature_report_size() const {
    return device_->max_feature_report_size;
  }

  // The raw HID report descriptor is not available on Windows.
  const std::vector<uint8_t>& report_descriptor() const {
    return device_->report_descriptor;
  }
  const std::string& device_node() const { return device_->device_node; }

 protected:
  virtual ~HidDeviceInfo();

 private:
  friend class base::RefCountedThreadSafe<HidDeviceInfo>;

  HidPlatformDeviceId platform_device_id_;
  mojom::HidDeviceInfoPtr device_;

  DISALLOW_COPY_AND_ASSIGN(HidDeviceInfo);
};

}  // namespace device

#endif  // SERVICES_DEVICE_HID_HID_DEVICE_INFO_H_
