// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/crostini/crostini_remover.h"

#include <string>
#include <utility>

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/crostini/crostini_pref_names.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service.h"
#include "chrome/browser/chromeos/crostini/crostini_registry_service_factory.h"
#include "chrome/browser/chromeos/crostini/crostini_util.h"
#include "chrome/browser/component_updater/cros_component_installer_chromeos.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"

namespace crostini {

CrostiniRemover::CrostiniRemover(
    Profile* profile,
    std::string vm_name,
    std::string container_name,
    CrostiniManager::RemoveCrostiniCallback callback)
    : profile_(profile),
      vm_name_(vm_name),
      container_name_(container_name),
      callback_(std::move(callback)) {}

CrostiniRemover::~CrostiniRemover() = default;

void CrostiniRemover::RemoveCrostini() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  restart_id_ = CrostiniManager::GetInstance()->RestartCrostini(
      profile_, vm_name_, container_name_,
      base::BindOnce(&CrostiniRemover::OnRestartCrostini, this), this);
}

void CrostiniRemover::OnConciergeStarted(ConciergeClientResult result) {
  // Abort RestartCrostini after it has started the Concierge service, and
  // before it starts the VM.
  CrostiniManager::GetInstance()->AbortRestartCrostini(profile_, restart_id_);
  // Now that we have started the Concierge service, we can use it to stop the
  // VM.
  if (result != ConciergeClientResult::SUCCESS) {
    std::move(callback_).Run(result);
    return;
  }
  CrostiniManager::GetInstance()->StopVm(
      profile_, kCrostiniDefaultVmName,
      base::BindOnce(&CrostiniRemover::StopVmFinished, this));
}

void CrostiniRemover::OnRestartCrostini(ConciergeClientResult result) {
  DCHECK_NE(result, ConciergeClientResult::SUCCESS)
      << "RestartCrostini should have been aborted after starting the "
         "Concierge service.";
  LOG(ERROR) << "Failed to start concierge service";
}

void CrostiniRemover::StopVmFinished(ConciergeClientResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (result != ConciergeClientResult::SUCCESS) {
    std::move(callback_).Run(result);
    return;
  }
  CrostiniRegistryServiceFactory::GetForProfile(profile_)->ClearApplicationList(
      kCrostiniDefaultVmName, kCrostiniDefaultContainerName);
  CrostiniManager::GetInstance()->DestroyDiskImage(
      CryptohomeIdForProfile(profile_), base::FilePath(kCrostiniDefaultVmName),
      vm_tools::concierge::StorageLocation::STORAGE_CRYPTOHOME_ROOT,
      base::BindOnce(&CrostiniRemover::DestroyDiskImageFinished, this));
}

void CrostiniRemover::DestroyDiskImageFinished(ConciergeClientResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (result != ConciergeClientResult::SUCCESS) {
    std::move(callback_).Run(result);
    return;
  }
  // Only set kCrostiniEnabled to false once cleanup is completely finished.
  CrostiniManager::GetInstance()->StopConcierge(
      base::BindOnce(&CrostiniRemover::StopConciergeFinished, this));
}

void CrostiniRemover::StopConciergeFinished(bool success) {
  // The success parameter is never set by debugd.
  auto* cros_component_manager =
      g_browser_process->platform_part()->cros_component_manager();
  if (cros_component_manager) {
    if (cros_component_manager->Unload("cros-termina")) {
      profile_->GetPrefs()->SetBoolean(prefs::kCrostiniEnabled, false);
    }
  }
  std::move(callback_).Run(ConciergeClientResult::SUCCESS);
}

}  // namespace crostini
