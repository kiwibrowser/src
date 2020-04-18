// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/search_resource_manager.h"

#include <memory>

#include "chrome/browser/ui/app_list/app_list_model_updater.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace app_list {

SearchResourceManager::SearchResourceManager(Profile* profile,
                                             AppListModelUpdater* model_updater)
    : model_updater_(model_updater) {
  // Give |SearchBoxModel| tablet and clamshell A11y Announcements.
  model_updater_->SetSearchTabletAndClamshellAccessibleName(
      l10n_util::GetStringUTF16(IDS_SEARCH_BOX_ACCESSIBILITY_NAME_TABLET),
      l10n_util::GetStringUTF16(IDS_SEARCH_BOX_ACCESSIBILITY_NAME));
  model_updater_->SetSearchHintText(
      l10n_util::GetStringUTF16(IDS_SEARCH_BOX_HINT_FULLSCREEN));
}

SearchResourceManager::~SearchResourceManager() {
}

}  // namespace app_list
