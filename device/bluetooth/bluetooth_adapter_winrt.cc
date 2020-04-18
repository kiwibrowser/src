// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter_winrt.h"

#include <windows.foundation.h>
#include <wrl/event.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/core_winrt_util.h"
#include "device/bluetooth/bluetooth_discovery_filter.h"

namespace device {

namespace {

// In order to avoid a name clash with device::BluetoothAdapter we need this
// auxiliary namespace.
namespace uwp {
using ABI::Windows::Devices::Bluetooth::BluetoothAdapter;
}  // namespace uwp
using ABI::Windows::Devices::Bluetooth::IBluetoothAdapter;
using ABI::Windows::Devices::Bluetooth::IBluetoothAdapterStatics;
using ABI::Windows::Devices::Enumeration::DeviceInformation;
using ABI::Windows::Devices::Enumeration::IDeviceInformation;
using ABI::Windows::Devices::Enumeration::IDeviceInformationStatics;
using ABI::Windows::Foundation::IAsyncOperation;
using ABI::Windows::Foundation::IAsyncOperationCompletedHandler;
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

bool ResolveCoreWinRT() {
  return base::win::ResolveCoreWinRTDelayload() &&
         base::win::ScopedHString::ResolveCoreWinRTStringDelayload();
}

// Utility functions to pretty print enum values.
constexpr const char* ToCString(AsyncStatus async_status) {
  switch (async_status) {
    case AsyncStatus::Started:
      return "AsyncStatus::Started";
    case AsyncStatus::Completed:
      return "AsyncStatus::Completed";
    case AsyncStatus::Canceled:
      return "AsyncStatus::Canceled";
    case AsyncStatus::Error:
      return "AsyncStatus::Error";
  }

  NOTREACHED();
  return "";
}

template <typename T>
using AsyncAbiT = typename ABI::Windows::Foundation::Internal::GetAbiType<
    typename IAsyncOperation<T>::TResult_complex>::type;

// Compile time switch to decide what container to use for the async results for
// |T|. Depends on whether the underlying Abi type is a pointer to IUnknown or
// not. It queries the internals of Windows::Foundation to obtain this
// information.
template <typename T>
using AsyncResultsT =
    std::conditional_t<std::is_convertible<AsyncAbiT<T>, IUnknown*>::value,
                       ComPtr<std::remove_pointer_t<AsyncAbiT<T>>>,
                       AsyncAbiT<T>>;

// Obtains the results of the provided async operation. Returns it if the
// operation completed successfully.
template <typename T>
AsyncResultsT<T> GetAsyncResults(IAsyncOperation<T>* async_op,
                                 AsyncStatus async_status) {
  if (async_status != AsyncStatus::Completed) {
    VLOG(2) << "Got unexpected AsyncStatus: " << ToCString(async_status);
    return {};
  }

  AsyncResultsT<T> results;
  HRESULT hr = async_op->GetResults(&results);
  if (FAILED(hr)) {
    VLOG(2) << "GotAsyncResults failed: "
            << logging::SystemErrorCodeToString(hr);
  }

  return results;
}

// This method registers a completion handler for |async_op| and will post the
// results to |callback| on |task_runner|. While a Callback can be constructed
// from callable types such as a lambda or std::function objects, it cannot be
// directly constructed from a base::OnceCallback. Thus the callback is moved
// into a capturing lambda, which then posts the callback once it is run.
// Posting the results to the TaskRunner is required, since the completion
// callback might be invoked on an arbitrary thread. Lastly, the lambda takes
// ownership of |async_op|, as this needs to be kept alive until GetAsyncResults
// can be invoked. Once that is done it is safe to release the |async_op|.
template <typename T>
HRESULT PostAsyncResults(ComPtr<IAsyncOperation<T>> async_op,
                         scoped_refptr<base::TaskRunner> task_runner,
                         base::OnceCallback<void(AsyncResultsT<T>)> callback) {
  auto async_op_raw = async_op.Get();
  return async_op_raw->put_Completed(
      Callback<IAsyncOperationCompletedHandler<T>>([
        async_op(std::move(async_op)), task_runner(std::move(task_runner)),
        callback(std::move(callback))
      ](IAsyncOperation<T> * async_op_raw, AsyncStatus async_status) mutable {
        // Note: We are using |async_op_raw| instead of async_op.Get(), as this
        // could be executed on any thread and only |async_op_raw| is guaranteed
        // to be in the correct COM apartment.
        task_runner->PostTask(
            FROM_HERE,
            base::BindOnce(std::move(callback),
                           GetAsyncResults(async_op_raw, async_status)));
        async_op.Reset();
        return S_OK;
      })
          .Get());
}

}  // namespace

std::string BluetoothAdapterWinrt::GetAddress() const {
  return address_;
}

std::string BluetoothAdapterWinrt::GetName() const {
  return name_;
}

void BluetoothAdapterWinrt::SetName(const std::string& name,
                                    const base::Closure& callback,
                                    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterWinrt::IsInitialized() const {
  return is_initialized_;
}

bool BluetoothAdapterWinrt::IsPresent() const {
  return is_present_;
}

bool BluetoothAdapterWinrt::IsPowered() const {
  // Note: While the UWP APIs can provide access to the underlying radio [1] and
  // its power state [2], this feature is not available for x86 Apps [3]. Thus
  // we simply assume the adapter is powered if it is present.
  //
  // [1]
  // https://docs.microsoft.com/en-us/uwp/api/windows.devices.bluetooth.bluetoothadapter.getradioasync
  // [2]
  // https://docs.microsoft.com/en-us/uwp/api/windows.devices.radios.radiostate
  // [3] https://github.com/Microsoft/cppwinrt/issues/47#issuecomment-335181782
  if (IsPresent()) {
    LOG(WARNING) << "Optimistically assuming the adapter is powered since it "
                    "is present. This might not actually be true.";
  }

  return IsPresent();
}

bool BluetoothAdapterWinrt::IsDiscoverable() const {
  NOTIMPLEMENTED();
  return false;
}

void BluetoothAdapterWinrt::SetDiscoverable(
    bool discoverable,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

bool BluetoothAdapterWinrt::IsDiscovering() const {
  NOTIMPLEMENTED();
  return false;
}

BluetoothAdapter::UUIDList BluetoothAdapterWinrt::GetUUIDs() const {
  NOTIMPLEMENTED();
  return UUIDList();
}

void BluetoothAdapterWinrt::CreateRfcommService(
    const BluetoothUUID& uuid,
    const ServiceOptions& options,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothAdapterWinrt::CreateL2capService(
    const BluetoothUUID& uuid,
    const ServiceOptions& options,
    const CreateServiceCallback& callback,
    const CreateServiceErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothAdapterWinrt::RegisterAdvertisement(
    std::unique_ptr<BluetoothAdvertisement::Data> advertisement_data,
    const CreateAdvertisementCallback& callback,
    const AdvertisementErrorCallback& error_callback) {
  NOTIMPLEMENTED();
}

BluetoothLocalGattService* BluetoothAdapterWinrt::GetGattService(
    const std::string& identifier) const {
  NOTIMPLEMENTED();
  return nullptr;
}

BluetoothAdapterWinrt::BluetoothAdapterWinrt() : weak_ptr_factory_(this) {
  ui_task_runner_ = base::ThreadTaskRunnerHandle::Get();
}

BluetoothAdapterWinrt::~BluetoothAdapterWinrt() = default;

void BluetoothAdapterWinrt::Init(InitCallback init_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // We are wrapping |init_cb| in a ScopedClosureRunner to ensure it gets run no
  // matter how the function exits. Furthermore, we set |is_initialized_| to
  // true if adapter is still active when the callback gets run.
  base::ScopedClosureRunner on_init(base::BindOnce(
      [](base::WeakPtr<BluetoothAdapterWinrt> adapter, InitCallback init_cb) {
        if (adapter)
          adapter->is_initialized_ = true;
        std::move(init_cb).Run();
      },
      weak_ptr_factory_.GetWeakPtr(), std::move(init_cb)));

  if (!ResolveCoreWinRT())
    return;

  ComPtr<IBluetoothAdapterStatics> adapter_statics;
  HRESULT hr = GetBluetoothAdapterStaticsActivationFactory(&adapter_statics);
  if (FAILED(hr)) {
    VLOG(2) << "GetBluetoothAdapterStaticsActivationFactory failed: "
            << logging::SystemErrorCodeToString(hr);
    return;
  }

  ComPtr<IAsyncOperation<uwp::BluetoothAdapter*>> get_default_adapter_op;
  hr = adapter_statics->GetDefaultAsync(&get_default_adapter_op);
  if (FAILED(hr)) {
    VLOG(2) << "BluetoothAdapter::GetDefaultAsync failed: "
            << logging::SystemErrorCodeToString(hr);
    return;
  }

  hr = PostAsyncResults(
      std::move(get_default_adapter_op), ui_task_runner_,
      base::BindOnce(&BluetoothAdapterWinrt::OnGetDefaultAdapter,
                     weak_ptr_factory_.GetWeakPtr(), std::move(on_init)));

  if (FAILED(hr)) {
    VLOG(2) << "PostAsyncResults failed: "
            << logging::SystemErrorCodeToString(hr);
  }
}

bool BluetoothAdapterWinrt::SetPoweredImpl(bool powered) {
  NOTIMPLEMENTED();
  return false;
}

void BluetoothAdapterWinrt::AddDiscoverySession(
    BluetoothDiscoveryFilter* discovery_filter,
    const base::Closure& callback,
    DiscoverySessionErrorCallback error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothAdapterWinrt::RemoveDiscoverySession(
    BluetoothDiscoveryFilter* discovery_filter,
    const base::Closure& callback,
    DiscoverySessionErrorCallback error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothAdapterWinrt::SetDiscoveryFilter(
    std::unique_ptr<BluetoothDiscoveryFilter> discovery_filter,
    const base::Closure& callback,
    DiscoverySessionErrorCallback error_callback) {
  NOTIMPLEMENTED();
}

void BluetoothAdapterWinrt::RemovePairingDelegateInternal(
    BluetoothDevice::PairingDelegate* pairing_delegate) {
  NOTIMPLEMENTED();
}

HRESULT BluetoothAdapterWinrt::GetBluetoothAdapterStaticsActivationFactory(
    IBluetoothAdapterStatics** statics) const {
  return base::win::GetActivationFactory<
      IBluetoothAdapterStatics,
      RuntimeClass_Windows_Devices_Bluetooth_BluetoothAdapter>(statics);
}

HRESULT BluetoothAdapterWinrt::GetDeviceInformationStaticsActivationFactory(
    IDeviceInformationStatics** statics) const {
  return base::win::GetActivationFactory<
      IDeviceInformationStatics,
      RuntimeClass_Windows_Devices_Enumeration_DeviceInformation>(statics);
}

void BluetoothAdapterWinrt::OnGetDefaultAdapter(
    base::ScopedClosureRunner on_init,
    ComPtr<IBluetoothAdapter> adapter) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!adapter) {
    VLOG(2) << "Getting Default Adapter failed.";
    return;
  }

  // Obtaining the default adapter will fail if no physical adapter is present.
  // Thus a non-zero |adapter| implies that a physical adapter is present.
  is_present_ = true;
  uint64_t raw_address;
  HRESULT hr = adapter->get_BluetoothAddress(&raw_address);
  if (FAILED(hr)) {
    VLOG(2) << "Getting BluetoothAddress failed: "
            << logging::SystemErrorCodeToString(hr);
    return;
  }

  address_ = BluetoothDevice::CanonicalizeAddress(
      base::StringPrintf("%012llX", raw_address));
  DCHECK(!address_.empty());

  HSTRING device_id;
  hr = adapter->get_DeviceId(&device_id);
  if (FAILED(hr)) {
    VLOG(2) << "Getting DeviceId failed: "
            << logging::SystemErrorCodeToString(hr);
    return;
  }

  ComPtr<IDeviceInformationStatics> device_information_statics;
  hr =
      GetDeviceInformationStaticsActivationFactory(&device_information_statics);
  if (FAILED(hr)) {
    VLOG(2) << "GetDeviceInformationStaticsActivationFactory failed: "
            << logging::SystemErrorCodeToString(hr);
    return;
  }

  ComPtr<IAsyncOperation<DeviceInformation*>> create_from_id_op;
  hr = device_information_statics->CreateFromIdAsync(device_id,
                                                     &create_from_id_op);
  if (FAILED(hr)) {
    VLOG(2) << "CreateFromIdAsync failed: "
            << logging::SystemErrorCodeToString(hr);
    return;
  }

  hr = PostAsyncResults(
      std::move(create_from_id_op), ui_task_runner_,
      base::BindOnce(&BluetoothAdapterWinrt::OnCreateFromIdAsync,
                     weak_ptr_factory_.GetWeakPtr(), std::move(on_init)));
  if (FAILED(hr)) {
    VLOG(2) << "PostAsyncResults failed: "
            << logging::SystemErrorCodeToString(hr);
  }
}

void BluetoothAdapterWinrt::OnCreateFromIdAsync(
    base::ScopedClosureRunner on_init,
    ComPtr<IDeviceInformation> device_information) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!device_information) {
    VLOG(2) << "Getting Device Information failed.";
    return;
  }

  HSTRING name;
  HRESULT hr = device_information->get_Name(&name);
  if (FAILED(hr)) {
    VLOG(2) << "Getting Name failed: " << logging::SystemErrorCodeToString(hr);
    return;
  }

  name_ = base::win::ScopedHString(name).GetAsUTF8();
}

}  // namespace device
