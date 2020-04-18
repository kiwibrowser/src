// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/search_webstore_result.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/search/search_util.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/theme_resources.h"
#include "extensions/common/extension_urls.h"
#include "net/base/url_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace app_list {

SearchWebstoreResult::SearchWebstoreResult(
    Profile* profile,
    AppListControllerDelegate* controller,
    const std::string& query)
    : profile_(profile),
      controller_(controller),
      query_(query),
      launch_url_(extension_urls::GetWebstoreSearchPageUrl(query)) {
  set_id(launch_url_.spec());
  set_relevance(0.0);
  SetResultType(ash::SearchResultType::kWebStoreApp);
  SetTitle(base::UTF8ToUTF16(query));

  const base::string16 details =
      l10n_util::GetStringUTF16(IDS_EXTENSION_WEB_STORE_TITLE);
  Tags details_tags;
  details_tags.push_back(Tag(ash::SearchResultTag::DIM, 0, details.length()));

  SetDetails(details);
  SetDetailsTags(details_tags);

  SetIcon(*ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      IDR_WEBSTORE_ICON_32));
}

SearchWebstoreResult::~SearchWebstoreResult() {}

void SearchWebstoreResult::Open(int event_flags) {
  RecordHistogram(WEBSTORE_SEARCH_RESULT);
  const GURL store_url = net::AppendQueryParameter(
      launch_url_,
      extension_urls::kWebstoreSourceField,
      extension_urls::kLaunchSourceAppListSearch);

  controller_->OpenURL(profile_,
                       store_url,
                       ui::PAGE_TRANSITION_LINK,
                       ui::DispositionFromEventFlags(event_flags));
}

}  // namespace app_list
