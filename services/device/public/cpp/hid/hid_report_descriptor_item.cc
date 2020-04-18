// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/public/cpp/hid/hid_report_descriptor_item.h"

#include <stdlib.h>

#include "base/logging.h"

namespace device {

namespace {

struct Header {
  uint8_t size : 2;
  uint8_t type : 2;
  uint8_t tag : 4;
};

}  // namespace

HidReportDescriptorItem::HidReportDescriptorItem(
    const uint8_t* bytes,
    size_t size,
    HidReportDescriptorItem* previous)
    : previous_(previous),
      next_(NULL),
      parent_(NULL),
      shortData_(0),
      payload_size_(0) {
  Header* header = (Header*)&bytes[0];
  tag_ = (Tag)(header->tag << 2 | header->type);

  if (IsLong()) {
    // In a long item, payload size is the second byte.
    if (size >= 2)
      payload_size_ = bytes[1];
  } else {
    // As per HID spec, a bSize value of 3 means 4 bytes.
    payload_size_ = header->size == 0x3 ? 4 : header->size;
    DCHECK(payload_size_ <= sizeof(shortData_));
    if (GetHeaderSize() + payload_size() <= size)
      memcpy(&shortData_, &bytes[GetHeaderSize()], payload_size());
  }

  if (previous) {
    DCHECK(!previous->next_);
    previous->next_ = this;
    switch (previous->tag()) {
      case kTagCollection:
        parent_ = previous;
        break;
      default:
        break;
    }
    if (!parent_) {
      switch (tag()) {
        case kTagEndCollection:
          if (previous->parent()) {
            parent_ = previous->parent()->parent();
          }
          break;
        default:
          parent_ = previous->parent();
          break;
      }
    }
  }
}

size_t HidReportDescriptorItem::GetDepth() const {
  HidReportDescriptorItem* parent_item = parent();
  if (parent_item)
    return parent_item->GetDepth() + 1;
  return 0;
}

bool HidReportDescriptorItem::IsLong() const {
  return tag() == kTagLong;
}

size_t HidReportDescriptorItem::GetHeaderSize() const {
  return IsLong() ? 3 : 1;
}

size_t HidReportDescriptorItem::GetSize() const {
  return GetHeaderSize() + payload_size();
}

uint32_t HidReportDescriptorItem::GetShortData() const {
  DCHECK(!IsLong());
  return shortData_;
}

HidReportDescriptorItem::CollectionType
HidReportDescriptorItem::GetCollectionTypeFromValue(uint32_t value) {
  switch (value) {
    case 0x00:
      return kCollectionTypePhysical;
    case 0x01:
      return kCollectionTypeApplication;
    case 0x02:
      return kCollectionTypeLogical;
    case 0x03:
      return kCollectionTypeReport;
    case 0x04:
      return kCollectionTypeNamedArray;
    case 0x05:
      return kCollectionTypeUsageSwitch;
    case 0x06:
      return kCollectionTypeUsageModifier;
    default:
      break;
  }
  if (0x80 < value && value < 0xFF)
    return kCollectionTypeVendor;
  return kCollectionTypeReserved;
}

}  // namespace device
