// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/webusb/usb.h"

#include <utility>

#include "device/usb/public/mojom/device.mojom-blink.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/webusb/usb_connection_event.h"
#include "third_party/blink/renderer/modules/webusb/usb_device.h"
#include "third_party/blink/renderer/modules/webusb/usb_device_filter.h"
#include "third_party/blink/renderer/modules/webusb/usb_device_request_options.h"
#include "third_party/blink/renderer/platform/feature_policy/feature_policy.h"
#include "third_party/blink/renderer/platform/mojo/mojo_helper.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

using device::mojom::blink::UsbDeviceFilterPtr;
using device::mojom::blink::UsbDeviceInfoPtr;
using device::mojom::blink::UsbDevicePtr;

namespace blink {
namespace {

const char kFeaturePolicyBlocked[] =
    "Access to the feature \"usb\" is disallowed by feature policy.";
const char kIframeBlocked[] =
    "Access to this method is not allowed in embedded frames.";
const char kNoDeviceSelected[] = "No device selected.";

UsbDeviceFilterPtr ConvertDeviceFilter(const USBDeviceFilter& filter) {
  auto mojo_filter = device::mojom::blink::UsbDeviceFilter::New();
  mojo_filter->has_vendor_id = filter.hasVendorId();
  if (mojo_filter->has_vendor_id)
    mojo_filter->vendor_id = filter.vendorId();
  mojo_filter->has_product_id = filter.hasProductId();
  if (mojo_filter->has_product_id)
    mojo_filter->product_id = filter.productId();
  mojo_filter->has_class_code = filter.hasClassCode();
  if (mojo_filter->has_class_code)
    mojo_filter->class_code = filter.classCode();
  mojo_filter->has_subclass_code = filter.hasSubclassCode();
  if (mojo_filter->has_subclass_code)
    mojo_filter->subclass_code = filter.subclassCode();
  mojo_filter->has_protocol_code = filter.hasProtocolCode();
  if (mojo_filter->has_protocol_code)
    mojo_filter->protocol_code = filter.protocolCode();
  if (filter.hasSerialNumber())
    mojo_filter->serial_number = filter.serialNumber();
  return mojo_filter;
}

}  // namespace

USB::USB(ExecutionContext& context)
    : ContextLifecycleObserver(&context), client_binding_(this) {}

USB::~USB() {
  // |m_deviceManager| and |m_chooserService| may still be valid but there
  // should be no more outstanding requests to them because each holds a
  // persistent handle to this object.
  DCHECK(device_manager_requests_.IsEmpty());
  DCHECK(chooser_service_requests_.IsEmpty());
}

void USB::Dispose() {
  // The pipe to this object must be closed when is marked unreachable to
  // prevent messages from being dispatched before lazy sweeping.
  client_binding_.Close();
}

ScriptPromise USB::getDevices(ScriptState* script_state) {
  if (!IsContextSupported()) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kNotSupportedError));
  }
  if (!IsFeatureEnabled()) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kSecurityError, kFeaturePolicyBlocked));
  }

  EnsureDeviceManagerConnection();
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  device_manager_requests_.insert(resolver);
  device_manager_->GetDevices(
      nullptr, WTF::Bind(&USB::OnGetDevices, WrapPersistent(this),
                         WrapPersistent(resolver)));
  return resolver->Promise();
}

ScriptPromise USB::requestDevice(ScriptState* script_state,
                                 const USBDeviceRequestOptions& options) {
  LocalFrame* frame = GetFrame();
  if (!frame) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kNotSupportedError));
  }

  if (IsSupportedInFeaturePolicy(mojom::FeaturePolicyFeature::kUsb)) {
    if (!frame->IsFeatureEnabled(mojom::FeaturePolicyFeature::kUsb)) {
      return ScriptPromise::RejectWithDOMException(
          script_state,
          DOMException::Create(kSecurityError, kFeaturePolicyBlocked));
    }
  } else if (!frame->IsMainFrame()) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kSecurityError, kIframeBlocked));
  }

  if (!chooser_service_) {
    GetFrame()->GetInterfaceProvider().GetInterface(
        mojo::MakeRequest(&chooser_service_));
    chooser_service_.set_connection_error_handler(WTF::Bind(
        &USB::OnChooserServiceConnectionError, WrapWeakPersistent(this)));
  }

  if (!Frame::HasTransientUserActivation(frame)) {
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(
            kSecurityError,
            "Must be handling a user gesture to show a permission request."));
  }

  Vector<UsbDeviceFilterPtr> filters;
  if (options.hasFilters()) {
    filters.ReserveCapacity(options.filters().size());
    for (const auto& filter : options.filters())
      filters.push_back(ConvertDeviceFilter(filter));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  chooser_service_requests_.insert(resolver);
  chooser_service_->GetPermission(
      std::move(filters), WTF::Bind(&USB::OnGetPermission, WrapPersistent(this),
                                    WrapPersistent(resolver)));
  return resolver->Promise();
}

ExecutionContext* USB::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

const AtomicString& USB::InterfaceName() const {
  return EventTargetNames::USB;
}

void USB::ContextDestroyed(ExecutionContext*) {
  device_manager_.reset();
  device_manager_requests_.clear();
  chooser_service_.reset();
  chooser_service_requests_.clear();
}

USBDevice* USB::GetOrCreateDevice(UsbDeviceInfoPtr device_info) {
  USBDevice* device = device_cache_.at(device_info->guid);
  if (!device) {
    String guid = device_info->guid;
    UsbDevicePtr pipe;
    device_manager_->GetDevice(guid, mojo::MakeRequest(&pipe));
    device = USBDevice::Create(std::move(device_info), std::move(pipe),
                               GetExecutionContext());
    device_cache_.insert(guid, device);
  }
  return device;
}

void USB::OnGetDevices(ScriptPromiseResolver* resolver,
                       Vector<UsbDeviceInfoPtr> device_infos) {
  auto request_entry = device_manager_requests_.find(resolver);
  if (request_entry == device_manager_requests_.end())
    return;
  device_manager_requests_.erase(request_entry);

  HeapVector<Member<USBDevice>> devices;
  for (auto& device_info : device_infos)
    devices.push_back(GetOrCreateDevice(std::move(device_info)));
  resolver->Resolve(devices);
  device_manager_requests_.erase(resolver);
}

void USB::OnGetPermission(ScriptPromiseResolver* resolver,
                          UsbDeviceInfoPtr device_info) {
  auto request_entry = chooser_service_requests_.find(resolver);
  if (request_entry == chooser_service_requests_.end())
    return;
  chooser_service_requests_.erase(request_entry);

  EnsureDeviceManagerConnection();

  if (device_manager_ && device_info)
    resolver->Resolve(GetOrCreateDevice(std::move(device_info)));
  else
    resolver->Reject(DOMException::Create(kNotFoundError, kNoDeviceSelected));
}

void USB::OnDeviceAdded(UsbDeviceInfoPtr device_info) {
  if (!device_manager_)
    return;

  DispatchEvent(USBConnectionEvent::Create(
      EventTypeNames::connect, GetOrCreateDevice(std::move(device_info))));
}

void USB::OnDeviceRemoved(UsbDeviceInfoPtr device_info) {
  String guid = device_info->guid;
  USBDevice* device = device_cache_.at(guid);
  if (!device) {
    device = USBDevice::Create(std::move(device_info), nullptr,
                               GetExecutionContext());
  }
  DispatchEvent(USBConnectionEvent::Create(EventTypeNames::disconnect, device));
  device_cache_.erase(guid);
}

void USB::OnDeviceManagerConnectionError() {
  device_manager_.reset();
  client_binding_.Close();
  for (ScriptPromiseResolver* resolver : device_manager_requests_)
    resolver->Resolve(HeapVector<Member<USBDevice>>(0));
  device_manager_requests_.clear();
}

void USB::OnChooserServiceConnectionError() {
  chooser_service_.reset();
  for (ScriptPromiseResolver* resolver : chooser_service_requests_)
    resolver->Reject(DOMException::Create(kNotFoundError, kNoDeviceSelected));
  chooser_service_requests_.clear();
}

void USB::AddedEventListener(const AtomicString& event_type,
                             RegisteredEventListener& listener) {
  EventTargetWithInlineData::AddedEventListener(event_type, listener);
  if (event_type != EventTypeNames::connect &&
      event_type != EventTypeNames::disconnect) {
    return;
  }

  if (!IsContextSupported() || !IsFeatureEnabled())
    return;

  EnsureDeviceManagerConnection();
}

void USB::EnsureDeviceManagerConnection() {
  if (device_manager_)
    return;

  DCHECK(IsContextSupported());
  DCHECK(IsFeatureEnabled());
  GetExecutionContext()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&device_manager_));
  device_manager_.set_connection_error_handler(WTF::Bind(
      &USB::OnDeviceManagerConnectionError, WrapWeakPersistent(this)));

  DCHECK(!client_binding_.is_bound());

  device::mojom::blink::UsbDeviceManagerClientPtr client;
  client_binding_.Bind(mojo::MakeRequest(&client));
  device_manager_->SetClient(std::move(client));
}

bool USB::IsContextSupported() const {
  ExecutionContext* context = GetExecutionContext();
  if (!context)
    return false;

  if (!(context->IsDedicatedWorkerGlobalScope() ||
        context->IsSharedWorkerGlobalScope() || context->IsDocument()))
    return false;

  // Since WebUSB on Web Workers is in the process of being implemented, we
  // check here if the runtime flag for this feature is enabled.
  // TODO(https://crbug.com/837406): Remove this check once the feature has
  // shipped.
  if (!context->IsDocument() &&
      !RuntimeEnabledFeatures::WebUSBOnDedicatedAndSharedWorkersEnabled())
    return false;

  return true;
}

bool USB::IsFeatureEnabled() const {
  // At the moment, FeaturePolicy is not supported in workers, so we skip the
  // check on workers.
  // TODO(https://crbug.com/843780): Enable the FeaturePolicy check for the
  // supported worker contexts once it is available for workers.
  if (GetExecutionContext()->IsDocument()) {
    FeaturePolicy* policy =
        GetExecutionContext()->GetSecurityContext().GetFeaturePolicy();
    return policy->IsFeatureEnabled(mojom::FeaturePolicyFeature::kUsb);
  }
  if (GetExecutionContext()->IsDedicatedWorkerGlobalScope() ||
      GetExecutionContext()->IsSharedWorkerGlobalScope()) {
    return true;
  }
  return false;
}

void USB::Trace(blink::Visitor* visitor) {
  visitor->Trace(device_manager_requests_);
  visitor->Trace(chooser_service_requests_);
  visitor->Trace(device_cache_);
  EventTargetWithInlineData::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
