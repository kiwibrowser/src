// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/md_history_ui.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/webui/browsing_history_handler.h"
#include "chrome/browser/ui/webui/foreign_session_handler.h"
#include "chrome/browser/ui/webui/history_login_handler.h"
#include "chrome/browser/ui/webui/metrics_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

constexpr char kIsUserSignedInKey[] = "isUserSignedIn";
constexpr char kShowMenuPromoKey[] = "showMenuPromo";

bool IsUserSignedIn(Profile* profile) {
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(profile);
  return signin_manager && signin_manager->IsAuthenticated();
}

bool MenuPromoShown(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(prefs::kMdHistoryMenuPromoShown);
}

content::WebUIDataSource* CreateMdHistoryUIHTMLSource(Profile* profile,
                                                      bool use_test_title) {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIHistoryHost);
  source->OverrideContentSecurityPolicyScriptSrc(
      "script-src chrome://resources 'self';");

  // Localized strings (alphabetical order).
  source->AddLocalizedString("bookmarked", IDS_HISTORY_ENTRY_BOOKMARKED);
  source->AddLocalizedString("cancel", IDS_CANCEL);
  source->AddLocalizedString("clearBrowsingData",
                             IDS_CLEAR_BROWSING_DATA_TITLE);
  source->AddLocalizedString("clearSearch", IDS_MD_HISTORY_CLEAR_SEARCH);
  source->AddLocalizedString("closeMenuPromo", IDS_MD_HISTORY_CLOSE_MENU_PROMO);
  source->AddLocalizedString("collapseSessionButton",
                             IDS_HISTORY_OTHER_SESSIONS_COLLAPSE_SESSION);
  source->AddLocalizedString("delete", IDS_MD_HISTORY_DELETE);
  source->AddLocalizedString("deleteConfirm",
                             IDS_HISTORY_DELETE_PRIOR_VISITS_CONFIRM_BUTTON);
  source->AddLocalizedString("deleteSession",
                             IDS_HISTORY_OTHER_SESSIONS_HIDE_FOR_NOW);
  source->AddLocalizedString("deleteWarning",
                             IDS_HISTORY_DELETE_PRIOR_VISITS_WARNING);
  source->AddLocalizedString("entrySummary", IDS_HISTORY_ENTRY_SUMMARY);
  source->AddLocalizedString("expandSessionButton",
                             IDS_HISTORY_OTHER_SESSIONS_EXPAND_SESSION);
  source->AddLocalizedString("foundSearchResults",
                             IDS_HISTORY_FOUND_SEARCH_RESULTS);
  source->AddLocalizedString("historyMenuButton",
                             IDS_MD_HISTORY_HISTORY_MENU_DESCRIPTION);
  source->AddLocalizedString("historyMenuItem",
                             IDS_MD_HISTORY_HISTORY_MENU_ITEM);
  source->AddLocalizedString("itemsSelected", IDS_MD_HISTORY_ITEMS_SELECTED);
  source->AddLocalizedString("loading", IDS_HISTORY_LOADING);
  source->AddLocalizedString("menuPromo", IDS_MD_HISTORY_MENU_PROMO);
  source->AddLocalizedString("moreActionsButton",
                             IDS_HISTORY_ACTION_MENU_DESCRIPTION);
  source->AddLocalizedString("moreFromSite", IDS_HISTORY_MORE_FROM_SITE);
  source->AddLocalizedString("openAll", IDS_HISTORY_OTHER_SESSIONS_OPEN_ALL);
  source->AddLocalizedString("openTabsMenuItem",
                             IDS_MD_HISTORY_OPEN_TABS_MENU_ITEM);
  source->AddLocalizedString("noResults", IDS_HISTORY_NO_RESULTS);
  source->AddLocalizedString("noSearchResults", IDS_HISTORY_NO_SEARCH_RESULTS);
  source->AddLocalizedString("noSyncedResults",
                             IDS_MD_HISTORY_NO_SYNCED_RESULTS);
  source->AddLocalizedString("removeBookmark", IDS_HISTORY_REMOVE_BOOKMARK);
  source->AddLocalizedString("removeFromHistory", IDS_HISTORY_REMOVE_PAGE);
  source->AddLocalizedString("removeSelected",
                             IDS_HISTORY_REMOVE_SELECTED_ITEMS);
  source->AddLocalizedString("searchPrompt", IDS_MD_HISTORY_SEARCH_PROMPT);
  source->AddLocalizedString("searchResult", IDS_HISTORY_SEARCH_RESULT);
  source->AddLocalizedString("searchResults", IDS_HISTORY_SEARCH_RESULTS);
  source->AddLocalizedString("signInButton", IDS_MD_HISTORY_SIGN_IN_BUTTON);
  source->AddLocalizedString("signInPromo", IDS_MD_HISTORY_SIGN_IN_PROMO);
  source->AddLocalizedString("signInPromoDesc",
                             IDS_MD_HISTORY_SIGN_IN_PROMO_DESC);
  if (use_test_title)
    source->AddString("title", "MD History");
  else
    source->AddLocalizedString("title", IDS_HISTORY_TITLE);

  source->AddString(
      "sidebarFooter",
      l10n_util::GetStringFUTF16(
          IDS_HISTORY_OTHER_FORMS_OF_HISTORY,
          l10n_util::GetStringUTF16(
              IDS_SETTINGS_CLEAR_DATA_MYACTIVITY_URL_IN_HISTORY)));

  PrefService* prefs = profile->GetPrefs();
  bool allow_deleting_history =
      prefs->GetBoolean(prefs::kAllowDeletingBrowserHistory);
  source->AddBoolean("allowDeletingHistory", allow_deleting_history);

  source->AddBoolean(kShowMenuPromoKey, !MenuPromoShown(profile));
  source->AddBoolean("isGuestSession", profile->IsGuestSession());

  source->AddBoolean(kIsUserSignedInKey, IsUserSignedIn(profile));

  struct UncompressedResource {
    const char* path;
    int idr;
  };
  const UncompressedResource uncompressed_resources[] = {
    {"constants.html", IDR_MD_HISTORY_CONSTANTS_HTML},
    {"constants.js", IDR_MD_HISTORY_CONSTANTS_JS},
    {"history.js", IDR_MD_HISTORY_HISTORY_JS},
    {"images/100/sign_in_promo.jpg",
     IDR_MD_HISTORY_IMAGES_100_SIGN_IN_PROMO_JPG},
    {"images/200/sign_in_promo.jpg",
     IDR_MD_HISTORY_IMAGES_200_SIGN_IN_PROMO_JPG},
#if !BUILDFLAG(OPTIMIZE_WEBUI)
    {"app.html", IDR_MD_HISTORY_APP_HTML},
    {"app.js", IDR_MD_HISTORY_APP_JS},
    {"browser_service.html", IDR_MD_HISTORY_BROWSER_SERVICE_HTML},
    {"browser_service.js", IDR_MD_HISTORY_BROWSER_SERVICE_JS},
    {"history_item.html", IDR_MD_HISTORY_HISTORY_ITEM_HTML},
    {"history_item.js", IDR_MD_HISTORY_HISTORY_ITEM_JS},
    {"history_list.html", IDR_MD_HISTORY_HISTORY_LIST_HTML},
    {"history_list.js", IDR_MD_HISTORY_HISTORY_LIST_JS},
    {"history_toolbar.html", IDR_MD_HISTORY_HISTORY_TOOLBAR_HTML},
    {"history_toolbar.js", IDR_MD_HISTORY_HISTORY_TOOLBAR_JS},
    {"icons.html", IDR_MD_HISTORY_ICONS_HTML},
    {"lazy_load.html", IDR_MD_HISTORY_LAZY_LOAD_HTML},
    {"query_manager.html", IDR_MD_HISTORY_QUERY_MANAGER_HTML},
    {"query_manager.js", IDR_MD_HISTORY_QUERY_MANAGER_JS},
    {"router.html", IDR_MD_HISTORY_ROUTER_HTML},
    {"router.js", IDR_MD_HISTORY_ROUTER_JS},
    {"searched_label.html", IDR_MD_HISTORY_SEARCHED_LABEL_HTML},
    {"searched_label.js", IDR_MD_HISTORY_SEARCHED_LABEL_JS},
    {"shared_style.html", IDR_MD_HISTORY_SHARED_STYLE_HTML},
    {"shared_vars.html", IDR_MD_HISTORY_SHARED_VARS_HTML},
    {"side_bar.html", IDR_MD_HISTORY_SIDE_BAR_HTML},
    {"side_bar.js", IDR_MD_HISTORY_SIDE_BAR_JS},
    {"synced_device_card.html", IDR_MD_HISTORY_SYNCED_DEVICE_CARD_HTML},
    {"synced_device_card.js", IDR_MD_HISTORY_SYNCED_DEVICE_CARD_JS},
    {"synced_device_manager.html", IDR_MD_HISTORY_SYNCED_DEVICE_MANAGER_HTML},
    {"synced_device_manager.js", IDR_MD_HISTORY_SYNCED_DEVICE_MANAGER_JS},
#endif
  };

  std::vector<std::string> exclude_from_gzip;
  for (const auto& resource : uncompressed_resources) {
    source->AddResourcePath(resource.path, resource.idr);
    exclude_from_gzip.push_back(resource.path);
  }
  source->UseGzip(exclude_from_gzip);

#if BUILDFLAG(OPTIMIZE_WEBUI)
  source->AddResourcePath("app.html",
                          IDR_MD_HISTORY_APP_VULCANIZED_HTML);
  source->AddResourcePath("app.crisper.js",
                          IDR_MD_HISTORY_APP_CRISPER_JS);
  source->AddResourcePath("lazy_load.html",
                          IDR_MD_HISTORY_LAZY_LOAD_VULCANIZED_HTML);
  source->AddResourcePath("lazy_load.crisper.js",
                          IDR_MD_HISTORY_LAZY_LOAD_CRISPER_JS);
#endif

  source->SetDefaultResource(IDR_MD_HISTORY_HISTORY_HTML);
  source->SetJsonPath("strings.js");

  return source;
}

}  // namespace

bool MdHistoryUI::use_test_title_ = false;

MdHistoryUI::MdHistoryUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* data_source =
      CreateMdHistoryUIHTMLSource(profile, use_test_title_);
  content::WebUIDataSource::Add(profile, data_source);

  web_ui->AddMessageHandler(std::make_unique<BrowsingHistoryHandler>());
  web_ui->AddMessageHandler(std::make_unique<MetricsHandler>());

  web_ui->AddMessageHandler(
      std::make_unique<browser_sync::ForeignSessionHandler>());
  web_ui->AddMessageHandler(std::make_unique<HistoryLoginHandler>(
      base::Bind(&MdHistoryUI::UpdateDataSource, base::Unretained(this))));

  web_ui->RegisterMessageCallback(
      "menuPromoShown", base::BindRepeating(&MdHistoryUI::HandleMenuPromoShown,
                                            base::Unretained(this)));
}

MdHistoryUI::~MdHistoryUI() {}

void MdHistoryUI::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kMdHistoryMenuPromoShown, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

void MdHistoryUI::UpdateDataSource() {
  CHECK(web_ui());

  Profile* profile = Profile::FromWebUI(web_ui());

  std::unique_ptr<base::DictionaryValue> update(new base::DictionaryValue);
  update->SetBoolean(kIsUserSignedInKey, IsUserSignedIn(profile));
  update->SetBoolean(kShowMenuPromoKey, !MenuPromoShown(profile));

  content::WebUIDataSource::Update(profile, chrome::kChromeUIHistoryHost,
                                   std::move(update));
}

void MdHistoryUI::HandleMenuPromoShown(const base::ListValue* args) {
  Profile::FromWebUI(web_ui())->GetPrefs()->SetBoolean(
      prefs::kMdHistoryMenuPromoShown, true);
  UpdateDataSource();
}
