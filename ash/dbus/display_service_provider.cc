// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/dbus/display_service_provider.h"

#include <utility>

#include "ash/public/interfaces/ash_display_controller.mojom.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "ash/wm/screen_dimmer.h"
#include "base/bind.h"
#include "base/callback.h"
#include "dbus/message.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/base/user_activity/user_activity_detector.h"
#include "ui/display/manager/display_configurator.h"

namespace ash {
namespace {

void OnDisplayOwnershipChanged(
    const dbus::ExportedObject::ResponseSender& response_sender,
    std::unique_ptr<dbus::Response> response,
    bool status) {
  dbus::MessageWriter writer(response.get());
  writer.AppendBool(status);
  response_sender.Run(std::move(response));
}

}  // namespace

class DisplayServiceProvider::Impl {
 public:
  Impl() = default;
  ~Impl() = default;

  void SetDimming(bool dimmed);
  void TakeDisplayOwnership(base::OnceCallback<void(bool)> callback);
  void ReleaseDisplayOwnership(base::OnceCallback<void(bool)> callback);

 private:
  // Tests may not have a service_manager::Connector. Connect() is called
  // whenever ash_display_controller_ is used to lazily connect as needed.
  bool Connect();

  mojom::AshDisplayControllerPtr ash_display_controller_;
  std::unique_ptr<ScreenDimmer> screen_dimmer_;

  DISALLOW_COPY_AND_ASSIGN(Impl);
};

bool DisplayServiceProvider::Impl::Connect() {
  if (ash_display_controller_)
    return true;
  Shell::Get()->shell_delegate()->GetShellConnector()->BindInterface(
      mojom::kServiceName, &ash_display_controller_);
  return !!ash_display_controller_;
}

void DisplayServiceProvider::Impl::SetDimming(bool dimmed) {
  if (!screen_dimmer_) {
    screen_dimmer_ =
        std::make_unique<ScreenDimmer>(ScreenDimmer::Container::ROOT);
  }
  screen_dimmer_->SetDimming(dimmed);
}

void DisplayServiceProvider::Impl::TakeDisplayOwnership(
    base::OnceCallback<void(bool)> callback) {
  if (!Connect()) {
    LOG(ERROR) << "Display Controller not connected";
    std::move(callback).Run(false);
    return;
  }
  ash_display_controller_->TakeDisplayControl(std::move(callback));
}

void DisplayServiceProvider::Impl::ReleaseDisplayOwnership(
    base::OnceCallback<void(bool)> callback) {
  if (!Connect()) {
    LOG(ERROR) << "Display Controller not connected";
    std::move(callback).Run(false);
    return;
  }
  ash_display_controller_->RelinquishDisplayControl(std::move(callback));
}

DisplayServiceProvider::DisplayServiceProvider()
    : impl_(std::make_unique<Impl>()), weak_ptr_factory_(this) {}

DisplayServiceProvider::~DisplayServiceProvider() = default;

void DisplayServiceProvider::Start(
    scoped_refptr<dbus::ExportedObject> exported_object) {
  exported_object->ExportMethod(
      chromeos::kDisplayServiceInterface,
      chromeos::kDisplayServiceSetPowerMethod,
      base::BindRepeating(&DisplayServiceProvider::SetDisplayPower,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindRepeating(&DisplayServiceProvider::OnExported,
                          weak_ptr_factory_.GetWeakPtr()));

  exported_object->ExportMethod(
      chromeos::kDisplayServiceInterface,
      chromeos::kDisplayServiceSetSoftwareDimmingMethod,
      base::BindRepeating(&DisplayServiceProvider::SetDisplaySoftwareDimming,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindRepeating(&DisplayServiceProvider::OnExported,
                          weak_ptr_factory_.GetWeakPtr()));

  exported_object->ExportMethod(
      chromeos::kDisplayServiceInterface,
      chromeos::kDisplayServiceTakeOwnershipMethod,
      base::BindRepeating(&DisplayServiceProvider::TakeDisplayOwnership,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindRepeating(&DisplayServiceProvider::OnExported,
                          weak_ptr_factory_.GetWeakPtr()));

  exported_object->ExportMethod(
      chromeos::kDisplayServiceInterface,
      chromeos::kDisplayServiceReleaseOwnershipMethod,
      base::BindRepeating(&DisplayServiceProvider::ReleaseDisplayOwnership,
                          weak_ptr_factory_.GetWeakPtr()),
      base::BindRepeating(&DisplayServiceProvider::OnExported,
                          weak_ptr_factory_.GetWeakPtr()));
}

void DisplayServiceProvider::SetDisplayPower(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  dbus::MessageReader reader(method_call);
  int int_state = 0;
  if (!reader.PopInt32(&int_state)) {
    LOG(ERROR) << "Unable to parse request: "
               << chromeos::kDisplayServiceSetPowerMethod;
    response_sender.Run(dbus::Response::FromMethodCall(method_call));
    return;
  }

  // Turning displays off when the device becomes idle or on just before
  // we suspend may trigger a mouse move, which would then be incorrectly
  // reported as user activity.  Let the UserActivityDetector
  // know so that it can ignore such events.
  ui::UserActivityDetector::Get()->OnDisplayPowerChanging();

  Shell::Get()->display_configurator()->SetDisplayPower(
      static_cast<chromeos::DisplayPowerState>(int_state),
      display::DisplayConfigurator::kSetDisplayPowerNoFlags,
      base::BindRepeating(
          [](dbus::MethodCall* method_call,
             dbus::ExportedObject::ResponseSender response_sender,
             bool /*status*/) {
            response_sender.Run(dbus::Response::FromMethodCall(method_call));
          },
          method_call, response_sender));
}

void DisplayServiceProvider::SetDisplaySoftwareDimming(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  dbus::MessageReader reader(method_call);
  bool dimmed = false;
  if (reader.PopBool(&dimmed)) {
    impl_->SetDimming(dimmed);
  } else {
    LOG(ERROR) << "Unable to parse request: "
               << chromeos::kDisplayServiceSetSoftwareDimmingMethod;
  }
  response_sender.Run(dbus::Response::FromMethodCall(method_call));
}

void DisplayServiceProvider::TakeDisplayOwnership(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  impl_->TakeDisplayOwnership(base::BindOnce(
      &OnDisplayOwnershipChanged, response_sender,
      base::Passed(dbus::Response::FromMethodCall(method_call))));
}

void DisplayServiceProvider::ReleaseDisplayOwnership(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  impl_->ReleaseDisplayOwnership(base::BindOnce(
      &OnDisplayOwnershipChanged, response_sender,
      base::Passed(dbus::Response::FromMethodCall(method_call))));
}

void DisplayServiceProvider::OnExported(const std::string& interface_name,
                                        const std::string& method_name,
                                        bool success) {
  if (!success)
    LOG(ERROR) << "Failed to export " << interface_name << "." << method_name;
}

}  // namespace ash
