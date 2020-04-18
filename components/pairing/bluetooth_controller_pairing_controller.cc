// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pairing/bluetooth_controller_pairing_controller.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/pairing/bluetooth_pairing_constants.h"
#include "components/pairing/pairing_api.pb.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "net/base/io_buffer.h"

namespace {
const int kReceiveSize = 16384;
}

namespace pairing_chromeos {

BluetoothControllerPairingController::BluetoothControllerPairingController()
    : current_stage_(STAGE_NONE),
      proto_decoder_(new ProtoDecoder(this)),
      ptr_factory_(this) {
}

BluetoothControllerPairingController::~BluetoothControllerPairingController() {
  Reset();
}

device::BluetoothDevice* BluetoothControllerPairingController::GetController() {
  DCHECK(!controller_device_id_.empty());
  device::BluetoothDevice* device = adapter_->GetDevice(controller_device_id_);
  if (!device) {
    LOG(ERROR) << "Lost connection to controller.";
    ChangeStage(STAGE_ESTABLISHING_CONNECTION_ERROR);
  }

  return device;
}

void BluetoothControllerPairingController::ChangeStage(Stage new_stage) {
  if (current_stage_ == new_stage)
    return;
  VLOG(1) << "ChangeStage " << new_stage;
  current_stage_ = new_stage;
  for (ControllerPairingController::Observer& observer : observers_)
    observer.PairingStageChanged(new_stage);
}

void BluetoothControllerPairingController::Reset() {
  controller_device_id_.clear();
  discovery_session_.reset();

  if (socket_.get()) {
    socket_->Close();
    socket_ = NULL;
  }

  if (adapter_.get()) {
    adapter_->RemoveObserver(this);
    adapter_ = NULL;
  }
}

void BluetoothControllerPairingController::DeviceFound(
    device::BluetoothDevice* device) {
  DCHECK_EQ(current_stage_, STAGE_DEVICES_DISCOVERY);
  DCHECK(thread_checker_.CalledOnValidThread());

  device::BluetoothDevice::UUIDSet uuids = device->GetUUIDs();
  if (base::StartsWith(device->GetNameForDisplay(),
                       base::ASCIIToUTF16(kDeviceNamePrefix),
                       base::CompareCase::INSENSITIVE_ASCII) &&
      base::ContainsKey(uuids, device::BluetoothUUID(kPairingServiceUUID))) {
    discovered_devices_.insert(device->GetAddress());
    for (ControllerPairingController::Observer& observer : observers_)
      observer.DiscoveredDevicesListChanged();
  }
}

void BluetoothControllerPairingController::DeviceLost(
    device::BluetoothDevice* device) {
  DCHECK_EQ(current_stage_, STAGE_DEVICES_DISCOVERY);
  DCHECK(thread_checker_.CalledOnValidThread());
  std::set<std::string>::iterator ix =
      discovered_devices_.find(device->GetAddress());
  if (ix != discovered_devices_.end()) {
    discovered_devices_.erase(ix);
    for (ControllerPairingController::Observer& observer : observers_)
      observer.DiscoveredDevicesListChanged();
  }
}

void BluetoothControllerPairingController::SendBuffer(
    scoped_refptr<net::IOBuffer> io_buffer, int size) {
  socket_->Send(
      io_buffer, size,
      base::Bind(&BluetoothControllerPairingController::OnSendComplete,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothControllerPairingController::OnErrorWithMessage,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothControllerPairingController::OnSetPowered() {
  DCHECK(thread_checker_.CalledOnValidThread());
  adapter_->StartDiscoverySession(
      base::Bind(&BluetoothControllerPairingController::OnStartDiscoverySession,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothControllerPairingController::OnError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothControllerPairingController::OnGetAdapter(
    scoped_refptr<device::BluetoothAdapter> adapter) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!adapter_.get());
  adapter_ = adapter;
  adapter_->AddObserver(this);

  if (adapter_->IsPowered()) {
    OnSetPowered();
  } else {
    adapter_->SetPowered(
        true,
        base::Bind(&BluetoothControllerPairingController::OnSetPowered,
                   ptr_factory_.GetWeakPtr()),
        base::Bind(&BluetoothControllerPairingController::OnError,
                   ptr_factory_.GetWeakPtr()));
  }
}

void BluetoothControllerPairingController::OnStartDiscoverySession(
    std::unique_ptr<device::BluetoothDiscoverySession> discovery_session) {
  DCHECK(thread_checker_.CalledOnValidThread());
  discovery_session_ = std::move(discovery_session);
  ChangeStage(STAGE_DEVICES_DISCOVERY);

  for (auto* device : adapter_->GetDevices())
    DeviceFound(device);
}

void BluetoothControllerPairingController::OnConnect() {
  DCHECK(thread_checker_.CalledOnValidThread());
  device::BluetoothDevice* device = GetController();
  if (device) {
    device->ConnectToService(
        device::BluetoothUUID(kPairingServiceUUID),
        base::Bind(&BluetoothControllerPairingController::OnConnectToService,
                   ptr_factory_.GetWeakPtr()),
        base::Bind(&BluetoothControllerPairingController::OnErrorWithMessage,
                   ptr_factory_.GetWeakPtr()));
  }
}

void BluetoothControllerPairingController::OnConnectToService(
    scoped_refptr<device::BluetoothSocket> socket) {
  DCHECK(thread_checker_.CalledOnValidThread());
  socket_ = socket;

  socket_->Receive(
      kReceiveSize,
      base::Bind(&BluetoothControllerPairingController::OnReceiveComplete,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothControllerPairingController::OnReceiveError,
                 ptr_factory_.GetWeakPtr()));
  ChangeStage(STAGE_PAIRING_DONE);
}

void BluetoothControllerPairingController::OnSendComplete(int bytes_sent) {}

void BluetoothControllerPairingController::OnReceiveComplete(
    int bytes, scoped_refptr<net::IOBuffer> io_buffer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  proto_decoder_->DecodeIOBuffer(bytes, io_buffer);

  socket_->Receive(
      kReceiveSize,
      base::Bind(&BluetoothControllerPairingController::OnReceiveComplete,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothControllerPairingController::OnReceiveError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothControllerPairingController::OnError() {
  LOG(ERROR) << "Pairing initialization failed";
  ChangeStage(STAGE_INITIALIZATION_ERROR);
  Reset();
}

void BluetoothControllerPairingController::OnErrorWithMessage(
    const std::string& message) {
  LOG(ERROR) << message;
  ChangeStage(STAGE_INITIALIZATION_ERROR);
  Reset();
}

void BluetoothControllerPairingController::OnConnectError(
    device::BluetoothDevice::ConnectErrorCode error_code) {
  DCHECK(thread_checker_.CalledOnValidThread());
  device::BluetoothDevice* device = GetController();

  if (device && device->IsPaired()) {
    // The connection attempt is only used to start the pairing between the
    // devices.  If the connection fails, it's not a problem as long as pairing
    // was successful.
    OnConnect();
  } else {
    // This can happen if the confirmation dialog times out.
    ChangeStage(STAGE_ESTABLISHING_CONNECTION_ERROR);
  }
}

void BluetoothControllerPairingController::OnReceiveError(
    device::BluetoothSocket::ErrorReason reason,
    const std::string& error_message) {
  LOG(ERROR) << reason << ", " << error_message;
  Reset();
}

void BluetoothControllerPairingController::AddObserver(
    ControllerPairingController::Observer* observer) {
  observers_.AddObserver(observer);
}

void BluetoothControllerPairingController::RemoveObserver(
    ControllerPairingController::Observer* observer) {
  observers_.RemoveObserver(observer);
}

ControllerPairingController::Stage
BluetoothControllerPairingController::GetCurrentStage() {
  return current_stage_;
}

void BluetoothControllerPairingController::StartPairing() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(current_stage_ == STAGE_NONE ||
         current_stage_ == STAGE_DEVICE_NOT_FOUND ||
         current_stage_ == STAGE_ESTABLISHING_CONNECTION_ERROR ||
         current_stage_ == STAGE_HOST_ENROLLMENT_ERROR);
  if (!device::BluetoothAdapterFactory::IsBluetoothSupported()) {
    ChangeStage(STAGE_INITIALIZATION_ERROR);
    return;
  }

  device::BluetoothAdapterFactory::GetAdapter(
      base::Bind(&BluetoothControllerPairingController::OnGetAdapter,
                 ptr_factory_.GetWeakPtr()));

}

ControllerPairingController::DeviceIdList
BluetoothControllerPairingController::GetDiscoveredDevices() {
  DCHECK_EQ(current_stage_, STAGE_DEVICES_DISCOVERY);
  return DeviceIdList(discovered_devices_.begin(), discovered_devices_.end());
}

void BluetoothControllerPairingController::ChooseDeviceForPairing(
    const std::string& device_id) {
  DCHECK_EQ(current_stage_, STAGE_DEVICES_DISCOVERY);
  DCHECK(discovered_devices_.count(device_id));
  discovery_session_.reset();
  controller_device_id_ = device_id;

  device::BluetoothDevice* device = GetController();

  if (device) {
    ChangeStage(STAGE_ESTABLISHING_CONNECTION);
    if (device->IsPaired()) {
      OnConnect();
    } else {
      device->Connect(
          this,
          base::Bind(&BluetoothControllerPairingController::OnConnect,
                     ptr_factory_.GetWeakPtr()),
          base::Bind(&BluetoothControllerPairingController::OnConnectError,
                     ptr_factory_.GetWeakPtr()));
    }
  }
}

void BluetoothControllerPairingController::RepeatDiscovery() {
  DCHECK(current_stage_ == STAGE_DEVICE_NOT_FOUND ||
         current_stage_ == STAGE_ESTABLISHING_CONNECTION_ERROR ||
         current_stage_ == STAGE_HOST_ENROLLMENT_ERROR);
  Reset();
  StartPairing();
}

std::string BluetoothControllerPairingController::GetConfirmationCode() {
  DCHECK_EQ(current_stage_, STAGE_WAITING_FOR_CODE_CONFIRMATION);
  DCHECK(!confirmation_code_.empty());
  return confirmation_code_;
}

void BluetoothControllerPairingController::SetConfirmationCodeIsCorrect(
    bool correct) {
  DCHECK_EQ(current_stage_, STAGE_WAITING_FOR_CODE_CONFIRMATION);

  device::BluetoothDevice* device = GetController();
  if (!device)
    return;

  if (correct) {
    device->ConfirmPairing();
    // Once pairing is confirmed, the connection will either be successful, or
    // fail.  Either case is acceptable as long as the devices are paired.
  } else {
    device->RejectPairing();
    controller_device_id_.clear();
    RepeatDiscovery();
  }
}

void BluetoothControllerPairingController::SetHostNetwork(
    const std::string& onc_spec) {
  pairing_api::AddNetwork add_network;
  add_network.set_api_version(kPairingAPIVersion);
  add_network.mutable_parameters()->set_onc_spec(onc_spec);

  int size = 0;
  scoped_refptr<net::IOBuffer> io_buffer(
      ProtoDecoder::SendHostNetwork(add_network, &size));

  SendBuffer(io_buffer, size);
}

void BluetoothControllerPairingController::SetHostConfiguration(
    bool accepted_eula,
    const std::string& lang,
    const std::string& timezone,
    bool send_reports,
    const std::string& keyboard_layout) {
  VLOG(1) << "SetHostConfiguration lang=" << lang
          << ", timezone=" << timezone
          << ", keyboard_layout=" << keyboard_layout;

  pairing_api::ConfigureHost host_config;
  host_config.set_api_version(kPairingAPIVersion);
  host_config.mutable_parameters()->set_accepted_eula(accepted_eula);
  host_config.mutable_parameters()->set_lang(lang);
  host_config.mutable_parameters()->set_timezone(timezone);
  host_config.mutable_parameters()->set_send_reports(send_reports);
  host_config.mutable_parameters()->set_keyboard_layout(keyboard_layout);

  int size = 0;
  scoped_refptr<net::IOBuffer> io_buffer(
      ProtoDecoder::SendConfigureHost(host_config, &size));

  SendBuffer(io_buffer, size);
}

void BluetoothControllerPairingController::OnAuthenticationDone(
    const std::string& domain,
    const std::string& auth_token) {
  DCHECK_EQ(current_stage_, STAGE_WAITING_FOR_CREDENTIALS);

  pairing_api::PairDevices pair_devices;
  pair_devices.set_api_version(kPairingAPIVersion);
  pair_devices.mutable_parameters()->set_admin_access_token(auth_token);
  pair_devices.mutable_parameters()->set_enrolling_domain(domain);

  int size = 0;
  scoped_refptr<net::IOBuffer> io_buffer(
      ProtoDecoder::SendPairDevices(pair_devices, &size));

  SendBuffer(io_buffer, size);
  ChangeStage(STAGE_HOST_ENROLLMENT_IN_PROGRESS);
}

void BluetoothControllerPairingController::StartSession() {
  DCHECK_EQ(current_stage_, STAGE_HOST_ENROLLMENT_SUCCESS);
  ChangeStage(STAGE_FINISHED);
}

void BluetoothControllerPairingController::OnHostStatusMessage(
    const pairing_api::HostStatus& message) {
  pairing_api::HostStatusParameters::UpdateStatus update_status =
      message.parameters().update_status();
  pairing_api::HostStatusParameters::EnrollmentStatus enrollment_status =
      message.parameters().enrollment_status();
  pairing_api::HostStatusParameters::Connectivity connectivity =
      message.parameters().connectivity();
  VLOG(1) << "OnHostStatusMessage, update_status=" << update_status;
  // TODO(zork): Check domain. (http://crbug.com/405761)
  if (connectivity == pairing_api::HostStatusParameters::CONNECTIVITY_NONE) {
    ChangeStage(STAGE_HOST_NETWORK_ERROR);
  } else if (enrollment_status ==
      pairing_api::HostStatusParameters::ENROLLMENT_STATUS_SUCCESS) {
    // TODO(achuith, zork): Need to ensure that controller has also successfully
    // enrolled.
    CompleteSetup();
  } else if (enrollment_status ==
             pairing_api::HostStatusParameters::ENROLLMENT_STATUS_FAILURE) {
    ChangeStage(STAGE_HOST_ENROLLMENT_ERROR);
    // Reboot the host if enrollment failed.
    pairing_api::Reboot reboot;
    reboot.set_api_version(kPairingAPIVersion);
    int size = 0;
    scoped_refptr<net::IOBuffer> io_buffer(
        ProtoDecoder::SendRebootHost(reboot, &size));
    SendBuffer(io_buffer, size);
  } else if (update_status ==
      pairing_api::HostStatusParameters::UPDATE_STATUS_UPDATING) {
    ChangeStage(STAGE_HOST_UPDATE_IN_PROGRESS);
  } else if (update_status ==
      pairing_api::HostStatusParameters::UPDATE_STATUS_UPDATED) {
    ChangeStage(STAGE_WAITING_FOR_CREDENTIALS);
  }
}

void BluetoothControllerPairingController::CompleteSetup() {
  pairing_api::CompleteSetup complete_setup;
  complete_setup.set_api_version(kPairingAPIVersion);
  // TODO(zork): Get AddAnother from UI (http://crbug.com/405757)
  complete_setup.mutable_parameters()->set_add_another(false);

  int size = 0;
  scoped_refptr<net::IOBuffer> io_buffer(
      ProtoDecoder::SendCompleteSetup(complete_setup, &size));

  SendBuffer(io_buffer, size);
  ChangeStage(STAGE_HOST_ENROLLMENT_SUCCESS);
}

void BluetoothControllerPairingController::OnConfigureHostMessage(
    const pairing_api::ConfigureHost& message) {
  NOTREACHED();
}

void BluetoothControllerPairingController::OnPairDevicesMessage(
    const pairing_api::PairDevices& message) {
  NOTREACHED();
}

void BluetoothControllerPairingController::OnCompleteSetupMessage(
    const pairing_api::CompleteSetup& message) {
  NOTREACHED();
}

void BluetoothControllerPairingController::OnErrorMessage(
    const pairing_api::Error& message) {
  LOG(ERROR) << message.parameters().code() << ", " <<
      message.parameters().description();
  ChangeStage(STAGE_HOST_ENROLLMENT_ERROR);
}

void BluetoothControllerPairingController::OnAddNetworkMessage(
    const pairing_api::AddNetwork& message) {
  NOTREACHED();
}

void BluetoothControllerPairingController::OnRebootMessage(
    const pairing_api::Reboot& message) {
  NOTREACHED();
}

void BluetoothControllerPairingController::DeviceAdded(
    device::BluetoothAdapter* adapter,
    device::BluetoothDevice* device) {
  DCHECK_EQ(adapter, adapter_.get());
  DeviceFound(device);
}

void BluetoothControllerPairingController::DeviceRemoved(
    device::BluetoothAdapter* adapter,
    device::BluetoothDevice* device) {
  DCHECK_EQ(adapter, adapter_.get());
  DeviceLost(device);
}

void BluetoothControllerPairingController::RequestPinCode(
    device::BluetoothDevice* device) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::RequestPasskey(
    device::BluetoothDevice* device) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::DisplayPinCode(
    device::BluetoothDevice* device,
    const std::string& pincode) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::DisplayPasskey(
    device::BluetoothDevice* device,
    uint32_t passkey) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::KeysEntered(
    device::BluetoothDevice* device,
    uint32_t entered) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::ConfirmPasskey(
    device::BluetoothDevice* device,
    uint32_t passkey) {
  confirmation_code_ = base::StringPrintf("%06d", passkey);
  ChangeStage(STAGE_WAITING_FOR_CODE_CONFIRMATION);
}

void BluetoothControllerPairingController::AuthorizePairing(
    device::BluetoothDevice* device) {
  // Disallow unknown device.
  device->RejectPairing();
}

}  // namespace pairing_chromeos
