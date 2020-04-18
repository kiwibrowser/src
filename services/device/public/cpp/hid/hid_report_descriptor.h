// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_HID_HID_REPORT_DESCRIPTOR_H_
#define DEVICE_HID_HID_REPORT_DESCRIPTOR_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "services/device/public/cpp/hid/hid_report_descriptor_item.h"
#include "services/device/public/mojom/hid.mojom.h"

namespace device {

// HID report descriptor.
// See section 6.2.2 of HID specifications (v1.11).
class HidReportDescriptor {
 public:
  HidReportDescriptor(const std::vector<uint8_t>& bytes);
  ~HidReportDescriptor();

  const std::vector<std::unique_ptr<HidReportDescriptorItem>>& items() const {
    return items_;
  }

  // Returns top-level collections present in the descriptor,
  // together with max report sizes
  void GetDetails(
      std::vector<mojom::HidCollectionInfoPtr>* top_level_collections,
      bool* has_report_id,
      size_t* max_input_report_size,
      size_t* max_output_report_size,
      size_t* max_feature_report_size);

 private:
  std::vector<std::unique_ptr<HidReportDescriptorItem>> items_;
};

}  // namespace device

#endif  // DEVICE_HID_HID_REPORT_DESCRIPTOR_H_
