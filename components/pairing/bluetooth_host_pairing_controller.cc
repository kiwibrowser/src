// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pairing/bluetooth_host_pairing_controller.h"

#include <utility>

#include "base/bind.h"
#include "base/hash.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "chromeos/system/devicetype.h"
#include "components/pairing/bluetooth_pairing_constants.h"
#include "components/pairing/pairing_api.pb.h"
#include "components/pairing/proto_decoder.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/base/io_buffer.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace pairing_chromeos {

namespace {
const int kReceiveSize = 16384;

pairing_api::HostStatusParameters::Connectivity PairingApiConnectivityStatus(
    HostPairingController::Connectivity connectivity_status) {
  switch (connectivity_status) {
    case HostPairingController::CONNECTIVITY_UNTESTED:
      return pairing_api::HostStatusParameters::CONNECTIVITY_UNTESTED;
    case HostPairingController::CONNECTIVITY_NONE:
      return pairing_api::HostStatusParameters::CONNECTIVITY_NONE;
    case HostPairingController::CONNECTIVITY_LIMITED:
      return pairing_api::HostStatusParameters::CONNECTIVITY_LIMITED;
    case HostPairingController::CONNECTIVITY_CONNECTING:
      return pairing_api::HostStatusParameters::CONNECTIVITY_CONNECTING;
    case HostPairingController::CONNECTIVITY_CONNECTED:
      return pairing_api::HostStatusParameters::CONNECTIVITY_CONNECTED;
    default:
      NOTREACHED();
      return pairing_api::HostStatusParameters::CONNECTIVITY_UNTESTED;
  }
}

pairing_api::HostStatusParameters::UpdateStatus PairingApiUpdateStatus(
    HostPairingController::UpdateStatus update_status) {
  switch(update_status) {
    case HostPairingController::UPDATE_STATUS_UNKNOWN:
      return pairing_api::HostStatusParameters::UPDATE_STATUS_UNKNOWN;
    case HostPairingController::UPDATE_STATUS_UPDATING:
      return pairing_api::HostStatusParameters::UPDATE_STATUS_UPDATING;
    case HostPairingController::UPDATE_STATUS_REBOOTING:
      return pairing_api::HostStatusParameters::UPDATE_STATUS_REBOOTING;
    case HostPairingController::UPDATE_STATUS_UPDATED:
      return pairing_api::HostStatusParameters::UPDATE_STATUS_UPDATED;
    default:
      NOTREACHED();
      return pairing_api::HostStatusParameters::UPDATE_STATUS_UNKNOWN;
  }
}

pairing_api::HostStatusParameters::EnrollmentStatus PairingApiEnrollmentStatus(
    HostPairingController::EnrollmentStatus enrollment_status) {
  switch(enrollment_status) {
    case HostPairingController::ENROLLMENT_STATUS_UNKNOWN:
      return pairing_api::HostStatusParameters::ENROLLMENT_STATUS_UNKNOWN;
    case HostPairingController::ENROLLMENT_STATUS_ENROLLING:
      return pairing_api::HostStatusParameters::ENROLLMENT_STATUS_ENROLLING;
    case HostPairingController::ENROLLMENT_STATUS_FAILURE:
      return pairing_api::HostStatusParameters::ENROLLMENT_STATUS_FAILURE;
    case HostPairingController::ENROLLMENT_STATUS_SUCCESS:
      return pairing_api::HostStatusParameters::ENROLLMENT_STATUS_SUCCESS;
    default:
      NOTREACHED();
      return pairing_api::HostStatusParameters::ENROLLMENT_STATUS_UNKNOWN;
  }
}

}  // namespace

BluetoothHostPairingController::BluetoothHostPairingController(
    service_manager::Connector* connector)
    : proto_decoder_(std::make_unique<ProtoDecoder>(this)) {
  DCHECK(connector);
  connector->BindInterface(device::mojom::kServiceName,
                           mojo::MakeRequest(&input_device_manager_));
}

BluetoothHostPairingController::~BluetoothHostPairingController() {
  Reset();
}

void BluetoothHostPairingController::SetDelegateForTesting(
    TestDelegate* delegate) {
  delegate_ = delegate;
}

scoped_refptr<device::BluetoothAdapter>
BluetoothHostPairingController::GetAdapterForTesting() {
  return adapter_;
}

void BluetoothHostPairingController::ChangeStage(Stage new_stage) {
  if (current_stage_ == new_stage)
    return;
  VLOG(1) << "ChangeStage " << new_stage;
  current_stage_ = new_stage;
  for (Observer& observer : observers_)
    observer.PairingStageChanged(new_stage);
}

void BluetoothHostPairingController::SendHostStatus() {
  pairing_api::HostStatus host_status;

  host_status.set_api_version(kPairingAPIVersion);
  if (!enrollment_domain_.empty())
    host_status.mutable_parameters()->set_domain(enrollment_domain_);
  if (!permanent_id_.empty())
    host_status.mutable_parameters()->set_permanent_id(permanent_id_);

  // TODO(zork): Get these values from the UI. (http://crbug.com/405744)
  host_status.mutable_parameters()->set_connectivity(
      PairingApiConnectivityStatus(connectivity_status_));
  host_status.mutable_parameters()->set_update_status(
      PairingApiUpdateStatus(update_status_));
  host_status.mutable_parameters()->set_enrollment_status(
      PairingApiEnrollmentStatus(enrollment_status_));

  // TODO(zork): Get a list of other paired controllers.
  // (http://crbug.com/405757)

  int size = 0;
  scoped_refptr<net::IOBuffer> io_buffer(
      ProtoDecoder::SendHostStatus(host_status, &size));

  controller_socket_->Send(
      io_buffer, size,
      base::Bind(&BluetoothHostPairingController::OnSendComplete,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothHostPairingController::OnSendError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothHostPairingController::SendErrorCodeAndMessage() {
  if (error_code_ ==
      static_cast<int>(HostPairingController::ErrorCode::ERROR_NONE)) {
    return;
  }

  pairing_api::Error error_status;
  error_status.set_api_version(kPairingAPIVersion);
  error_status.mutable_parameters()->set_code(error_code_);
  error_status.mutable_parameters()->set_description(error_message_);

  int size = 0;
  scoped_refptr<net::IOBuffer> io_buffer(
      ProtoDecoder::SendError(error_status, &size));

  controller_socket_->Send(
      io_buffer, size,
      base::Bind(&BluetoothHostPairingController::OnSendComplete,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothHostPairingController::OnSendError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothHostPairingController::Reset() {
  if (adapter_.get()) {
    device::BluetoothDevice* device =
        adapter_->GetDevice(controller_device_address_);
    if (device && device->IsPaired()) {
      device->Forget(base::Bind(&BluetoothHostPairingController::OnForget,
                                ptr_factory_.GetWeakPtr()),
                     base::Bind(&BluetoothHostPairingController::OnForget,
                                ptr_factory_.GetWeakPtr()));
      return;
    }
  }
  OnForget();
}

void BluetoothHostPairingController::OnGetAdapter(
    scoped_refptr<device::BluetoothAdapter> adapter) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!adapter_.get());
  adapter_ = adapter;

  if (adapter_->IsPresent()) {
    SetPowered();
  } else {
    // Set the name once the adapter is present.
    adapter_->AddObserver(this);
  }
}

void BluetoothHostPairingController::SetPowered() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (adapter_->IsPowered()) {
    was_powered_ = true;
    OnSetPowered();
  } else {
    adapter_->SetPowered(
        true,
        base::Bind(&BluetoothHostPairingController::OnSetPowered,
                   ptr_factory_.GetWeakPtr()),
        base::Bind(&BluetoothHostPairingController::OnSetError,
                   ptr_factory_.GetWeakPtr()));
  }
}

void BluetoothHostPairingController::OnSetPowered() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  adapter_->AddPairingDelegate(
      this, device::BluetoothAdapter::PAIRING_DELEGATE_PRIORITY_HIGH);

  device::BluetoothAdapter::ServiceOptions options;
  options.name.reset(new std::string(kPairingServiceName));

  adapter_->CreateRfcommService(
      device::BluetoothUUID(kPairingServiceUUID), options,
      base::Bind(&BluetoothHostPairingController::OnCreateService,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothHostPairingController::OnCreateServiceError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothHostPairingController::OnCreateService(
    scoped_refptr<device::BluetoothSocket> socket) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  service_socket_ = socket;

  service_socket_->Accept(
      base::Bind(&BluetoothHostPairingController::OnAccept,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothHostPairingController::OnAcceptError,
                 ptr_factory_.GetWeakPtr()));

  adapter_->SetDiscoverable(
      true,
      base::Bind(&BluetoothHostPairingController::OnSetDiscoverable,
                 ptr_factory_.GetWeakPtr(), true),
      base::Bind(&BluetoothHostPairingController::OnSetError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothHostPairingController::OnAccept(
    const device::BluetoothDevice* device,
    scoped_refptr<device::BluetoothSocket> socket) {
  controller_device_address_ = device->GetAddress();

  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  adapter_->SetDiscoverable(
      false,
      base::Bind(&BluetoothHostPairingController::OnSetDiscoverable,
                 ptr_factory_.GetWeakPtr(), false),
      base::Bind(&BluetoothHostPairingController::OnSetError,
                 ptr_factory_.GetWeakPtr()));
  controller_socket_ = socket;

  SendHostStatus();

  controller_socket_->Receive(
      kReceiveSize,
      base::Bind(&BluetoothHostPairingController::OnReceiveComplete,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothHostPairingController::OnReceiveError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothHostPairingController::OnSetDiscoverable(bool change_stage) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (change_stage) {
    DCHECK_EQ(current_stage_, STAGE_NONE);
    ChangeStage(STAGE_WAITING_FOR_CONTROLLER);
  }
}

void BluetoothHostPairingController::OnSendComplete(int bytes_sent) {}

void BluetoothHostPairingController::OnReceiveComplete(
    int bytes, scoped_refptr<net::IOBuffer> io_buffer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  proto_decoder_->DecodeIOBuffer(bytes, io_buffer);

  if (controller_socket_.get()) {
    controller_socket_->Receive(
        kReceiveSize,
        base::Bind(&BluetoothHostPairingController::OnReceiveComplete,
                   ptr_factory_.GetWeakPtr()),
        base::Bind(&BluetoothHostPairingController::OnReceiveError,
                   ptr_factory_.GetWeakPtr()));
  }
}

void BluetoothHostPairingController::OnCreateServiceError(
    const std::string& message) {
  LOG(ERROR) << message;
  ChangeStage(STAGE_INITIALIZATION_ERROR);
}

void BluetoothHostPairingController::OnSetError() {
  adapter_->RemovePairingDelegate(this);
  ChangeStage(STAGE_INITIALIZATION_ERROR);
}

void BluetoothHostPairingController::OnAcceptError(
    const std::string& error_message) {
  LOG(ERROR) << error_message;
  ChangeStage(STAGE_CONTROLLER_CONNECTION_ERROR);
}

void BluetoothHostPairingController::OnSendError(
    const std::string& error_message) {
  LOG(ERROR) << error_message;
  if (enrollment_status_ != ENROLLMENT_STATUS_ENROLLING &&
      enrollment_status_ != ENROLLMENT_STATUS_SUCCESS) {
    ChangeStage(STAGE_CONTROLLER_CONNECTION_ERROR);
  }
}

void BluetoothHostPairingController::PowerOffAdapterIfApplicable(
    std::vector<InputDeviceInfoPtr> devices) {
  bool use_bluetooth = false;
  for (auto& device : devices) {
    if (device->type == device::mojom::InputDeviceType::TYPE_BLUETOOTH) {
      use_bluetooth = true;
      break;
    }
  }
  if (!was_powered_ && !use_bluetooth) {
    adapter_->SetPowered(
        false, base::Bind(&BluetoothHostPairingController::ResetAdapter,
                          ptr_factory_.GetWeakPtr()),
        base::Bind(&BluetoothHostPairingController::ResetAdapter,
                   ptr_factory_.GetWeakPtr()));
  } else {
    ResetAdapter();
  }
}

void BluetoothHostPairingController::ResetAdapter() {
  adapter_->RemoveObserver(this);
  adapter_ = nullptr;
  if (delegate_)
    delegate_->OnAdapterReset();
}

void BluetoothHostPairingController::OnForget() {
  if (controller_socket_.get()) {
    controller_socket_->Close();
    controller_socket_ = nullptr;
  }

  if (service_socket_.get()) {
    service_socket_->Close();
    service_socket_ = nullptr;
  }

  if (adapter_.get()) {
    if (adapter_->IsDiscoverable()) {
      adapter_->SetDiscoverable(false, base::DoNothing(), base::DoNothing());
    }

    input_device_manager_->GetDevices(base::BindOnce(
        &BluetoothHostPairingController::PowerOffAdapterIfApplicable,
        ptr_factory_.GetWeakPtr()));
  }
  ChangeStage(STAGE_NONE);
}

void BluetoothHostPairingController::SetControllerDeviceAddressForTesting(
    const std::string& address) {
  controller_device_address_ = address;
}

void BluetoothHostPairingController::OnReceiveError(
    device::BluetoothSocket::ErrorReason reason,
    const std::string& error_message) {
  LOG(ERROR) << reason << ", " << error_message;
  ChangeStage(STAGE_CONTROLLER_CONNECTION_ERROR);
}

void BluetoothHostPairingController::OnHostStatusMessage(
    const pairing_api::HostStatus& message) {
  NOTREACHED();
}

void BluetoothHostPairingController::OnConfigureHostMessage(
    const pairing_api::ConfigureHost& message) {
  ChangeStage(STAGE_SETUP_BASIC_CONFIGURATION);
  for (Observer& observer : observers_) {
    observer.ConfigureHostRequested(
        message.parameters().accepted_eula(), message.parameters().lang(),
        message.parameters().timezone(), message.parameters().send_reports(),
        message.parameters().keyboard_layout());
  }
}

void BluetoothHostPairingController::OnPairDevicesMessage(
    const pairing_api::PairDevices& message) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  enrollment_domain_ = message.parameters().enrolling_domain();
  ChangeStage(STAGE_ENROLLING);
  for (Observer& observer : observers_)
    observer.EnrollHostRequested(message.parameters().admin_access_token());
}

void BluetoothHostPairingController::OnCompleteSetupMessage(
    const pairing_api::CompleteSetup& message) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (current_stage_ != STAGE_ENROLLMENT_SUCCESS) {
    ChangeStage(STAGE_ENROLLMENT_ERROR);
  } else {
    // TODO(zork): Handle adding another controller. (http://crbug.com/405757)
    ChangeStage(STAGE_FINISHED);
  }
  Reset();
}

void BluetoothHostPairingController::OnErrorMessage(
    const pairing_api::Error& message) {
  NOTREACHED();
}

void BluetoothHostPairingController::OnRebootMessage(
    const pairing_api::Reboot& message) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  for (Observer& observer : observers_)
    observer.RebootHostRequested();
}

void BluetoothHostPairingController::OnAddNetworkMessage(
    const pairing_api::AddNetwork& message) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  for (Observer& observer : observers_)
    observer.AddNetworkRequested(message.parameters().onc_spec());
}

void BluetoothHostPairingController::AdapterPresentChanged(
    device::BluetoothAdapter* adapter,
    bool present) {
  DCHECK_EQ(adapter, adapter_.get());
  if (present) {
    adapter_->RemoveObserver(this);
    SetPowered();
  }
}

void BluetoothHostPairingController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void BluetoothHostPairingController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

HostPairingController::Stage BluetoothHostPairingController::GetCurrentStage() {
  return current_stage_;
}

void BluetoothHostPairingController::StartPairing() {
  DCHECK_EQ(current_stage_, STAGE_NONE);
  const bool bluetooth_supported =
      device::BluetoothAdapterFactory::IsBluetoothSupported();
  if (!bluetooth_supported) {
    ChangeStage(STAGE_INITIALIZATION_ERROR);
    return;
  }

  device::BluetoothAdapterFactory::GetAdapter(
      base::Bind(&BluetoothHostPairingController::OnGetAdapter,
                 ptr_factory_.GetWeakPtr()));
}

std::string BluetoothHostPairingController::GetDeviceName() {
  return adapter_.get() ? adapter_->GetName() : std::string();
}

std::string BluetoothHostPairingController::GetConfirmationCode() {
  DCHECK_EQ(current_stage_, STAGE_WAITING_FOR_CODE_CONFIRMATION);
  return confirmation_code_;
}

std::string BluetoothHostPairingController::GetEnrollmentDomain() {
  return enrollment_domain_;
}

void BluetoothHostPairingController::OnNetworkConnectivityChanged(
    Connectivity connectivity_status) {
  connectivity_status_ = connectivity_status;
  if (connectivity_status == CONNECTIVITY_NONE) {
    ChangeStage(STAGE_SETUP_NETWORK_ERROR);
    SendErrorCodeAndMessage();
  } else {
    SendHostStatus();
  }
}

void BluetoothHostPairingController::OnUpdateStatusChanged(
    UpdateStatus update_status) {
  update_status_ = update_status;
  if (update_status == UPDATE_STATUS_UPDATED)
    ChangeStage(STAGE_WAITING_FOR_CREDENTIALS);
  SendHostStatus();
}

void BluetoothHostPairingController::OnEnrollmentStatusChanged(
    EnrollmentStatus enrollment_status) {
  DCHECK_EQ(current_stage_, STAGE_ENROLLING);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  enrollment_status_ = enrollment_status;
  if (enrollment_status == ENROLLMENT_STATUS_SUCCESS) {
    ChangeStage(STAGE_ENROLLMENT_SUCCESS);
    SendHostStatus();
  } else if (enrollment_status == ENROLLMENT_STATUS_FAILURE) {
    ChangeStage(STAGE_ENROLLMENT_ERROR);
    SendErrorCodeAndMessage();
  }
}

void BluetoothHostPairingController::SetPermanentId(
    const std::string& permanent_id) {
  permanent_id_ = permanent_id;
}

void BluetoothHostPairingController::SetErrorCodeAndMessage(
    int error_code,
    const std::string& error_message) {
  error_code_ = error_code;
  error_message_ = error_message;
}

void BluetoothHostPairingController::RequestPinCode(
    device::BluetoothDevice* device) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothHostPairingController::RequestPasskey(
    device::BluetoothDevice* device) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothHostPairingController::DisplayPinCode(
    device::BluetoothDevice* device,
    const std::string& pincode) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothHostPairingController::DisplayPasskey(
    device::BluetoothDevice* device,
    uint32_t passkey) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothHostPairingController::KeysEntered(
    device::BluetoothDevice* device,
    uint32_t entered) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothHostPairingController::ConfirmPasskey(
    device::BluetoothDevice* device,
    uint32_t passkey) {
  // If a new connection is occurring, reset the stage.  This can occur if the
  // pairing times out, or a new controller connects.
  if (current_stage_ == STAGE_WAITING_FOR_CODE_CONFIRMATION)
    ChangeStage(STAGE_WAITING_FOR_CONTROLLER);

  confirmation_code_ = base::StringPrintf("%06d", passkey);
  device->ConfirmPairing();
  ChangeStage(STAGE_WAITING_FOR_CODE_CONFIRMATION);
}

void BluetoothHostPairingController::AuthorizePairing(
    device::BluetoothDevice* device) {
  // Disallow unknown device.
  device->RejectPairing();
}

}  // namespace pairing_chromeos
