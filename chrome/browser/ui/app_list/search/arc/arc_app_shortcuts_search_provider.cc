// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/arc/arc_app_shortcuts_search_provider.h"

#include <memory>
#include <utility>

#include "base/i18n/string_search.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/app_list/search/arc/arc_app_shortcut_search_result.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"

namespace app_list {

ArcAppShortcutsSearchProvider::ArcAppShortcutsSearchProvider(
    Profile* profile,
    AppListControllerDelegate* list_controller)
    : profile_(profile),
      list_controller_(list_controller),
      weak_ptr_factory_(this) {}

ArcAppShortcutsSearchProvider::~ArcAppShortcutsSearchProvider() = default;

void ArcAppShortcutsSearchProvider::Start(const base::string16& query) {
  arc::mojom::AppInstance* app_instance =
      arc::ArcServiceManager::Get()
          ? ARC_GET_INSTANCE_FOR_METHOD(
                arc::ArcServiceManager::Get()->arc_bridge_service()->app(),
                GetAppShortcutItems)
          : nullptr;

  if (!app_instance || query.empty()) {
    ClearResults();
    return;
  }

  // Invalidate the weak ptr to prevent previous callback run.
  weak_ptr_factory_.InvalidateWeakPtrs();
  // Pass empty package name to do query for all packages.
  app_instance->GetAppShortcutItems(
      std::string(),
      base::BindOnce(&ArcAppShortcutsSearchProvider::OnGetAppShortcutItems,
                     weak_ptr_factory_.GetWeakPtr(), query));
}

void ArcAppShortcutsSearchProvider::OnGetAppShortcutItems(
    const base::string16& query,
    std::vector<arc::mojom::AppShortcutItemPtr> shortcut_items) {
  SearchProvider::Results search_results;
  base::i18n::FixedPatternStringSearchIgnoringCaseAndAccents finder(query);
  for (auto& item : shortcut_items) {
    // TODO(warx): Use tokenized string match.
    const base::string16& short_label = base::UTF8ToUTF16(item->short_label);
    if (!finder.Search(short_label, nullptr, nullptr))
      continue;

    search_results.emplace_back(std::make_unique<ArcAppShortcutSearchResult>(
        std::move(item), profile_, list_controller_));
    DCHECK(!short_label.empty());
    search_results.back()->set_relevance(query.length() / short_label.length());
  }

  SwapResults(&search_results);
}

}  // namespace app_list
