// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/concierge_client.h"

#include "base/bind.h"
#include "base/observer_list.h"
#include "base/threading/thread_task_runner_handle.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "third_party/cros_system_api/dbus/vm_concierge/dbus-constants.h"

namespace chromeos {

class ConciergeClientImpl : public ConciergeClient {
 public:
  ConciergeClientImpl() : weak_ptr_factory_(this) {}

  ~ConciergeClientImpl() override = default;

  // ConciergeClient override.
  void AddObserver(Observer* observer) override {
    observer_list_.AddObserver(observer);
  }

  // ConciergeClient override.
  void RemoveObserver(Observer* observer) override {
    observer_list_.RemoveObserver(observer);
  }

  bool IsContainerStartedSignalConnected() override {
    return is_container_started_signal_connected_;
  }

  bool IsContainerStartupFailedSignalConnected() override {
    return is_container_startup_failed_signal_connected_;
  }

  void CreateDiskImage(
      const vm_tools::concierge::CreateDiskImageRequest& request,
      DBusMethodCallback<vm_tools::concierge::CreateDiskImageResponse> callback)
      override {
    dbus::MethodCall method_call(vm_tools::concierge::kVmConciergeInterface,
                                 vm_tools::concierge::kCreateDiskImageMethod);
    dbus::MessageWriter writer(&method_call);

    if (!writer.AppendProtoAsArrayOfBytes(request)) {
      LOG(ERROR) << "Failed to encode CreateDiskImageRequest protobuf";
      std::move(callback).Run(base::nullopt);
      return;
    }

    concierge_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&ConciergeClientImpl::OnDBusProtoResponse<
                           vm_tools::concierge::CreateDiskImageResponse>,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void DestroyDiskImage(
      const vm_tools::concierge::DestroyDiskImageRequest& request,
      DBusMethodCallback<vm_tools::concierge::DestroyDiskImageResponse>
          callback) override {
    dbus::MethodCall method_call(vm_tools::concierge::kVmConciergeInterface,
                                 vm_tools::concierge::kDestroyDiskImageMethod);
    dbus::MessageWriter writer(&method_call);

    if (!writer.AppendProtoAsArrayOfBytes(request)) {
      LOG(ERROR) << "Failed to encode DestroyDiskImageRequest protobuf";
      std::move(callback).Run(base::nullopt);
      return;
    }

    concierge_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&ConciergeClientImpl::OnDBusProtoResponse<
                           vm_tools::concierge::DestroyDiskImageResponse>,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void StartTerminaVm(const vm_tools::concierge::StartVmRequest& request,
                      DBusMethodCallback<vm_tools::concierge::StartVmResponse>
                          callback) override {
    dbus::MethodCall method_call(vm_tools::concierge::kVmConciergeInterface,
                                 vm_tools::concierge::kStartVmMethod);
    dbus::MessageWriter writer(&method_call);

    if (!writer.AppendProtoAsArrayOfBytes(request)) {
      LOG(ERROR) << "Failed to encode StartVmRequest protobuf";
      std::move(callback).Run(base::nullopt);
      return;
    }

    concierge_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&ConciergeClientImpl::OnDBusProtoResponse<
                           vm_tools::concierge::StartVmResponse>,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void StopVm(const vm_tools::concierge::StopVmRequest& request,
              DBusMethodCallback<vm_tools::concierge::StopVmResponse> callback)
      override {
    dbus::MethodCall method_call(vm_tools::concierge::kVmConciergeInterface,
                                 vm_tools::concierge::kStopVmMethod);
    dbus::MessageWriter writer(&method_call);

    if (!writer.AppendProtoAsArrayOfBytes(request)) {
      LOG(ERROR) << "Failed to encode StopVmRequest protobuf";
      std::move(callback).Run(base::nullopt);
      return;
    }

    concierge_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&ConciergeClientImpl::OnDBusProtoResponse<
                           vm_tools::concierge::StopVmResponse>,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void StartContainer(
      const vm_tools::concierge::StartContainerRequest& request,
      DBusMethodCallback<vm_tools::concierge::StartContainerResponse> callback)
      override {
    dbus::MethodCall method_call(vm_tools::concierge::kVmConciergeInterface,
                                 vm_tools::concierge::kStartContainerMethod);
    dbus::MessageWriter writer(&method_call);

    if (!writer.AppendProtoAsArrayOfBytes(request)) {
      LOG(ERROR) << "Failed to encode StartContainerRequest protobuf";
      std::move(callback).Run(base::nullopt);
      return;
    }

    concierge_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&ConciergeClientImpl::OnDBusProtoResponse<
                           vm_tools::concierge::StartContainerResponse>,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void LaunchContainerApplication(
      const vm_tools::concierge::LaunchContainerApplicationRequest& request,
      DBusMethodCallback<
          vm_tools::concierge::LaunchContainerApplicationResponse> callback)
      override {
    dbus::MethodCall method_call(
        vm_tools::concierge::kVmConciergeInterface,
        vm_tools::concierge::kLaunchContainerApplicationMethod);
    dbus::MessageWriter writer(&method_call);

    if (!writer.AppendProtoAsArrayOfBytes(request)) {
      LOG(ERROR)
          << "Failed to encode LaunchContainerApplicationRequest protobuf";
      std::move(callback).Run(base::nullopt);
      return;
    }

    concierge_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_INFINITE,
        base::BindOnce(
            &ConciergeClientImpl::OnDBusProtoResponse<
                vm_tools::concierge::LaunchContainerApplicationResponse>,
            weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void GetContainerAppIcons(
      const vm_tools::concierge::ContainerAppIconRequest& request,
      DBusMethodCallback<vm_tools::concierge::ContainerAppIconResponse>
          callback) override {
    dbus::MethodCall method_call(
        vm_tools::concierge::kVmConciergeInterface,
        vm_tools::concierge::kGetContainerAppIconMethod);
    dbus::MessageWriter writer(&method_call);

    if (!writer.AppendProtoAsArrayOfBytes(request)) {
      LOG(ERROR) << "Failed to encode ContainerAppIonRequest protobuf";
      std::move(callback).Run(base::nullopt);
      return;
    }

    concierge_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_INFINITE,
        base::BindOnce(&ConciergeClientImpl::OnDBusProtoResponse<
                           vm_tools::concierge::ContainerAppIconResponse>,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void WaitForServiceToBeAvailable(
      dbus::ObjectProxy::WaitForServiceToBeAvailableCallback callback)
      override {
    concierge_proxy_->WaitForServiceToBeAvailable(std::move(callback));
  }

  void GetContainerSshKeys(
      const vm_tools::concierge::ContainerSshKeysRequest& request,
      DBusMethodCallback<vm_tools::concierge::ContainerSshKeysResponse>
          callback) override {
    dbus::MethodCall method_call(
        vm_tools::concierge::kVmConciergeInterface,
        vm_tools::concierge::kGetContainerSshKeysMethod);
    dbus::MessageWriter writer(&method_call);

    if (!writer.AppendProtoAsArrayOfBytes(request)) {
      LOG(ERROR) << "Failed to encode ContainerSshKeysRequest protobuf";
      std::move(callback).Run(base::nullopt);
      return;
    }

    concierge_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&ConciergeClientImpl::OnDBusProtoResponse<
                           vm_tools::concierge::ContainerSshKeysResponse>,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

 protected:
  void Init(dbus::Bus* bus) override {
    concierge_proxy_ = bus->GetObjectProxy(
        vm_tools::concierge::kVmConciergeServiceName,
        dbus::ObjectPath(vm_tools::concierge::kVmConciergeServicePath));
    if (!concierge_proxy_) {
      LOG(ERROR) << "Unable to get dbus proxy for "
                 << vm_tools::concierge::kVmConciergeServiceName;
    }
    concierge_proxy_->ConnectToSignal(
        vm_tools::concierge::kVmConciergeInterface,
        vm_tools::concierge::kContainerStartedSignal,
        base::BindRepeating(&ConciergeClientImpl::OnContainerStartedSignal,
                            weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&ConciergeClientImpl::OnSignalConnected,
                       weak_ptr_factory_.GetWeakPtr()));
    concierge_proxy_->ConnectToSignal(
        vm_tools::concierge::kVmConciergeInterface,
        vm_tools::concierge::kContainerStartupFailedSignal,
        base::BindRepeating(
            &ConciergeClientImpl::OnContainerStartupFailedSignal,
            weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&ConciergeClientImpl::OnSignalConnected,
                       weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  template <typename ResponseProto>
  void OnDBusProtoResponse(DBusMethodCallback<ResponseProto> callback,
                           dbus::Response* dbus_response) {
    if (!dbus_response) {
      std::move(callback).Run(base::nullopt);
      return;
    }
    ResponseProto reponse_proto;
    dbus::MessageReader reader(dbus_response);
    if (!reader.PopArrayOfBytesAsProto(&reponse_proto)) {
      LOG(ERROR) << "Failed to parse proto from DBus Response.";
      std::move(callback).Run(base::nullopt);
      return;
    }
    std::move(callback).Run(std::move(reponse_proto));
  }

  void OnContainerStartedSignal(dbus::Signal* signal) {
    DCHECK_EQ(signal->GetInterface(),
              vm_tools::concierge::kVmConciergeInterface);
    DCHECK_EQ(signal->GetMember(),
              vm_tools::concierge::kContainerStartedSignal);

    vm_tools::concierge::ContainerStartedSignal container_started_signal;
    dbus::MessageReader reader(signal);
    if (!reader.PopArrayOfBytesAsProto(&container_started_signal)) {
      LOG(ERROR) << "Failed to parse proto from DBus Signal";
      return;
    }
    // Tell our Observers.
    for (auto& observer : observer_list_) {
      observer.OnContainerStarted(container_started_signal);
    }
  }

  void OnContainerStartupFailedSignal(dbus::Signal* signal) {
    DCHECK_EQ(signal->GetInterface(),
              vm_tools::concierge::kVmConciergeInterface);
    DCHECK_EQ(signal->GetMember(),
              vm_tools::concierge::kContainerStartupFailedSignal);

    vm_tools::concierge::ContainerStartedSignal container_startup_failed_signal;
    dbus::MessageReader reader(signal);
    if (!reader.PopArrayOfBytesAsProto(&container_startup_failed_signal)) {
      LOG(ERROR) << "Failed to parse proto from DBus Signal";
      return;
    }
    // Tell our Observers.
    for (auto& observer : observer_list_) {
      observer.OnContainerStartupFailed(container_startup_failed_signal);
    }
  }

  void OnSignalConnected(const std::string& interface_name,
                         const std::string& signal_name,
                         bool is_connected) {
    DCHECK_EQ(interface_name, vm_tools::concierge::kVmConciergeInterface);
    if (!is_connected) {
      LOG(ERROR)
          << "Failed to connect to Signal. Async StartContainer will not work";
    }
    if (signal_name == vm_tools::concierge::kContainerStartedSignal) {
      is_container_started_signal_connected_ = is_connected;
    } else if (signal_name ==
               vm_tools::concierge::kContainerStartupFailedSignal) {
      is_container_startup_failed_signal_connected_ = is_connected;
    } else {
      NOTREACHED();
    }
  }

  dbus::ObjectProxy* concierge_proxy_ = nullptr;

  base::ObserverList<Observer> observer_list_;

  bool is_container_started_signal_connected_ = false;
  bool is_container_startup_failed_signal_connected_ = false;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<ConciergeClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ConciergeClientImpl);
};

ConciergeClient::ConciergeClient() = default;

ConciergeClient::~ConciergeClient() = default;

ConciergeClient* ConciergeClient::Create() {
  return new ConciergeClientImpl();
}

}  // namespace chromeos
