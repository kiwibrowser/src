// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_WEBUSB_USB_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_WEBUSB_USB_H_

#include "device/usb/public/mojom/chooser_service.mojom-blink.h"
#include "device/usb/public/mojom/device_manager.mojom-blink.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/core/dom/context_lifecycle_observer.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class ScriptPromiseResolver;
class ScriptState;
class USBDevice;
class USBDeviceRequestOptions;

class USB final : public EventTargetWithInlineData,
                  public ContextLifecycleObserver,
                  public device::mojom::blink::UsbDeviceManagerClient {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(USB);
  USING_PRE_FINALIZER(USB, Dispose);

 public:
  static USB* Create(ExecutionContext& context) { return new USB(context); }

  ~USB() override;

  void Dispose();

  // USB.idl
  ScriptPromise getDevices(ScriptState*);
  ScriptPromise requestDevice(ScriptState*, const USBDeviceRequestOptions&);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(connect);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(disconnect);

  // EventTarget overrides.
  ExecutionContext* GetExecutionContext() const override;
  const AtomicString& InterfaceName() const override;

  // ContextLifecycleObserver overrides.
  void ContextDestroyed(ExecutionContext*) override;

  USBDevice* GetOrCreateDevice(device::mojom::blink::UsbDeviceInfoPtr);

  device::mojom::blink::UsbDeviceManager* GetDeviceManager() const {
    return device_manager_.get();
  }

  void OnGetDevices(ScriptPromiseResolver*,
                    Vector<device::mojom::blink::UsbDeviceInfoPtr>);
  void OnGetPermission(ScriptPromiseResolver*,
                       device::mojom::blink::UsbDeviceInfoPtr);

  // DeviceManagerClient implementation.
  void OnDeviceAdded(device::mojom::blink::UsbDeviceInfoPtr) override;
  void OnDeviceRemoved(device::mojom::blink::UsbDeviceInfoPtr) override;

  void OnDeviceManagerConnectionError();
  void OnChooserServiceConnectionError();

  void Trace(blink::Visitor*) override;

 protected:
  // EventTarget protected overrides.
  void AddedEventListener(const AtomicString& event_type,
                          RegisteredEventListener&) override;

 private:
  explicit USB(ExecutionContext&);

  void EnsureDeviceManagerConnection();

  bool IsContextSupported() const;
  bool IsFeatureEnabled() const;

  device::mojom::blink::UsbDeviceManagerPtr device_manager_;
  HeapHashSet<Member<ScriptPromiseResolver>> device_manager_requests_;
  device::mojom::blink::UsbChooserServicePtr chooser_service_;
  HeapHashSet<Member<ScriptPromiseResolver>> chooser_service_requests_;
  mojo::Binding<device::mojom::blink::UsbDeviceManagerClient> client_binding_;
  HeapHashMap<String, WeakMember<USBDevice>> device_cache_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_WEBUSB_USB_H_
