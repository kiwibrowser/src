// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_display_host.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/app_launch_controller.h"
#include "chrome/browser/chromeos/login/arc_kiosk_controller.h"
#include "chrome/browser/chromeos/login/demo_mode/demo_app_launcher.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/mobile_config.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/system/device_disabling_manager.h"
#include "chrome/browser/ui/ash/wallpaper_controller_client.h"

namespace chromeos {

// static
LoginDisplayHost* LoginDisplayHost::default_host_ = nullptr;

LoginDisplayHost::LoginDisplayHost() {
  DCHECK(default_host() == nullptr);
  default_host_ = this;
}

LoginDisplayHost::~LoginDisplayHost() {
  default_host_ = nullptr;
}

}  // namespace chromeos
