// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_flash.h"

#include "ppapi/c/pp_macros.h"
#include "ppapi/c/private/ppb_flash.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/private/flash.h"
#include "ppapi/cpp/var.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(Flash);

using pp::flash::Flash;
using pp::Var;

TestFlash::TestFlash(TestingInstance* instance)
    : TestCase(instance),
      callback_factory_(this) {
}

void TestFlash::RunTests(const std::string& filter) {
  RUN_TEST(SetInstanceAlwaysOnTop, filter);
  RUN_TEST(GetProxyForURL, filter);
  RUN_TEST(GetLocalTimeZoneOffset, filter);
  RUN_TEST(GetCommandLineArgs, filter);
  RUN_TEST(GetSetting, filter);
  RUN_TEST(SetCrashData, filter);
}

std::string TestFlash::TestSetInstanceAlwaysOnTop() {
  Flash::SetInstanceAlwaysOnTop(instance_, PP_TRUE);
  Flash::SetInstanceAlwaysOnTop(instance_, PP_FALSE);
  PASS();
}

std::string TestFlash::TestGetProxyForURL() {
  Var result = Flash::GetProxyForURL(instance_, "http://127.0.0.1/foobar/");
  ASSERT_TRUE(result.is_string());
  // Assume no one configures a proxy for localhost.
  ASSERT_EQ("DIRECT", result.AsString());

  result = Flash::GetProxyForURL(instance_, "http://www.google.com");
  // Don't know what the proxy might be, but it should be a valid result.
  ASSERT_TRUE(result.is_string());

  result = Flash::GetProxyForURL(instance_, "file:///tmp");
  ASSERT_TRUE(result.is_string());
  // Should get "DIRECT" for file:// URLs.
  ASSERT_EQ("DIRECT", result.AsString());

  result = Flash::GetProxyForURL(instance_, "this_isnt_an_url");
  // Should be an error.
  ASSERT_TRUE(result.is_undefined());

  PASS();
}

std::string TestFlash::TestGetLocalTimeZoneOffset() {
  double result = Flash::GetLocalTimeZoneOffset(instance_, 1321491298.0);
  // The result depends on the local time zone, but +/- 14h from UTC should
  // cover the possibilities.
  ASSERT_TRUE(result >= -14 * 60 * 60);
  ASSERT_TRUE(result <= 14 * 60 * 60);

  PASS();
}

std::string TestFlash::TestGetCommandLineArgs() {
  Var result = Flash::GetCommandLineArgs(pp::Module::Get());
  ASSERT_TRUE(result.is_string());

  PASS();
}

std::string TestFlash::TestGetSetting() {
  Var is_3denabled = Flash::GetSetting(instance_, PP_FLASHSETTING_3DENABLED);
  ASSERT_TRUE(is_3denabled.is_bool());

  Var is_incognito = Flash::GetSetting(instance_, PP_FLASHSETTING_INCOGNITO);
  ASSERT_TRUE(is_incognito.is_bool());

  Var is_stage3denabled = Flash::GetSetting(instance_,
                                            PP_FLASHSETTING_STAGE3DENABLED);
  // This may "fail" if 3d isn't enabled.
  ASSERT_TRUE(is_stage3denabled.is_bool() ||
              (is_stage3denabled.is_undefined() && !is_3denabled.AsBool()));

  Var num_cores = Flash::GetSetting(instance_, PP_FLASHSETTING_NUMCORES);
  ASSERT_TRUE(num_cores.is_int() && num_cores.AsInt() > 0);

  Var lso_restrictions = Flash::GetSetting(instance_,
                                           PP_FLASHSETTING_LSORESTRICTIONS);
  ASSERT_TRUE(lso_restrictions.is_int());
  int32_t lso_restrictions_int = lso_restrictions.AsInt();
  ASSERT_TRUE(lso_restrictions_int == PP_FLASHLSORESTRICTIONS_NONE ||
              lso_restrictions_int == PP_FLASHLSORESTRICTIONS_BLOCK ||
              lso_restrictions_int == PP_FLASHLSORESTRICTIONS_IN_MEMORY);

  // Invalid instance cases.
  Var result = Flash::GetSetting(
      pp::InstanceHandle(static_cast<PP_Instance>(0)),
      PP_FLASHSETTING_3DENABLED);
  ASSERT_TRUE(result.is_undefined());
  result = Flash::GetSetting(pp::InstanceHandle(static_cast<PP_Instance>(0)),
                             PP_FLASHSETTING_INCOGNITO);
  ASSERT_TRUE(result.is_undefined());
  result = Flash::GetSetting(pp::InstanceHandle(static_cast<PP_Instance>(0)),
                             PP_FLASHSETTING_STAGE3DENABLED);
  ASSERT_TRUE(result.is_undefined());

  PASS();
}

std::string TestFlash::TestSetCrashData() {
  pp::Var url("http://...");
  ASSERT_TRUE(Flash::SetCrashData(instance_, PP_FLASHCRASHKEY_URL, url));

  PASS();
}
