// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_INPUT_METHOD_MANAGER_ARC_INPUT_METHOD_MANAGER_BRIDGE_H_
#define CHROME_BROWSER_CHROMEOS_ARC_INPUT_METHOD_MANAGER_ARC_INPUT_METHOD_MANAGER_BRIDGE_H_

#include <string>

#include "base/macros.h"
#include "components/arc/common/input_method_manager.mojom.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace arc {

class ArcBridgeService;

class ArcInputMethodManagerBridge : public KeyedService,
                                    public mojom::InputMethodManagerHost {
 public:
  // Returns the instance for the given BrowserContext, or nullptr if the
  // browser |context| is not allowed to use ARC.
  static ArcInputMethodManagerBridge* GetForBrowserContext(
      content::BrowserContext* context);
  static ArcInputMethodManagerBridge* GetForBrowserContextForTesting(
      content::BrowserContext* context);

  ArcInputMethodManagerBridge(content::BrowserContext* context,
                              ArcBridgeService* bridge_service);
  ~ArcInputMethodManagerBridge() override;

  // mojom::InputMethodManagerHost overrides:
  void OnActiveImeChanged(const std::string& ime_id) override;
  void OnImeInfoChanged(std::vector<mojom::ImeInfoPtr> ime_infos) override;

 private:
  ArcBridgeService* const bridge_service_;  // Owned by ArcServiceManager

  DISALLOW_COPY_AND_ASSIGN(ArcInputMethodManagerBridge);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_INPUT_METHOD_MANAGER_ARC_INPUT_METHOD_MANAGER_BRIDGE_H_
