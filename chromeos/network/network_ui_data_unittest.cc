// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/network_ui_data.h"

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

TEST(NetworkUIDataTest, ONCSource) {
  base::DictionaryValue ui_data_dict;

  ui_data_dict.SetString(NetworkUIData::kKeyONCSource, "user_import");
  {
    NetworkUIData ui_data(ui_data_dict);
    EXPECT_EQ(::onc::ONC_SOURCE_USER_IMPORT, ui_data.onc_source());
  }

  ui_data_dict.SetString(NetworkUIData::kKeyONCSource, "device_policy");
  {
    NetworkUIData ui_data(ui_data_dict);
    EXPECT_EQ(::onc::ONC_SOURCE_DEVICE_POLICY, ui_data.onc_source());
  }
  ui_data_dict.SetString(NetworkUIData::kKeyONCSource, "user_policy");
  {
    NetworkUIData ui_data(ui_data_dict);
    EXPECT_EQ(::onc::ONC_SOURCE_USER_POLICY, ui_data.onc_source());
  }
}

}  // namespace chromeos
