// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OAUTH2_TOKEN_SERVICE_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OAUTH2_TOKEN_SERVICE_FACTORY_H_

#include <queue>
#include <string>

#include "base/macros.h"

namespace chromeos {

class DeviceOAuth2TokenService;

class DeviceOAuth2TokenServiceFactory {
 public:
  // Returns the instance of the DeviceOAuth2TokenService singleton.  May return
  // NULL during browser startup and shutdown.  When calling Get(), either make
  // sure that your code executes after browser startup and before shutdown or
  // be careful to call Get() every time (instead of holding a pointer) and
  // check for NULL to handle cases where you might access
  // DeviceOAuth2TokenService during startup or shutdown.
  static DeviceOAuth2TokenService* Get();

  // Called by ChromeBrowserMainPartsChromeOS in order to bootstrap the
  // DeviceOAuth2TokenService instance after the required global data is
  // available (local state, request context getter and CrosSettings).
  static void Initialize();

  // Called by ChromeBrowserMainPartsChromeOS in order to shutdown the
  // DeviceOAuth2TokenService instance and cancel all in-flight requests before
  // the required global data is destroyed (local state, request context getter
  // and CrosSettings).
  static void Shutdown();

 private:
  DeviceOAuth2TokenServiceFactory();
  ~DeviceOAuth2TokenServiceFactory();

  DISALLOW_COPY_AND_ASSIGN(DeviceOAuth2TokenServiceFactory);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SETTINGS_DEVICE_OAUTH2_TOKEN_SERVICE_FACTORY_H_
