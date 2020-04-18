// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/data/webui/ntp4_browsertest.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_manager.h"

NTP4LoggedInWebUITest::NTP4LoggedInWebUITest() {}

NTP4LoggedInWebUITest::~NTP4LoggedInWebUITest() {}

void NTP4LoggedInWebUITest::SetLoginName(const std::string& name) {
  SigninManagerFactory::GetForProfile(browser()->profile())->
      SetAuthenticatedAccountInfo(name, name);
}
