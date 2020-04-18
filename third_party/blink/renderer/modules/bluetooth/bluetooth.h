// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BLUETOOTH_BLUETOOTH_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BLUETOOTH_BLUETOOTH_H_

#include <memory>
#include "third_party/blink/public/platform/modules/bluetooth/web_bluetooth.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/modules/bluetooth/bluetooth_device.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class RequestDeviceOptions;
class ScriptPromise;
class ScriptState;

class Bluetooth final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Bluetooth* Create() { return new Bluetooth(); }

  // IDL exposed interface:
  ScriptPromise requestDevice(ScriptState*,
                              const RequestDeviceOptions&,
                              ExceptionState&);

  mojom::blink::WebBluetoothService* Service() { return service_.get(); }

  // Interface required by Garbage Collection:
  void Trace(blink::Visitor*) override;

 private:
  Bluetooth();

  BluetoothDevice* GetBluetoothDeviceRepresentingDevice(
      mojom::blink::WebBluetoothDevicePtr,
      ScriptPromiseResolver*);

  void RequestDeviceCallback(ScriptPromiseResolver*,
                             mojom::blink::WebBluetoothResult,
                             mojom::blink::WebBluetoothDevicePtr);

  // Map of device ids to BluetoothDevice objects.
  // Ensures only one BluetoothDevice instance represents each
  // Bluetooth device inside a single global object.
  HeapHashMap<String, Member<BluetoothDevice>> device_instance_map_;

  mojom::blink::WebBluetoothServicePtr service_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BLUETOOTH_BLUETOOTH_H_
