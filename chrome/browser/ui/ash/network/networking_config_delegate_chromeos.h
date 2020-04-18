// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_NETWORK_NETWORKING_CONFIG_DELEGATE_CHROMEOS_H_
#define CHROME_BROWSER_UI_ASH_NETWORK_NETWORKING_CONFIG_DELEGATE_CHROMEOS_H_

#include <string>

#include "ash/system/networking_config_delegate.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/browser_context.h"

namespace chromeos {

// A class which allows the ash tray to retrieve extension provided networking
// configuration through the networking config service.
class NetworkingConfigDelegateChromeos : public ash::NetworkingConfigDelegate {
 public:
  NetworkingConfigDelegateChromeos();
  ~NetworkingConfigDelegateChromeos() override;

  std::unique_ptr<const ExtensionInfo> LookUpExtensionForNetwork(
      const std::string& guid) override;

 private:
  std::string LookUpExtensionName(content::BrowserContext* context,
                                  std::string extension_id) const;

  DISALLOW_COPY_AND_ASSIGN(NetworkingConfigDelegateChromeos);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_ASH_NETWORK_NETWORKING_CONFIG_DELEGATE_CHROMEOS_H_
