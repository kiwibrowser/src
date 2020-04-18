// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_pai_starter.h"

#include <memory>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "components/arc/arc_prefs.h"
#include "components/prefs/pref_service.h"
#include "ui/events/event_constants.h"

namespace arc {

ArcPaiStarter::ArcPaiStarter(content::BrowserContext* context,
                             PrefService* pref_service)
    : context_(context), pref_service_(pref_service) {
  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(context_);
  // Prefs may not available in some unit tests.
  if (!prefs)
    return;
  prefs->AddObserver(this);
  MaybeStartPai();
}

ArcPaiStarter::~ArcPaiStarter() {
  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(context_);
  if (!prefs)
    return;
  prefs->RemoveObserver(this);
}

// static
std::unique_ptr<ArcPaiStarter> ArcPaiStarter::CreateIfNeeded(
    content::BrowserContext* context,
    PrefService* pref_service) {
  if (pref_service->GetBoolean(prefs::kArcPaiStarted))
    return std::unique_ptr<ArcPaiStarter>();
  return std::make_unique<ArcPaiStarter>(context, pref_service);
}

void ArcPaiStarter::AcquireLock() {
  DCHECK(!locked_);
  locked_ = true;
}

void ArcPaiStarter::ReleaseLock() {
  DCHECK(locked_);
  locked_ = false;
  MaybeStartPai();
}

void ArcPaiStarter::AddOnStartCallback(base::OnceClosure callback) {
  if (started_) {
    std::move(callback).Run();
    return;
  }

  onstart_callbacks_.push_back(std::move(callback));
}

void ArcPaiStarter::MaybeStartPai() {
  if (started_ || locked_)
    return;

  ArcAppListPrefs* prefs = ArcAppListPrefs::Get(context_);
  DCHECK(prefs);
  std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
      prefs->GetApp(kPlayStoreAppId);
  if (!app_info || !app_info->ready)
    return;

  started_ = true;
  StartPaiFlow();
  // TODO(khmel): Currently PAI flow is black-box for us. We can only start it
  // and rely that the Play Store will handle all cases. Ideally we need some
  // callback, notifying us that PAI flow finished successfully.
  pref_service_->SetBoolean(prefs::kArcPaiStarted, true);

  prefs->RemoveObserver(this);

  for (auto& callback : onstart_callbacks_)
    std::move(callback).Run();
  onstart_callbacks_.clear();
}

void ArcPaiStarter::OnAppRegistered(const std::string& app_id,
                                    const ArcAppListPrefs::AppInfo& app_info) {
  OnAppReadyChanged(app_id, app_info.ready);
}

void ArcPaiStarter::OnAppReadyChanged(const std::string& app_id, bool ready) {
  if (app_id == kPlayStoreAppId && ready)
    MaybeStartPai();
}

}  // namespace arc
