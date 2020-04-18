// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/options/network_config_view.h"

#include "chrome/test/base/in_process_browser_test.h"
#include "third_party/cros_system_api/dbus/shill/dbus-constants.h"

namespace chromeos {
namespace {

using NetworkConfigViewTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(NetworkConfigViewTest, ShowForType) {
  // An invalid network type does not create a dialog.
  EXPECT_FALSE(NetworkConfigView::ShowForType("invalid"));

  // Showing the dialog creates an always-on-top widget.
  NetworkConfigView* view = NetworkConfigView::ShowForType(shill::kTypeWifi);
  ASSERT_TRUE(view->GetWidget());
  EXPECT_TRUE(view->GetWidget()->IsVisible());
  EXPECT_TRUE(view->GetWidget()->IsAlwaysOnTop());

  // Showing again returns the same dialog.
  NetworkConfigView* repeat = NetworkConfigView::ShowForType(shill::kTypeWifi);
  EXPECT_EQ(view, repeat);

  view->GetWidget()->CloseNow();
}

IN_PROC_BROWSER_TEST_F(NetworkConfigViewTest, ShowForNetworkId) {
  // An invalid network ID does not create a dialog.
  EXPECT_FALSE(NetworkConfigView::ShowForNetworkId("invalid"));

  // Showing the dialog creates an always-on-top widget.
  NetworkConfigView* view = NetworkConfigView::ShowForNetworkId("wifi1_guid");
  ASSERT_TRUE(view->GetWidget());
  EXPECT_TRUE(view->GetWidget()->IsVisible());
  EXPECT_TRUE(view->GetWidget()->IsAlwaysOnTop());

  // Showing again returns the same dialog.
  NetworkConfigView* repeat = NetworkConfigView::ShowForNetworkId("wifi1_guid");
  EXPECT_EQ(view, repeat);

  view->GetWidget()->CloseNow();
}

}  // namespace
}  // namespace chromeos
