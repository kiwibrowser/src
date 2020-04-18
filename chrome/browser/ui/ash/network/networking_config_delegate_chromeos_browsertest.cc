// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/network/networking_config_delegate_chromeos.h"

#include "ash/ash_view_ids.h"
#include "ash/public/interfaces/constants.mojom.h"
#include "ash/public/interfaces/system_tray_test_api.mojom.h"
#include "ash/strings/grit/ash_strings.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/test/test_utils.h"
#include "extensions/test/extension_test_message_listener.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

using NetworkingConfigDelegateChromeosTest = extensions::ExtensionBrowserTest;

// Tests that an extension registering itself as handling a Wi-Fi SSID updates
// the ash system tray network item.
IN_PROC_BROWSER_TEST_F(NetworkingConfigDelegateChromeosTest, SystemTrayItem) {
  // Load the extension and wait for the background page script to run. This
  // registers the extension as the network config handler for wifi1.
  ExtensionTestMessageListener listener("done", false);
  ASSERT_TRUE(
      LoadExtension(test_data_dir_.AppendASCII("networking_config_delegate")));
  ASSERT_TRUE(listener.WaitUntilSatisfied());

  // Connect to ash.
  ash::mojom::SystemTrayTestApiPtr tray_test_api;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &tray_test_api);

  // Show the network detail view.
  ash::mojom::SystemTrayTestApiAsyncWaiter wait_for(tray_test_api.get());
  wait_for.ShowDetailedView(ash::mojom::TrayItem::kNetwork);

  // Expect that the extension-controlled VPN item appears.
  base::string16 expected_tooltip = l10n_util::GetStringFUTF16(
      IDS_ASH_STATUS_TRAY_EXTENSION_CONTROLLED_WIFI,
      base::UTF8ToUTF16("NetworkingConfigDelegate test extension"));
  base::string16 tooltip;
  wait_for.GetBubbleViewTooltip(ash::VIEW_ID_EXTENSION_CONTROLLED_WIFI,
                                &tooltip);
  EXPECT_EQ(expected_tooltip, tooltip);
}

}  // namespace
