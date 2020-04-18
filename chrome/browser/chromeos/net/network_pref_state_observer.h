// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NET_NETWORK_PREF_STATE_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_NET_NETWORK_PREF_STATE_OBSERVER_H_

#include "base/macros.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Profile;

namespace chromeos {

// Class to update NetworkHandler when the PrefService state changes. The
// implementation currently relies on g_browser_process since it holds the
// default PrefService.
class NetworkPrefStateObserver : public content::NotificationObserver {
 public:
  NetworkPrefStateObserver();
  ~NetworkPrefStateObserver() override;

  // content::NotificationObserver
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:
  void InitializeNetworkPrefServices(Profile* profile);

  content::NotificationRegistrar notification_registrar_;

  DISALLOW_COPY_AND_ASSIGN(NetworkPrefStateObserver);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NET_NETWORK_PREF_STATE_OBSERVER_H_
