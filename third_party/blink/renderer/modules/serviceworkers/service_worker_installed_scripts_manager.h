// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICEWORKERS_SERVICE_WORKER_INSTALLED_SCRIPTS_MANAGER_H_

#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_installed_scripts_manager.h"
#include "third_party/blink/renderer/core/workers/installed_scripts_manager.h"

namespace blink {

// ServiceWorkerInstalledScriptsManager provides the main script and imported
// scripts of an installed service worker. The scripts are streamed from the
// browser process in parallel with worker thread initialization.
class ServiceWorkerInstalledScriptsManager final
    : public InstalledScriptsManager {
 public:
  explicit ServiceWorkerInstalledScriptsManager(
      std::unique_ptr<WebServiceWorkerInstalledScriptsManager>);

  // InstalledScriptsManager implementation.
  bool IsScriptInstalled(const KURL& script_url) const override;
  ScriptStatus GetScriptData(const KURL& script_url,
                             ScriptData* out_script_data) override;

 private:
  std::unique_ptr<WebServiceWorkerInstalledScriptsManager> manager_;
};

}  // namespace blink

#endif  // WorkerInstalledScriptsManager_h
