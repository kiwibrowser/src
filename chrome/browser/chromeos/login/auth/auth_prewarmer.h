// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_AUTH_AUTH_PREWARMER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_AUTH_AUTH_PREWARMER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace chromeos {

class NetworkState;

// Class for prewarming authentication network connection.
class AuthPrewarmer : public NetworkStateHandlerObserver,
                      public content::NotificationObserver {
 public:
  AuthPrewarmer();
  ~AuthPrewarmer() override;

  void PrewarmAuthentication(base::OnceClosure completion_callback);

 private:
  // chromeos::NetworkStateHandlerObserver overrides.
  void DefaultNetworkChanged(const NetworkState* network) override;

  // content::NotificationObserver overrides.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  bool IsNetworkConnected() const;
  net::URLRequestContextGetter* GetRequestContext() const;
  void DoPrewarm();

  content::NotificationRegistrar registrar_;
  base::OnceClosure completion_callback_;
  bool doing_prewarm_;

  DISALLOW_COPY_AND_ASSIGN(AuthPrewarmer);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_AUTH_AUTH_PREWARMER_H_
