// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/bluetooth/bluetooth_remote_gatt_descriptor.h"

#include <memory>
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/modules/bluetooth/bluetooth_error.h"
#include "third_party/blink/renderer/modules/bluetooth/bluetooth_remote_gatt_service.h"
#include "third_party/blink/renderer/modules/bluetooth/bluetooth_remote_gatt_utils.h"

namespace blink {

BluetoothRemoteGATTDescriptor::BluetoothRemoteGATTDescriptor(
    mojom::blink::WebBluetoothRemoteGATTDescriptorPtr descriptor,
    BluetoothRemoteGATTCharacteristic* characteristic)
    : descriptor_(std::move(descriptor)), characteristic_(characteristic) {}

BluetoothRemoteGATTDescriptor* BluetoothRemoteGATTDescriptor::Create(
    mojom::blink::WebBluetoothRemoteGATTDescriptorPtr descriptor,

    BluetoothRemoteGATTCharacteristic* characteristic) {
  BluetoothRemoteGATTDescriptor* result =
      new BluetoothRemoteGATTDescriptor(std::move(descriptor), characteristic);
  return result;
}

void BluetoothRemoteGATTDescriptor::ReadValueCallback(
    ScriptPromiseResolver* resolver,
    mojom::blink::WebBluetoothResult result,
    const base::Optional<Vector<uint8_t>>& value) {
  if (!resolver->GetExecutionContext() ||
      resolver->GetExecutionContext()->IsContextDestroyed())
    return;

  // If the device is disconnected, reject.
  if (!GetGatt()->RemoveFromActiveAlgorithms(resolver)) {
    resolver->Reject(
        BluetoothError::CreateNotConnectedException(BluetoothOperation::kGATT));
    return;
  }

  if (result == mojom::blink::WebBluetoothResult::SUCCESS) {
    DCHECK(value);
    DOMDataView* dom_data_view =
        BluetoothRemoteGATTUtils::ConvertWTFVectorToDataView(value.value());
    value_ = dom_data_view;
    resolver->Resolve(dom_data_view);
  } else {
    resolver->Reject(BluetoothError::CreateDOMException(result));
  }
}

ScriptPromise BluetoothRemoteGATTDescriptor::readValue(
    ScriptState* script_state) {
  if (!GetGatt()->connected()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        BluetoothError::CreateNotConnectedException(BluetoothOperation::kGATT));
  }

  if (!GetGatt()->device()->IsValidDescriptor(descriptor_->instance_id)) {
    return ScriptPromise::RejectWithDOMException(
        script_state, CreateInvalidDescriptorError());
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  GetGatt()->AddToActiveAlgorithms(resolver);
  GetService()->RemoteDescriptorReadValue(
      descriptor_->instance_id,
      WTF::Bind(&BluetoothRemoteGATTDescriptor::ReadValueCallback,
                WrapPersistent(this), WrapPersistent(resolver)));

  return promise;
}

void BluetoothRemoteGATTDescriptor::WriteValueCallback(
    ScriptPromiseResolver* resolver,
    const Vector<uint8_t>& value,
    mojom::blink::WebBluetoothResult result) {
  if (!resolver->GetExecutionContext() ||
      resolver->GetExecutionContext()->IsContextDestroyed())
    return;

  // If the resolver is not in the set of ActiveAlgorithms then the frame
  // disconnected so we reject.
  if (!GetGatt()->RemoveFromActiveAlgorithms(resolver)) {
    resolver->Reject(
        BluetoothError::CreateNotConnectedException(BluetoothOperation::kGATT));
    return;
  }

  if (result == mojom::blink::WebBluetoothResult::SUCCESS) {
    value_ = BluetoothRemoteGATTUtils::ConvertWTFVectorToDataView(value);
    resolver->Resolve();
  } else {
    resolver->Reject(BluetoothError::CreateDOMException(result));
  }
}

ScriptPromise BluetoothRemoteGATTDescriptor::writeValue(
    ScriptState* script_state,
    const DOMArrayPiece& value) {
  if (!GetGatt()->connected()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        BluetoothError::CreateNotConnectedException(BluetoothOperation::kGATT));
  }

  if (!GetGatt()->device()->IsValidDescriptor(descriptor_->instance_id)) {
    return ScriptPromise::RejectWithDOMException(
        script_state, CreateInvalidDescriptorError());
  }

  // Partial implementation of writeValue algorithm:
  // https://webbluetoothcg.github.io/web-bluetooth/#dom-bluetoothremotegattdescriptor-writevalue

  // If bytes is more than 512 bytes long (the maximum length of an attribute
  // value, per Long Attribute Values) return a promise rejected with an
  // InvalidModificationError and abort.
  if (value.ByteLength() > 512) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kInvalidModificationError,
                                           "Value can't exceed 512 bytes."));
  }

  // Let valueVector be a copy of the bytes held by value.
  Vector<uint8_t> value_vector;
  value_vector.Append(value.Bytes(), value.ByteLength());

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  GetGatt()->AddToActiveAlgorithms(resolver);
  GetService()->RemoteDescriptorWriteValue(
      descriptor_->instance_id, value_vector,
      WTF::Bind(&BluetoothRemoteGATTDescriptor::WriteValueCallback,
                WrapPersistent(this), WrapPersistent(resolver), value_vector));

  return promise;
}

DOMException* BluetoothRemoteGATTDescriptor::CreateInvalidDescriptorError() {
  return BluetoothError::CreateDOMException(
      BluetoothErrorCode::kInvalidDescriptor,
      "Descriptor with UUID " + uuid() +
          " is no longer valid. Remember to retrieve the Descriptor again "
          "after reconnecting.");
}

void BluetoothRemoteGATTDescriptor::Trace(blink::Visitor* visitor) {
  visitor->Trace(characteristic_);
  visitor->Trace(value_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
