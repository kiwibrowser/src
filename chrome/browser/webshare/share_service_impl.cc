// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/webshare/share_service_impl.h"

#include <algorithm>
#include <functional>
#include <map>
#include <utility>

#include "base/strings/string_util.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/webshare/webshare_target.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/blink/public/common/manifest/manifest_share_target_util.h"

ShareServiceImpl::ShareServiceImpl() : weak_factory_(this) {}
ShareServiceImpl::~ShareServiceImpl() = default;

// static
void ShareServiceImpl::Create(blink::mojom::ShareServiceRequest request) {
  mojo::MakeStrongBinding(std::make_unique<ShareServiceImpl>(),
                          std::move(request));
}

void ShareServiceImpl::ShowPickerDialog(
    std::vector<WebShareTarget> targets,
    chrome::WebShareTargetPickerCallback callback) {
  // TODO(mgiuca): Get the browser window as |parent_window|.
  chrome::ShowWebShareTargetPickerDialog(
      nullptr /* parent_window */, std::move(targets), std::move(callback));
}

Browser* ShareServiceImpl::GetBrowser() {
  return BrowserList::GetInstance()->GetLastActive();
}

void ShareServiceImpl::OpenTargetURL(const GURL& target_url) {
  Browser* browser = GetBrowser();
  chrome::AddTabAt(browser, target_url,
                   browser->tab_strip_model()->active_index() + 1, true);
}

PrefService* ShareServiceImpl::GetPrefService() {
  return GetBrowser()->profile()->GetPrefs();
}

blink::mojom::EngagementLevel ShareServiceImpl::GetEngagementLevel(
    const GURL& url) {
  SiteEngagementService* site_engagement_service =
      SiteEngagementService::Get(GetBrowser()->profile());
  return site_engagement_service->GetEngagementLevel(url);
}

std::vector<WebShareTarget>
ShareServiceImpl::GetTargetsWithSufficientEngagement() {
  constexpr blink::mojom::EngagementLevel kMinimumEngagementLevel =
      blink::mojom::EngagementLevel::LOW;

  PrefService* pref_service = GetPrefService();

  const base::DictionaryValue* share_targets_dict =
      pref_service->GetDictionary(prefs::kWebShareVisitedTargets);

  std::vector<WebShareTarget> sufficiently_engaged_targets;
  for (const auto& it : *share_targets_dict) {
    GURL manifest_url(it.first);
    // This should not happen, but if the prefs file is corrupted, it might, so
    // don't (D)CHECK, just continue gracefully.
    if (!manifest_url.is_valid())
      continue;

    if (GetEngagementLevel(manifest_url) < kMinimumEngagementLevel)
      continue;

    const base::DictionaryValue* share_target_dict;
    bool result = it.second->GetAsDictionary(&share_target_dict);
    DCHECK(result);

    std::string name;
    share_target_dict->GetString("name", &name);
    std::string url_template;
    share_target_dict->GetString("url_template", &url_template);

    sufficiently_engaged_targets.emplace_back(std::move(manifest_url),
                                              std::move(name),
                                              GURL(std::move(url_template)));
  }

  return sufficiently_engaged_targets;
}

void ShareServiceImpl::Share(const std::string& title,
                             const std::string& text,
                             const GURL& share_url,
                             ShareCallback callback) {
  std::vector<WebShareTarget> sufficiently_engaged_targets =
      GetTargetsWithSufficientEngagement();

  ShowPickerDialog(std::move(sufficiently_engaged_targets),
                   base::BindOnce(&ShareServiceImpl::OnPickerClosed,
                                  weak_factory_.GetWeakPtr(), title, text,
                                  share_url, std::move(callback)));
}

void ShareServiceImpl::OnPickerClosed(const std::string& title,
                                      const std::string& text,
                                      const GURL& share_url,
                                      ShareCallback callback,
                                      const WebShareTarget* result) {
  if (result == nullptr) {
    std::move(callback).Run(blink::mojom::ShareError::CANCELED);
    return;
  }

  GURL url_template_filled;
  if (!blink::ReplaceWebShareUrlPlaceholders(result->url_template(), title,
                                             text, share_url,
                                             &url_template_filled)) {
    // This error should not be possible at share time. content::ManifestParser
    // should have filtered out invalid targets at manifest parse time.
    std::move(callback).Run(blink::mojom::ShareError::INTERNAL_ERROR);
    return;
  }

  // User should not be able to cause an invalid target URL. The replaced pieces
  // are escaped. If somehow we slip through this DCHECK, it will just open
  // about:blank.
  DCHECK(url_template_filled.is_valid());
  OpenTargetURL(url_template_filled);

  std::move(callback).Run(blink::mojom::ShareError::OK);
}
