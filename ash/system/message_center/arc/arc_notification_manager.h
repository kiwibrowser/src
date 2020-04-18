// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_MESSAGE_CENTER_ARC_ARC_NOTIFICATION_MANAGER_H_
#define ASH_SYSTEM_MESSAGE_CENTER_ARC_ARC_NOTIFICATION_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/memory/weak_ptr.h"
#include "components/account_id/account_id.h"
#include "components/arc/common/notifications.mojom.h"
#include "components/arc/connection_holder.h"
#include "components/arc/connection_observer.h"
#include "ui/message_center/message_center.h"

namespace ash {

class ArcNotificationItem;
class ArcNotificationManagerDelegate;

class ArcNotificationManager
    : public arc::ConnectionObserver<arc::mojom::NotificationsInstance>,
      public arc::mojom::NotificationsHost {
 public:
  // Sets the factory function to create ARC notification views. Exposed for
  // testing.
  static void SetCustomNotificationViewFactory();

  ArcNotificationManager(
      std::unique_ptr<ArcNotificationManagerDelegate> delegate,
      const AccountId& main_profile_id,
      message_center::MessageCenter* message_center);

  ~ArcNotificationManager() override;

  void SetInstance(arc::mojom::NotificationsInstancePtr instance);

  arc::ConnectionHolder<arc::mojom::NotificationsInstance,
                        arc::mojom::NotificationsHost>*
  GetConnectionHolderForTest();

  // ConnectionObserver<arc::mojom::NotificationsInstance> implementation:
  void OnConnectionReady() override;
  void OnConnectionClosed() override;

  // arc::mojom::NotificationsHost implementation:
  void OnNotificationPosted(arc::mojom::ArcNotificationDataPtr data) override;
  void OnNotificationUpdated(arc::mojom::ArcNotificationDataPtr data) override;
  void OnNotificationRemoved(const std::string& key) override;
  void OpenMessageCenter() override;

  // Methods called from ArcNotificationItem:
  void SendNotificationRemovedFromChrome(const std::string& key);
  void SendNotificationClickedOnChrome(const std::string& key);
  void SendNotificationButtonClickedOnChrome(const std::string& key,
                                             int button_index);
  void CreateNotificationWindow(const std::string& key);
  void CloseNotificationWindow(const std::string& key);
  void OpenNotificationSettings(const std::string& key);
  bool IsOpeningSettingsSupported() const;
  void SendNotificationToggleExpansionOnChrome(const std::string& key);

 private:
  // Helper class to own MojoChannel and ConnectionHolder.
  class InstanceOwner;

  bool ShouldIgnoreNotification(arc::mojom::ArcNotificationData* data);

  // Invoked when |get_app_id_callback_| gets back the app id.
  void OnGotAppId(arc::mojom::ArcNotificationDataPtr data,
                  const std::string& app_id);

  std::unique_ptr<ArcNotificationManagerDelegate> delegate_;
  const AccountId main_profile_id_;
  message_center::MessageCenter* const message_center_;

  using ItemMap =
      std::unordered_map<std::string, std::unique_ptr<ArcNotificationItem>>;
  ItemMap items_;

  bool ready_ = false;

  std::unique_ptr<InstanceOwner> instance_owner_;

  base::WeakPtrFactory<ArcNotificationManager> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ArcNotificationManager);
};

}  // namespace ash

#endif  // ASH_SYSTEM_MESSAGE_CENTER_ARC_ARC_NOTIFICATION_MANAGER_H_
