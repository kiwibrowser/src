// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_ARC_ARC_PAI_STARTER_H_
#define CHROME_BROWSER_UI_APP_LIST_ARC_ARC_PAI_STARTER_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"

class PrefService;

namespace content {
class BrowserContext;
}

namespace arc {

// Helper class that starts Play Auto Install flow when all conditions are met:
// The Play Store app is ready and there is no lock for PAI flow.
class ArcPaiStarter : public ArcAppListPrefs::Observer {
 public:
  ArcPaiStarter(content::BrowserContext* context, PrefService* pref_service);
  ~ArcPaiStarter() override;

  // Creates PAI starter in case it has not been executed for the requested
  // |context|.
  static std::unique_ptr<ArcPaiStarter> CreateIfNeeded(
      content::BrowserContext* context,
      PrefService* pref_service);

  // Locks PAI to be run on the Play Store app is ready.
  void AcquireLock();

  // Unlocks PAI to be run on the Play Store app is ready. If the Play Store app
  // is ready at this moment then PAI is started immediately.
  void ReleaseLock();

  // Registers callback that is called once PAI has been started. If PAI is
  // started already then callback is called immediately.
  void AddOnStartCallback(base::OnceClosure callback);

  // Returns true if lock was acquired.
  bool locked() const { return locked_; }

  bool started() const { return started_; }

 private:
  void MaybeStartPai();

  // ArcAppListPrefs::Observer:
  void OnAppRegistered(const std::string& app_id,
                       const ArcAppListPrefs::AppInfo& app_info) override;
  void OnAppReadyChanged(const std::string& app_id, bool ready) override;

  content::BrowserContext* const context_;
  PrefService* const pref_service_;
  std::vector<base::OnceClosure> onstart_callbacks_;
  bool locked_ = false;
  bool started_ = false;

  DISALLOW_COPY_AND_ASSIGN(ArcPaiStarter);
};

}  // namespace arc

#endif  // CHROME_BROWSER_UI_APP_LIST_ARC_ARC_PAI_STARTER_H_
