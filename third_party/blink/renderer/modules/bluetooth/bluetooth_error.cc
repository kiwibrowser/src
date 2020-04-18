// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/bluetooth/bluetooth_error.h"

#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"

namespace blink {

namespace {

const char kGATTServerNotConnectedBase[] =
    "GATT Server is disconnected. "
    "Cannot %s. (Re)connect first with `device.gatt.connect`.";

}  // namespace

// static
DOMException* BluetoothError::CreateNotConnectedException(
    BluetoothOperation operation) {
  const char* operation_string = nullptr;
  switch (operation) {
    case BluetoothOperation::kServicesRetrieval:
      operation_string = "retrieve services";
      break;
    case BluetoothOperation::kCharacteristicsRetrieval:
      operation_string = "retrieve characteristics";
      break;
    case BluetoothOperation::kDescriptorsRetrieval:
      operation_string = "retrieve descriptors";
      break;
    case BluetoothOperation::kGATT:
      operation_string = "perform GATT operations";
      break;
  }

  return DOMException::Create(
      kNetworkError,
      String::Format(kGATTServerNotConnectedBase, operation_string));
}

// static
DOMException* BluetoothError::CreateDOMException(
    BluetoothErrorCode error,
    const String& detailed_message) {
  switch (error) {
    case BluetoothErrorCode::kInvalidService:
    case BluetoothErrorCode::kInvalidCharacteristic:
    case BluetoothErrorCode::kInvalidDescriptor:
      return DOMException::Create(kInvalidStateError, detailed_message);
    case BluetoothErrorCode::kServiceNotFound:
    case BluetoothErrorCode::kCharacteristicNotFound:
    case BluetoothErrorCode::kDescriptorNotFound:
      return DOMException::Create(kNotFoundError, detailed_message);
  }
  NOTREACHED();
  return DOMException::Create(kUnknownError);
}

// static
DOMException* BluetoothError::CreateDOMException(
    mojom::blink::WebBluetoothResult error) {
  switch (error) {
    case mojom::blink::WebBluetoothResult::SUCCESS:
    case mojom::blink::WebBluetoothResult::SERVICE_NOT_FOUND:
    case mojom::blink::WebBluetoothResult::CHARACTERISTIC_NOT_FOUND:
    case mojom::blink::WebBluetoothResult::DESCRIPTOR_NOT_FOUND:
      // The above result codes are not expected here. SUCCESS is not
      // an error and the others have a detailed message and are
      // expected to be redirected to the switch above that handles
      // BluetoothErrorCode.
      NOTREACHED();
      return DOMException::Create(kUnknownError);
#define MAP_ERROR(enumeration, name, message)         \
  case mojom::blink::WebBluetoothResult::enumeration: \
    return DOMException::Create(name, message);

      // InvalidModificationErrors:
      MAP_ERROR(GATT_INVALID_ATTRIBUTE_LENGTH, kInvalidModificationError,
                "GATT Error: invalid attribute length.");

      // InvalidStateErrors:
      MAP_ERROR(SERVICE_NO_LONGER_EXISTS, kInvalidStateError,
                "GATT Service no longer exists.");
      MAP_ERROR(CHARACTERISTIC_NO_LONGER_EXISTS, kInvalidStateError,
                "GATT Characteristic no longer exists.");
      MAP_ERROR(DESCRIPTOR_NO_LONGER_EXISTS, kInvalidStateError,
                "GATT Descriptor no longer exists.");

      // NetworkErrors:
      MAP_ERROR(CONNECT_ALREADY_IN_PROGRESS, kNetworkError,
                "Connection already in progress.");
      MAP_ERROR(CONNECT_AUTH_CANCELED, kNetworkError,
                "Authentication canceled.");
      MAP_ERROR(CONNECT_AUTH_FAILED, kNetworkError, "Authentication failed.");
      MAP_ERROR(CONNECT_AUTH_REJECTED, kNetworkError,
                "Authentication rejected.");
      MAP_ERROR(CONNECT_AUTH_TIMEOUT, kNetworkError, "Authentication timeout.");
      MAP_ERROR(CONNECT_UNKNOWN_ERROR, kNetworkError,
                "Unknown error when connecting to the device.");
      MAP_ERROR(CONNECT_UNKNOWN_FAILURE, kNetworkError,
                "Connection failed for unknown reason.");
      MAP_ERROR(CONNECT_UNSUPPORTED_DEVICE, kNetworkError,
                "Unsupported device.");
      MAP_ERROR(DEVICE_NO_LONGER_IN_RANGE, kNetworkError,
                "Bluetooth Device is no longer in range.");
      MAP_ERROR(GATT_NOT_PAIRED, kNetworkError, "GATT Error: Not paired.");
      MAP_ERROR(GATT_OPERATION_IN_PROGRESS, kNetworkError,
                "GATT operation already in progress.");

      // NotFoundErrors:
      MAP_ERROR(WEB_BLUETOOTH_NOT_SUPPORTED, kNotFoundError,
                "Web Bluetooth is not supported on this platform. For a list "
                "of supported platforms see: https://goo.gl/J6ASzs");
      MAP_ERROR(NO_BLUETOOTH_ADAPTER, kNotFoundError,
                "Bluetooth adapter not available.");
      MAP_ERROR(CHOSEN_DEVICE_VANISHED, kNotFoundError,
                "User selected a device that doesn't exist anymore.");
      MAP_ERROR(CHOOSER_CANCELLED, kNotFoundError,
                "User cancelled the requestDevice() chooser.");
      MAP_ERROR(CHOOSER_NOT_SHOWN_API_GLOBALLY_DISABLED, kNotFoundError,
                "Web Bluetooth API globally disabled.");
      MAP_ERROR(CHOOSER_NOT_SHOWN_API_LOCALLY_DISABLED, kNotFoundError,
                "User or their enterprise policy has disabled Web Bluetooth.");
      MAP_ERROR(
          CHOOSER_NOT_SHOWN_USER_DENIED_PERMISSION_TO_SCAN, kNotFoundError,
          "User denied the browser permission to scan for Bluetooth devices.");
      MAP_ERROR(NO_SERVICES_FOUND, kNotFoundError,
                "No Services found in device.");
      MAP_ERROR(NO_CHARACTERISTICS_FOUND, kNotFoundError,
                "No Characteristics found in service.");
      MAP_ERROR(NO_DESCRIPTORS_FOUND, kNotFoundError,
                "No Descriptors found in Characteristic.");
      MAP_ERROR(BLUETOOTH_LOW_ENERGY_NOT_AVAILABLE, kNotFoundError,
                "Bluetooth Low Energy not available.");

      // NotSupportedErrors:
      MAP_ERROR(GATT_UNKNOWN_ERROR, kNotSupportedError, "GATT Error Unknown.");
      MAP_ERROR(GATT_UNKNOWN_FAILURE, kNotSupportedError,
                "GATT operation failed for unknown reason.");
      MAP_ERROR(GATT_NOT_PERMITTED, kNotSupportedError,
                "GATT operation not permitted.");
      MAP_ERROR(GATT_NOT_SUPPORTED, kNotSupportedError,
                "GATT Error: Not supported.");
      MAP_ERROR(GATT_UNTRANSLATED_ERROR_CODE, kNotSupportedError,
                "GATT Error: Unknown GattErrorCode.");

      // SecurityErrors:
      MAP_ERROR(GATT_NOT_AUTHORIZED, kSecurityError,
                "GATT operation not authorized.");
      MAP_ERROR(BLOCKLISTED_CHARACTERISTIC_UUID, kSecurityError,
                "getCharacteristic(s) called with blocklisted UUID. "
                "https://goo.gl/4NeimX");
      MAP_ERROR(BLOCKLISTED_DESCRIPTOR_UUID, kSecurityError,
                "getDescriptor(s) called with blocklisted UUID. "
                "https://goo.gl/4NeimX");
      MAP_ERROR(BLOCKLISTED_READ, kSecurityError,
                "readValue() called on blocklisted object marked "
                "exclude-reads. https://goo.gl/4NeimX");
      MAP_ERROR(BLOCKLISTED_WRITE, kSecurityError,
                "writeValue() called on blocklisted object marked "
                "exclude-writes. https://goo.gl/4NeimX");
      MAP_ERROR(NOT_ALLOWED_TO_ACCESS_ANY_SERVICE, kSecurityError,
                "Origin is not allowed to access any service. Tip: Add the "
                "service UUID to 'optionalServices' in requestDevice() "
                "options. https://goo.gl/HxfxSQ");
      MAP_ERROR(NOT_ALLOWED_TO_ACCESS_SERVICE, kSecurityError,
                "Origin is not allowed to access the service. Tip: Add the "
                "service UUID to 'optionalServices' in requestDevice() "
                "options. https://goo.gl/HxfxSQ");
      MAP_ERROR(REQUEST_DEVICE_WITH_BLOCKLISTED_UUID, kSecurityError,
                "requestDevice() called with a filter containing a blocklisted "
                "UUID. https://goo.gl/4NeimX");
      MAP_ERROR(REQUEST_DEVICE_FROM_CROSS_ORIGIN_IFRAME, kSecurityError,
                "requestDevice() called from cross-origin iframe.");

#undef MAP_ERROR
  }

  NOTREACHED();
  return DOMException::Create(kUnknownError);
}

}  // namespace blink
