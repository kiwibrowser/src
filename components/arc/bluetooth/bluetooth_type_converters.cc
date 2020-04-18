// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cctype>
#include <iomanip>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/arc/bluetooth/bluetooth_type_converters.h"
#include "device/bluetooth/bluetooth_gatt_service.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace {

constexpr size_t kAddressSize = 6;
constexpr char kInvalidAddress[] = "00:00:00:00:00:00";

// SDP Service attribute IDs.
constexpr uint16_t kServiceClassIDList = 0x0001;
constexpr uint16_t kProtocolDescriptorList = 0x0004;
constexpr uint16_t kBrowseGroupList = 0x0005;
constexpr uint16_t kBluetoothProfileDescriptorList = 0x0009;
constexpr uint16_t kServiceName = 0x0100;

bool IsNonHex(char c) {
  return !isxdigit(c);
}

std::string StripNonHex(const std::string& str) {
  std::string result = str;
  base::EraseIf(result, IsNonHex);
  return result;
}

}  // namespace

namespace mojo {

// static
arc::mojom::BluetoothAddressPtr
TypeConverter<arc::mojom::BluetoothAddressPtr, std::string>::Convert(
    const std::string& address) {

  arc::mojom::BluetoothAddressPtr mojo_addr =
      arc::mojom::BluetoothAddress::New();
  base::HexStringToBytes(StripNonHex(address), &mojo_addr->address);

  return mojo_addr;
}

// static
std::string TypeConverter<std::string, arc::mojom::BluetoothAddress>::Convert(
    const arc::mojom::BluetoothAddress& address) {
  std::ostringstream addr_stream;
  addr_stream << std::setfill('0') << std::hex << std::uppercase;

  const std::vector<uint8_t>& bytes = address.address;

  if (address.address.size() != kAddressSize)
    return std::string(kInvalidAddress);

  for (size_t k = 0; k < bytes.size(); k++) {
    addr_stream << std::setw(2) << (unsigned int)bytes[k];
    addr_stream << ((k == bytes.size() - 1) ? "" : ":");
  }

  return addr_stream.str();
}

// static
arc::mojom::BluetoothSdpAttributePtr
TypeConverter<arc::mojom::BluetoothSdpAttributePtr,
              bluez::BluetoothServiceAttributeValueBlueZ>::
    Convert(const bluez::BluetoothServiceAttributeValueBlueZ& attr_bluez,
            size_t depth) {
  auto result = arc::mojom::BluetoothSdpAttribute::New();
  result->type = attr_bluez.type();
  result->type_size = 0;

  switch (result->type) {
    case bluez::BluetoothServiceAttributeValueBlueZ::NULLTYPE:
    case bluez::BluetoothServiceAttributeValueBlueZ::UINT:
    case bluez::BluetoothServiceAttributeValueBlueZ::INT:
    case bluez::BluetoothServiceAttributeValueBlueZ::UUID:
    case bluez::BluetoothServiceAttributeValueBlueZ::STRING:
    case bluez::BluetoothServiceAttributeValueBlueZ::URL:
    case bluez::BluetoothServiceAttributeValueBlueZ::BOOL: {
      result->type_size = attr_bluez.size();
      std::string json;
      base::JSONWriter::Write(attr_bluez.value(), &json);
      result->json_value = std::move(json);
      break;
    }
    case bluez::BluetoothServiceAttributeValueBlueZ::SEQUENCE:
      if (depth + 1 >= arc::kBluetoothSDPMaxDepth) {
        result->type = bluez::BluetoothServiceAttributeValueBlueZ::NULLTYPE;
        result->type_size = 0;
        return result;
      }
      for (const auto& child : attr_bluez.sequence()) {
        result->sequence.push_back(Convert(child, depth + 1));
      }
      result->type_size = result->sequence.size();
      break;
    default:
      NOTREACHED();
  }

  return result;
}

// static
bluez::BluetoothServiceAttributeValueBlueZ
TypeConverter<bluez::BluetoothServiceAttributeValueBlueZ,
              arc::mojom::BluetoothSdpAttributePtr>::
    Convert(const arc::mojom::BluetoothSdpAttributePtr& attr, size_t depth) {
  bluez::BluetoothServiceAttributeValueBlueZ::Type type = attr->type;

  switch (type) {
    case bluez::BluetoothServiceAttributeValueBlueZ::NULLTYPE:
    case bluez::BluetoothServiceAttributeValueBlueZ::UINT:
    case bluez::BluetoothServiceAttributeValueBlueZ::INT:
    case bluez::BluetoothServiceAttributeValueBlueZ::UUID:
    case bluez::BluetoothServiceAttributeValueBlueZ::STRING:
    case bluez::BluetoothServiceAttributeValueBlueZ::URL:
    case bluez::BluetoothServiceAttributeValueBlueZ::BOOL: {
      if (!attr->json_value.has_value()) {
        return bluez::BluetoothServiceAttributeValueBlueZ(
            bluez::BluetoothServiceAttributeValueBlueZ::NULLTYPE, 0,
            std::make_unique<base::Value>());
      }

      return bluez::BluetoothServiceAttributeValueBlueZ(
          type, static_cast<size_t>(attr->type_size),
          base::JSONReader::Read(attr->json_value.value()));
    }
    case bluez::BluetoothServiceAttributeValueBlueZ::SEQUENCE: {
      if (depth + 1 >= arc::kBluetoothSDPMaxDepth || attr->sequence.empty()) {
        return bluez::BluetoothServiceAttributeValueBlueZ(
            bluez::BluetoothServiceAttributeValueBlueZ::NULLTYPE, 0,
            std::make_unique<base::Value>());
      }

      auto bluez_sequence = std::make_unique<
          bluez::BluetoothServiceAttributeValueBlueZ::Sequence>();
      for (const auto& child : attr->sequence) {
        bluez_sequence->push_back(Convert(child, depth + 1));
      }
      return bluez::BluetoothServiceAttributeValueBlueZ(
          std::move(bluez_sequence));
      break;
    }
    default:
      NOTREACHED();
  }
  return bluez::BluetoothServiceAttributeValueBlueZ(
      bluez::BluetoothServiceAttributeValueBlueZ::NULLTYPE, 0,
      std::make_unique<base::Value>());
}

// static
arc::mojom::BluetoothSdpRecordPtr
TypeConverter<arc::mojom::BluetoothSdpRecordPtr,
              bluez::BluetoothServiceRecordBlueZ>::
    Convert(const bluez::BluetoothServiceRecordBlueZ& record_bluez) {
  arc::mojom::BluetoothSdpRecordPtr result =
      arc::mojom::BluetoothSdpRecord::New();

  for (auto id : record_bluez.GetAttributeIds()) {
    switch (id) {
      case kServiceClassIDList:
      case kProtocolDescriptorList:
      case kBrowseGroupList:
      case kBluetoothProfileDescriptorList:
      case kServiceName:
        result->attrs[id] = arc::mojom::BluetoothSdpAttribute::From(
            record_bluez.GetAttributeValue(id));
        break;
      default:
        // Android does not support this.
        break;
    }
  }

  return result;
}

// static
bluez::BluetoothServiceRecordBlueZ
TypeConverter<bluez::BluetoothServiceRecordBlueZ,
              arc::mojom::BluetoothSdpRecordPtr>::
    Convert(const arc::mojom::BluetoothSdpRecordPtr& record) {
  bluez::BluetoothServiceRecordBlueZ record_bluez;

  for (const auto& pair : record->attrs) {
    switch (pair.first) {
      case kServiceClassIDList:
      case kProtocolDescriptorList:
      case kBrowseGroupList:
      case kBluetoothProfileDescriptorList:
      case kServiceName:
        record_bluez.AddRecordEntry(
            pair.first,
            pair.second.To<bluez::BluetoothServiceAttributeValueBlueZ>());
        break;
      default:
        NOTREACHED();
        break;
    }
  }

  return record_bluez;
}

}  // namespace mojo
