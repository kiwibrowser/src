// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MULTI_DEVICE_SETUP_MULTI_DEVICE_NOTIFICATION_PRESENTER_H_
#define ASH_MULTI_DEVICE_SETUP_MULTI_DEVICE_NOTIFICATION_PRESENTER_H_

#include <memory>
#include <string>

#include "ash/ash_export.h"
#include "ash/session/session_observer.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "chromeos/services/multidevice_setup/public/mojom/multidevice_setup.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace message_center {
class MessageCenter;
class Notification;
}  // namespace message_center

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace ash {

// Presents notifications necessary for MultiDevice setup flow. It observes the
// MultiDeviceSetup mojo service to show a notification when
// (1) a potential host is found for someone who has not gone through the setup
//     flow before,
// (2) the host has switched for someone who has, or
// (3) a new Chromebook has been added to an account for someone who has.
//
// The behavior caused by clicking a notification depends its content as
// described above:
// (1) triggers the setup UI to appear to prompt setup flow,
// (2) opens Settings/Connected Devices/Change Device, and
// (3) opens Setting/Connected Devices.
//
// Note that if one notification is showing and another one is triggered, the
// old text is replaced (if it's different) and the notification pops up again.
class ASH_EXPORT MultiDeviceNotificationPresenter
    : public chromeos::multidevice_setup::mojom::MultiDeviceSetupObserver,
      public SessionObserver {
 public:
  MultiDeviceNotificationPresenter(
      message_center::MessageCenter* message_center,
      service_manager::Connector* connector);
  ~MultiDeviceNotificationPresenter() override;

  // Removes the notification created by NotifyPotentialHostExists() or does
  // nothing if that notification is not currently displayed.
  // TODO(khorimoto): Change this to Mojo function.
  void RemoveMultiDeviceSetupNotification();

 protected:
  // multidevice_setup::mojom::MultiDeviceSetupObserver:
  void OnPotentialHostExistsForNewUser() override;
  void OnConnectedHostSwitchedForExistingUser() override;
  void OnNewChromebookAddedForExistingUser() override;

  // SessionObserver:
  void OnUserSessionAdded(const AccountId& account_id) override;
  void OnSessionStateChanged(session_manager::SessionState state) override;

 private:
  friend class MultiDeviceNotificationPresenterTest;

  // MultiDevice setup notification ID.
  static const char kNotificationId[];

  // These methods are delegated to a nested class to make them easier to stub
  // in unit tests. This way they can all be stubbed simultaneously by building
  // a test delegate class deriving from OpenUiDelegate.
  class OpenUiDelegate {
   public:
    virtual ~OpenUiDelegate();
    virtual void OpenMultiDeviceSetupUi();
    virtual void OpenChangeConnectedPhoneSettings();
    virtual void OpenConnectedDevicesSettings();
  };

  // Represents each possible MultiDevice setup notification that the setup flow
  // can show with a "none" option for the general state with no notification
  // present.
  enum class Status {
    kNoNotificationVisible,
    kNewUserNotificationVisible,
    kExistingUserHostSwitchedNotificationVisible,
    kExistingUserNewChromebookNotificationVisible
  };

  // Reflects MultiDeviceSetupNotification enum in enums.xml. Do not
  // rearrange.
  enum NotificationType {
    kNotificationTypeNewUserPotentialHostExists = 0,
    kNotificationTypeExistingUserHostSwitched = 1,
    kNotificationTypeExistingUserNewChromebookAdded = 2,
    kNotificationTypeMax
  };

  static NotificationType GetMetricValueForNotification(
      Status notification_status);

  static std::string GetNotificationDescriptionForLogging(
      Status notification_status);

  void ObserveMultiDeviceSetupIfPossible();
  void OnNotificationClicked();
  void ShowNotification(const Status notification_status,
                        const base::string16& title,
                        const base::string16& message);
  std::unique_ptr<message_center::Notification> CreateNotification(
      const base::string16& title,
      const base::string16& message);

  void FlushForTesting();

  message_center::MessageCenter* message_center_;
  service_manager::Connector* connector_;

  // Notification currently showing or
  // Status::kNoNotificationVisible if there isn't one.
  Status notification_status_ = Status::kNoNotificationVisible;

  chromeos::multidevice_setup::mojom::MultiDeviceSetupPtr
      multidevice_setup_ptr_;
  mojo::Binding<chromeos::multidevice_setup::mojom::MultiDeviceSetupObserver>
      binding_;

  std::unique_ptr<OpenUiDelegate> open_ui_delegate_;
  base::WeakPtrFactory<MultiDeviceNotificationPresenter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MultiDeviceNotificationPresenter);
};

}  // namespace ash

#endif  // ASH_MULTI_DEVICE_SETUP_MULTI_DEVICE_NOTIFICATION_PRESENTER_H_
