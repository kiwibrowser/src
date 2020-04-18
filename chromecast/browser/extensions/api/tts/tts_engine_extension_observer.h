// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// PLEASE NOTE: this is a copy with modifications from
// /chrome/browser/speech/extension_api
// It is temporary until a refactoring to move the chrome TTS implementation up
// into components and extensions/components can be completed.

#ifndef CHROMECAST_BROWSER_EXTENSIONS_API_TTS_TTS_ENGINE_EXTENSION_OBSERVER_H_
#define CHROMECAST_BROWSER_EXTENSIONS_API_TTS_TTS_ENGINE_EXTENSION_OBSERVER_H_

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {
struct TtsVoice;
struct TtsVoices;
};  // namespace extensions

// BrowserContext-keyed class that observes the extension registry to determine
// load of extension-based tts engines.
class TtsEngineExtensionObserver
    : public KeyedService,
      public extensions::EventRouter::Observer,
      public extensions::ExtensionRegistryObserver {
 public:
  static TtsEngineExtensionObserver* GetInstance(
      content::BrowserContext* browser_context);

  // Returns if this observer saw the given extension load. Adds |extension_id|
  // as loaded immediately if |update| is set to true.
  bool SawExtensionLoad(const std::string& extension_id, bool update);

  // Gets the currently loaded TTS extension ids.
  const std::set<std::string> GetTtsExtensions();

  // Gets voices for |extension_id| updated through TtsEngine.updateVoices.
  const std::vector<extensions::TtsVoice>* GetRuntimeVoices(
      const std::string extension_id);

  // Called to update the voices list for the given extension. This overrides
  // voices declared in the extension manifest.
  void SetRuntimeVoices(std::unique_ptr<extensions::TtsVoices> tts_voices,
                        const std::string extension_id);

  // Implementation of KeyedService.
  void Shutdown() override;

  // Implementation of extensions::EventRouter::Observer.
  void OnListenerAdded(const extensions::EventListenerInfo& details) override;

  // extensions::ExtensionRegistryObserver overrides.
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const extensions::Extension* extension,
                           extensions::UnloadedExtensionReason reason) override;

 private:
  explicit TtsEngineExtensionObserver(content::BrowserContext* browser_context);
  ~TtsEngineExtensionObserver() override;

  bool IsLoadedTtsEngine(const std::string& extension_id);

  ScopedObserver<extensions::ExtensionRegistry,
                 extensions::ExtensionRegistryObserver>
      extension_registry_observer_;

  content::BrowserContext* browser_context_;

  std::set<std::string> engine_extension_ids_;

  std::map<std::string, std::unique_ptr<extensions::TtsVoices>>
      extension_id_to_runtime_voices_;

  friend class TtsEngineExtensionObserverFactory;

  DISALLOW_COPY_AND_ASSIGN(TtsEngineExtensionObserver);
};

#endif  // CHROMECAST_BROWSER_EXTENSIONS_API_TTS_TTS_ENGINE_EXTENSION_OBSERVER_H_
