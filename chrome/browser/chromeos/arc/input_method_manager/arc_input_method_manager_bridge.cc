// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/input_method_manager/arc_input_method_manager_bridge.h"

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"

namespace arc {

namespace {

// Singleton factory for ArcInputMethodManagerBridge
class ArcInputMethodManagerBridgeFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcInputMethodManagerBridge,
          ArcInputMethodManagerBridgeFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase
  static constexpr const char* kName = "ArcInputMethodManagerBridgeFactory";

  static ArcInputMethodManagerBridgeFactory* GetInstance() {
    return base::Singleton<ArcInputMethodManagerBridgeFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcInputMethodManagerBridgeFactory>;
  ArcInputMethodManagerBridgeFactory() = default;
  ~ArcInputMethodManagerBridgeFactory() override = default;
};

}  // anonymous namespace

// static
ArcInputMethodManagerBridge* ArcInputMethodManagerBridge::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcInputMethodManagerBridgeFactory::GetForBrowserContext(context);
}

// static
ArcInputMethodManagerBridge*
ArcInputMethodManagerBridge::GetForBrowserContextForTesting(
    content::BrowserContext* context) {
  return ArcInputMethodManagerBridgeFactory::GetForBrowserContextForTesting(
      context);
}

ArcInputMethodManagerBridge::ArcInputMethodManagerBridge(
    content::BrowserContext* context,
    ArcBridgeService* bridge_service)
    : bridge_service_(bridge_service) {
  bridge_service_->input_method_manager()->SetHost(this);
}

ArcInputMethodManagerBridge::~ArcInputMethodManagerBridge() {
  bridge_service_->input_method_manager()->SetHost(nullptr);
}

void ArcInputMethodManagerBridge::OnActiveImeChanged(
    const std::string& ime_id) {
  // Please see https://crbug.com/845079.
  NOTIMPLEMENTED();
}

void ArcInputMethodManagerBridge::OnImeInfoChanged(
    std::vector<mojom::ImeInfoPtr> ime_infos) {
  // Please see https://crbug.com/845079.
  NOTIMPLEMENTED();
}

}  // namespace arc
