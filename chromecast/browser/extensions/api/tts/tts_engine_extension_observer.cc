// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// PLEASE NOTE: this is a copy with modifications from
// /chrome/browser/speech/extension_api
// It is temporary until a refactoring to move the chrome TTS implementation up
// into components and extensions/components can be completed.

#include "chromecast/browser/extensions/api/tts/tts_engine_extension_observer.h"

#include "base/logging.h"
#include "base/memory/singleton.h"
#include "chromecast/browser/extensions/api/tts/tts_engine_extension_api.h"
#include "chromecast/browser/tts/tts_controller.h"
#include "chromecast/common/extensions_api/tts/tts_engine_manifest_handler.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/event_router_factory.h"

// Factory to load one instance of TtsExtensionLoaderChromeOs per profile.
class TtsEngineExtensionObserverFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static TtsEngineExtensionObserver* GetForBrowserContext(
      content::BrowserContext* browser_context) {
    return static_cast<TtsEngineExtensionObserver*>(
        GetInstance()->GetServiceForBrowserContext(browser_context, true));
  }

  static TtsEngineExtensionObserverFactory* GetInstance() {
    return base::Singleton<TtsEngineExtensionObserverFactory>::get();
  }

 private:
  friend struct base::DefaultSingletonTraits<TtsEngineExtensionObserverFactory>;

  TtsEngineExtensionObserverFactory()
      : BrowserContextKeyedServiceFactory(
            "TtsEngineExtensionObserver",
            BrowserContextDependencyManager::GetInstance()) {
    DependsOn(extensions::EventRouterFactory::GetInstance());
  }

  ~TtsEngineExtensionObserverFactory() override {}

  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override {
    // If given an incognito profile (including the Chrome OS login
    // profile), share the service with the original profile.
    // TODO(rdaum): FIXME -- use this for chrome/chromeos, but for cast, etc.
    // just use original profile return
    // chrome::GetBrowserContextRedirectedInIncognito(context);
    return context;
  }

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* browser_context) const override {
    return new TtsEngineExtensionObserver(browser_context);
  }
};

TtsEngineExtensionObserver* TtsEngineExtensionObserver::GetInstance(
    content::BrowserContext* browser_context) {
  return TtsEngineExtensionObserverFactory::GetInstance()->GetForBrowserContext(
      browser_context);
}

TtsEngineExtensionObserver::TtsEngineExtensionObserver(
    content::BrowserContext* browser_context)
    : extension_registry_observer_(this), browser_context_(browser_context) {
  extension_registry_observer_.Add(
      extensions::ExtensionRegistry::Get(browser_context_));

  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(browser_context_);
  DCHECK(event_router);
  event_router->RegisterObserver(this, tts_engine_events::kOnSpeak);
  event_router->RegisterObserver(this, tts_engine_events::kOnStop);
}

TtsEngineExtensionObserver::~TtsEngineExtensionObserver() {}

bool TtsEngineExtensionObserver::SawExtensionLoad(
    const std::string& extension_id,
    bool update) {
  bool previously_loaded =
      engine_extension_ids_.find(extension_id) != engine_extension_ids_.end();

  if (update)
    engine_extension_ids_.insert(extension_id);

  return previously_loaded;
}

const std::vector<extensions::TtsVoice>*
TtsEngineExtensionObserver::GetRuntimeVoices(const std::string extension_id) {
  auto it = extension_id_to_runtime_voices_.find(extension_id);
  if (it == extension_id_to_runtime_voices_.end())
    return nullptr;

  return &it->second->voices;
}

void TtsEngineExtensionObserver::SetRuntimeVoices(
    std::unique_ptr<extensions::TtsVoices> tts_voices,
    const std::string extension_id) {
  extension_id_to_runtime_voices_[extension_id] = std::move(tts_voices);
  TtsController::GetInstance()->VoicesChanged();
}

void TtsEngineExtensionObserver::Shutdown() {
  extensions::EventRouter::Get(browser_context_)->UnregisterObserver(this);
}

bool TtsEngineExtensionObserver::IsLoadedTtsEngine(
    const std::string& extension_id) {
  extensions::EventRouter* event_router =
      extensions::EventRouter::Get(browser_context_);
  DCHECK(event_router);
  if (event_router->ExtensionHasEventListener(extension_id,
                                              tts_engine_events::kOnSpeak) &&
      event_router->ExtensionHasEventListener(extension_id,
                                              tts_engine_events::kOnStop)) {
    return true;
  }

  return false;
}

void TtsEngineExtensionObserver::OnListenerAdded(
    const extensions::EventListenerInfo& details) {
  if (!IsLoadedTtsEngine(details.extension_id))
    return;

  TtsController::GetInstance()->VoicesChanged();
  engine_extension_ids_.insert(details.extension_id);
}

void TtsEngineExtensionObserver::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  size_t erase_count = 0;
  erase_count += engine_extension_ids_.erase(extension->id());
  erase_count += extension_id_to_runtime_voices_.erase(extension->id());
  if (erase_count > 0)
    TtsController::GetInstance()->VoicesChanged();
}
