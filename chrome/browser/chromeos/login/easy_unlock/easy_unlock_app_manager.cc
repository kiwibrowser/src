// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/easy_unlock/easy_unlock_app_manager.h"

#include <utility>

#include "base/command_line.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/sys_info.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/screenlock_private/screenlock_private_api.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/common/extensions/api/easy_unlock_private.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/proximity_auth/switches.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/one_shot_event.h"

namespace chromeos {

namespace {

class EasyUnlockAppManagerImpl : public EasyUnlockAppManager {
 public:
  EasyUnlockAppManagerImpl(extensions::ExtensionSystem* extension_system,
                           int manifest_id,
                           const base::FilePath& app_path);
  ~EasyUnlockAppManagerImpl() override;

  // EasyUnlockAppManager overrides.
  void EnsureReady(const base::Closure& ready_callback) override;
  void LaunchSetup() override;
  void LoadApp() override;
  void DisableAppIfLoaded() override;
  void ReloadApp() override;
  bool SendAuthAttemptEvent() override;

 private:
  extensions::ExtensionSystem* extension_system_;

  // The Easy Unlock app id.
  std::string app_id_;
  int manifest_id_;

  // The path from which Easy Unlock app should be loaded.
  base::FilePath app_path_;

  DISALLOW_COPY_AND_ASSIGN(EasyUnlockAppManagerImpl);
};

EasyUnlockAppManagerImpl::EasyUnlockAppManagerImpl(
    extensions::ExtensionSystem* extension_system,
    int manifest_id,
    const base::FilePath& app_path)
    : extension_system_(extension_system),
      app_id_(extension_misc::kEasyUnlockAppId),
      manifest_id_(manifest_id),
      app_path_(app_path) {}

EasyUnlockAppManagerImpl::~EasyUnlockAppManagerImpl() {}

void EasyUnlockAppManagerImpl::EnsureReady(
    const base::Closure& ready_callback) {
  extension_system_->ready().Post(FROM_HERE, ready_callback);
}

void EasyUnlockAppManagerImpl::LaunchSetup() {
  ExtensionService* extension_service = extension_system_->extension_service();
  if (!extension_service)
    return;

  const extensions::Extension* extension =
      extension_service->GetExtensionById(app_id_, false);
  if (!extension) {
    PA_LOG(WARNING) << "No extension";
    return;
  }

  OpenApplication(AppLaunchParams(extension_service->profile(), extension,
                                  extensions::LAUNCH_CONTAINER_WINDOW,
                                  WindowOpenDisposition::NEW_WINDOW,
                                  extensions::SOURCE_CHROME_INTERNAL));
}

void EasyUnlockAppManagerImpl::LoadApp() {
  ExtensionService* extension_service = extension_system_->extension_service();
  if (!extension_service)
    return;

  // TODO(xiyuan): Remove this when the app is bundled with chrome.
  if (!base::SysInfo::IsRunningOnChromeOS() &&
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          proximity_auth::switches::kForceLoadEasyUnlockAppInTests)) {
    return;
  }

  if (app_path_.empty())
    return;

  extensions::ComponentLoader* loader = extension_service->component_loader();
  if (!loader->Exists(app_id_))
    app_id_ = loader->Add(manifest_id_, app_path_);

  extension_service->EnableExtension(app_id_);
}

void EasyUnlockAppManagerImpl::DisableAppIfLoaded() {
  ExtensionService* extension_service = extension_system_->extension_service();
  if (!extension_service)
    return;

  if (!extension_service->component_loader()->Exists(app_id_))
    return;

  extension_service->DisableExtension(
      app_id_, extensions::disable_reason::DISABLE_RELOAD);
}

void EasyUnlockAppManagerImpl::ReloadApp() {
  ExtensionService* extension_service = extension_system_->extension_service();
  if (!extension_service)
    return;

  if (!extension_service->component_loader()->Exists(app_id_))
    return;

  extension_service->ReloadExtension(app_id_);
}

bool EasyUnlockAppManagerImpl::SendAuthAttemptEvent() {
  ExtensionService* extension_service = extension_system_->extension_service();
  if (!extension_service)
    return false;

  // TODO(tbarzic): Restrict this to EasyUnlock app.
  extensions::ScreenlockPrivateEventRouter* screenlock_router =
      extensions::ScreenlockPrivateEventRouter::GetFactoryInstance()->Get(
          extension_service->profile());
  return screenlock_router->OnAuthAttempted(
      proximity_auth::mojom::AuthType::USER_CLICK, std::string());
}

}  // namespace

EasyUnlockAppManager::~EasyUnlockAppManager() {}

// static
std::unique_ptr<EasyUnlockAppManager> EasyUnlockAppManager::Create(
    extensions::ExtensionSystem* extension_system,
    int manifest_id,
    const base::FilePath& app_path) {
  return std::unique_ptr<EasyUnlockAppManager>(
      new EasyUnlockAppManagerImpl(extension_system, manifest_id, app_path));
}

}  // namespace chromeos
