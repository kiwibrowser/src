// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media_router/media_router_webui_message_handler.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/browser/media/router/media_router_metrics.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/webui/media_router/media_router_ui.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/web_ui.h"
#include "extensions/common/constants.h"
#include "ui/base/l10n/l10n_util.h"

namespace media_router {

namespace {

const char kCastLearnMorePageUrl[] =
    "https://www.google.com/chrome/devices/chromecast/learn.html";
const char kHelpPageUrlPrefix[] =
    "https://support.google.com/chromecast/answer/%d";

// Message names.
const char kRequestInitialData[] = "requestInitialData";
const char kCreateRoute[] = "requestRoute";
const char kAcknowledgeFirstRunFlow[] = "acknowledgeFirstRunFlow";
const char kActOnIssue[] = "actOnIssue";
const char kCloseRoute[] = "closeRoute";
const char kJoinRoute[] = "joinRoute";
const char kCloseDialog[] = "closeDialog";
const char kReportBlur[] = "reportBlur";
const char kReportClickedSinkIndex[] = "reportClickedSinkIndex";
const char kReportFilter[] = "reportFilter";
const char kReportInitialAction[] = "reportInitialAction";
const char kReportInitialState[] = "reportInitialState";
const char kReportNavigateToView[] = "reportNavigateToView";
const char kReportRouteCreationOutcome[] = "reportRouteCreationOutcome";
const char kReportRouteCreation[] = "reportRouteCreation";
const char kReportSelectedCastMode[] = "reportSelectedCastMode";
const char kReportSinkCount[] = "reportSinkCount";
const char kReportTimeToClickSink[] = "reportTimeToClickSink";
const char kReportTimeToInitialActionClose[] = "reportTimeToInitialActionClose";
const char kReportWebUIRouteControllerLoaded[] =
    "reportWebUIRouteControllerLoaded";
const char kSearchSinksAndCreateRoute[] = "searchSinksAndCreateRoute";
const char kOnInitialDataReceived[] = "onInitialDataReceived";
const char kOnMediaControllerAvailable[] = "onMediaControllerAvailable";
const char kOnMediaControllerClosed[] = "onMediaControllerClosed";
const char kPauseCurrentMedia[] = "pauseCurrentMedia";
const char kPlayCurrentMedia[] = "playCurrentMedia";
const char kSeekCurrentMedia[] = "seekCurrentMedia";
const char kSelectLocalMediaFile[] = "selectLocalMediaFile";
const char kSetCurrentMediaMute[] = "setCurrentMediaMute";
const char kSetCurrentMediaVolume[] = "setCurrentMediaVolume";
const char kSetMediaRemotingEnabled[] = "setMediaRemotingEnabled";
const char kHangoutsSetLocalPresent[] = "hangouts.setLocalPresent";

// JS function names.
const char kSetInitialData[] = "media_router.ui.setInitialData";
const char kOnCreateRouteResponseReceived[] =
    "media_router.ui.onCreateRouteResponseReceived";
const char kOnRouteControllerInvalidated[] =
    "media_router.ui.onRouteControllerInvalidated";
const char kReceiveSearchResult[] = "media_router.ui.receiveSearchResult";
const char kSetFirstRunFlowData[] = "media_router.ui.setFirstRunFlowData";
const char kSetIssue[] = "media_router.ui.setIssue";
const char kSetSinkListAndIdentity[] = "media_router.ui.setSinkListAndIdentity";
const char kSetRouteList[] = "media_router.ui.setRouteList";
const char kSetCastModeList[] = "media_router.ui.setCastModeList";
const char kUpdateMaxHeight[] = "media_router.ui.updateMaxHeight";
const char kUpdateRouteStatus[] = "media_router.ui.updateRouteStatus";
const char kUserSelectedLocalMediaFile[] =
    "media_router.ui.userSelectedLocalMediaFile";
const char kWindowOpen[] = "window.open";

std::unique_ptr<base::DictionaryValue> SinksAndIdentityToValue(
    const std::vector<MediaSinkWithCastModes>& sinks,
    const AccountInfo& account_info) {
  auto sink_list_and_identity = std::make_unique<base::DictionaryValue>();
  bool show_email = false;
  bool show_domain = false;
  std::string user_domain;
  if (account_info.IsValid()) {
    user_domain = account_info.hosted_domain;
    sink_list_and_identity->SetString("userEmail", account_info.email);
  }

  auto sinks_val = std::make_unique<base::ListValue>();

  for (const MediaSinkWithCastModes& sink_with_cast_modes : sinks) {
    auto sink_val = std::make_unique<base::DictionaryValue>();

    const MediaSink& sink = sink_with_cast_modes.sink;
    sink_val->SetString("id", sink.id());
    sink_val->SetString("name", sink.name());
    sink_val->SetInteger("iconType", static_cast<int>(sink.icon_type()));
    if (sink.description())
      sink_val->SetString("description", *sink.description());

    bool is_pseudo_sink =
        base::StartsWith(sink.id(), "pseudo:", base::CompareCase::SENSITIVE);
    if (!user_domain.empty() && sink.domain() && !sink.domain()->empty()) {
      std::string domain = *sink.domain();
      // Convert default domains to user domain
      if (domain == "default") {
        domain = user_domain;
        if (domain == AccountTrackerService::kNoHostedDomainFound) {
          // Default domain will be empty for non-dasher accounts.
          domain.clear();
        }
      }

      sink_val->SetString("domain", domain);

      show_email = show_email || !is_pseudo_sink;
      if (!domain.empty() && domain != user_domain) {
        show_domain = true;
      }
    }

    int cast_mode_bits = 0;
    for (MediaCastMode cast_mode : sink_with_cast_modes.cast_modes)
      cast_mode_bits |= cast_mode;

    sink_val->SetInteger("castModes", cast_mode_bits);
    sink_val->SetBoolean("isPseudoSink", is_pseudo_sink);
    sinks_val->Append(std::move(sink_val));
  }

  sink_list_and_identity->Set("sinks", std::move(sinks_val));
  sink_list_and_identity->SetBoolean("showEmail", show_email);
  sink_list_and_identity->SetBoolean("showDomain", show_domain);
  return sink_list_and_identity;
}

std::unique_ptr<base::DictionaryValue> RouteToValue(
    const MediaRoute& route,
    bool can_join,
    bool incognito,
    int current_cast_mode) {
  auto dictionary = std::make_unique<base::DictionaryValue>();
  dictionary->SetString("id", route.media_route_id());
  dictionary->SetString("sinkId", route.media_sink_id());
  dictionary->SetString("description", route.description());
  dictionary->SetBoolean("isLocal", route.is_local());
  dictionary->SetBoolean("supportsWebUiController",
                         route.controller_type() != RouteControllerType::kNone);
  dictionary->SetBoolean("canJoin", can_join);
  if (current_cast_mode > 0) {
    dictionary->SetInteger("currentCastMode", current_cast_mode);
  }

  return dictionary;
}

std::unique_ptr<base::ListValue> CastModesToValue(
    const CastModeSet& cast_modes,
    const std::string& source_host,
    base::Optional<MediaCastMode> forced_cast_mode) {
  auto value = std::make_unique<base::ListValue>();

  for (const MediaCastMode& cast_mode : cast_modes) {
    auto cast_mode_val = std::make_unique<base::DictionaryValue>();
    cast_mode_val->SetInteger("type", cast_mode);
    cast_mode_val->SetString(
        "description", MediaCastModeToDescription(cast_mode, source_host));
    cast_mode_val->SetString("host", source_host);
    cast_mode_val->SetBoolean(
        "isForced", forced_cast_mode && forced_cast_mode == cast_mode);
    value->Append(std::move(cast_mode_val));
  }

  return value;
}

// Returns an Issue dictionary created from |issue| that can be used in WebUI.
std::unique_ptr<base::DictionaryValue> IssueToValue(const Issue& issue) {
  const IssueInfo& issue_info = issue.info();
  auto dictionary = std::make_unique<base::DictionaryValue>();
  dictionary->SetInteger("id", issue.id());
  dictionary->SetString("title", issue_info.title);
  dictionary->SetString("message", issue_info.message);
  dictionary->SetInteger("defaultActionType",
                         static_cast<int>(issue_info.default_action));
  if (!issue_info.secondary_actions.empty()) {
    DCHECK_EQ(1u, issue_info.secondary_actions.size());
    dictionary->SetInteger("secondaryActionType",
                           static_cast<int>(issue_info.secondary_actions[0]));
  }
  if (!issue_info.route_id.empty())
    dictionary->SetString("routeId", issue_info.route_id);
  dictionary->SetBoolean("isBlocking", issue_info.is_blocking);
  if (issue_info.help_page_id > 0)
    dictionary->SetInteger("helpPageId", issue_info.help_page_id);

  return dictionary;
}

bool IsValidIssueActionTypeNum(int issue_action_type_num) {
  return issue_action_type_num >= 0 &&
         issue_action_type_num <=
             static_cast<int>(IssueInfo::Action::NUM_VALUES);
}

// Composes a "learn more" URL. The URL depends on template arguments in |args|.
// Returns an empty string if |args| is invalid.
std::string GetLearnMoreUrl(const base::DictionaryValue* args) {
  // TODO(imcheng): The template arguments for determining the learn more URL
  // should come from the Issue object in the browser, not from WebUI.
  int help_page_id = -1;
  if (!args->GetInteger("helpPageId", &help_page_id) || help_page_id < 0) {
    DVLOG(1) << "Invalid help page id.";
    return std::string();
  }

  std::string help_url = base::StringPrintf(kHelpPageUrlPrefix, help_page_id);
  if (!GURL(help_url).is_valid()) {
    DVLOG(1) << "Error: URL is invalid and cannot be opened.";
    return std::string();
  }
  return help_url;
}

}  // namespace

MediaRouterWebUIMessageHandler::MediaRouterWebUIMessageHandler(
    MediaRouterUI* media_router_ui)
    : incognito_(
          Profile::FromWebUI(media_router_ui->web_ui())->IsOffTheRecord()),
      dialog_closing_(false),
      media_router_ui_(media_router_ui) {}

MediaRouterWebUIMessageHandler::~MediaRouterWebUIMessageHandler() {}

void MediaRouterWebUIMessageHandler::UpdateSinks(
    const std::vector<MediaSinkWithCastModes>& sinks) {
  DVLOG(2) << "UpdateSinks";
  std::unique_ptr<base::DictionaryValue> sinks_and_identity_val(
      SinksAndIdentityToValue(sinks, GetAccountInfo()));
  web_ui()->CallJavascriptFunctionUnsafe(kSetSinkListAndIdentity,
                                         *sinks_and_identity_val);
}

void MediaRouterWebUIMessageHandler::UpdateRoutes(
    const std::vector<MediaRoute>& routes,
    const std::vector<MediaRoute::Id>& joinable_route_ids,
    const std::unordered_map<MediaRoute::Id, MediaCastMode>&
        current_cast_modes) {
  std::unique_ptr<base::ListValue> routes_val(
      RoutesToValue(routes, joinable_route_ids, current_cast_modes));

  web_ui()->CallJavascriptFunctionUnsafe(kSetRouteList, *routes_val);
}

void MediaRouterWebUIMessageHandler::UpdateCastModes(
    const CastModeSet& cast_modes,
    const std::string& source_host,
    base::Optional<MediaCastMode> forced_cast_mode) {
  DVLOG(2) << "UpdateCastModes";
  std::unique_ptr<base::ListValue> cast_modes_val(
      CastModesToValue(cast_modes, source_host, forced_cast_mode));
  web_ui()->CallJavascriptFunctionUnsafe(kSetCastModeList, *cast_modes_val);
}

void MediaRouterWebUIMessageHandler::OnCreateRouteResponseReceived(
    const MediaSink::Id& sink_id,
    const MediaRoute* route) {
  DVLOG(2) << "OnCreateRouteResponseReceived";
  if (route) {
    int current_cast_mode = CurrentCastModeForRouteId(
        route->media_route_id(), media_router_ui_->routes_and_cast_modes());
    std::unique_ptr<base::DictionaryValue> route_value(
        RouteToValue(*route, false, incognito_, current_cast_mode));
    web_ui()->CallJavascriptFunctionUnsafe(kOnCreateRouteResponseReceived,
                                           base::Value(sink_id), *route_value,
                                           base::Value(route->for_display()));
  } else {
    web_ui()->CallJavascriptFunctionUnsafe(kOnCreateRouteResponseReceived,
                                           base::Value(sink_id), base::Value(),
                                           base::Value(false));
  }
}

void MediaRouterWebUIMessageHandler::ReturnSearchResult(
    const std::string& sink_id) {
  DVLOG(2) << "ReturnSearchResult";
  web_ui()->CallJavascriptFunctionUnsafe(kReceiveSearchResult,
                                         base::Value(sink_id));
}

void MediaRouterWebUIMessageHandler::UpdateIssue(const Issue& issue) {
  DVLOG(2) << "UpdateIssue";
  web_ui()->CallJavascriptFunctionUnsafe(kSetIssue, *IssueToValue(issue));
}

void MediaRouterWebUIMessageHandler::ClearIssue() {
  DVLOG(2) << "ClearIssue";
  web_ui()->CallJavascriptFunctionUnsafe(kSetIssue, base::Value());
}

void MediaRouterWebUIMessageHandler::UpdateMaxDialogHeight(int height) {
  DVLOG(2) << "UpdateMaxDialogHeight";
  web_ui()->CallJavascriptFunctionUnsafe(kUpdateMaxHeight, base::Value(height));
}

void MediaRouterWebUIMessageHandler::UpdateMediaRouteStatus(
    const MediaStatus& status) {
  current_media_status_ = base::make_optional<MediaStatus>(MediaStatus(status));

  base::DictionaryValue status_value;
  status_value.SetString("title", status.title);
  status_value.SetBoolean("canPlayPause", status.can_play_pause);
  status_value.SetBoolean("canMute", status.can_mute);
  status_value.SetBoolean("canSetVolume", status.can_set_volume);
  status_value.SetBoolean("canSeek", status.can_seek);
  status_value.SetInteger("playState", static_cast<int>(status.play_state));
  status_value.SetBoolean("isMuted", status.is_muted);
  status_value.SetInteger("duration", status.duration.InSeconds());
  status_value.SetInteger("currentTime", status.current_time.InSeconds());
  status_value.SetDouble("volume", status.volume);

  if (status.hangouts_extra_data) {
    base::Value hangouts_extra_data(base::Value::Type::DICTIONARY);
    hangouts_extra_data.SetKey(
        "localPresent", base::Value(status.hangouts_extra_data->local_present));
    status_value.SetKey("hangoutsExtraData", std::move(hangouts_extra_data));
  }

  if (status.mirroring_extra_data) {
    base::Value mirroring_extra_data(base::Value::Type::DICTIONARY);
    mirroring_extra_data.SetKey(
        "mediaRemotingEnabled",
        base::Value(status.mirroring_extra_data->media_remoting_enabled));
    status_value.SetKey("mirroringExtraData", std::move(mirroring_extra_data));
  }

  web_ui()->CallJavascriptFunctionUnsafe(kUpdateRouteStatus,
                                         std::move(status_value));
}

void MediaRouterWebUIMessageHandler::OnRouteControllerInvalidated() {
  web_ui()->CallJavascriptFunctionUnsafe(kOnRouteControllerInvalidated);
}

void MediaRouterWebUIMessageHandler::UserSelectedLocalMediaFile(
    base::FilePath::StringType file_name) {
  DVLOG(2) << "UserSelectedLocalMediaFile";
  web_ui()->CallJavascriptFunctionUnsafe(kUserSelectedLocalMediaFile,
                                         base::Value(file_name));
}

void MediaRouterWebUIMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      kRequestInitialData,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnRequestInitialData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kCreateRoute,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnCreateRoute,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kAcknowledgeFirstRunFlow,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnAcknowledgeFirstRunFlow,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kActOnIssue,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnActOnIssue,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kCloseRoute,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnCloseRoute,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kJoinRoute,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnJoinRoute,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kCloseDialog,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnCloseDialog,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportBlur,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnReportBlur,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportClickedSinkIndex,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnReportClickedSinkIndex,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportFilter,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnReportFilter,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportInitialState,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnReportInitialState,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportInitialAction,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnReportInitialAction,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportRouteCreation,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnReportRouteCreation,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportRouteCreationOutcome,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnReportRouteCreationOutcome,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportSelectedCastMode,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnReportSelectedCastMode,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportNavigateToView,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnReportNavigateToView,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportSinkCount,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnReportSinkCount,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportTimeToClickSink,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnReportTimeToClickSink,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportTimeToInitialActionClose,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnReportTimeToInitialActionClose,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kReportWebUIRouteControllerLoaded,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnReportWebUIRouteControllerLoaded,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kSearchSinksAndCreateRoute,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnSearchSinksAndCreateRoute,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kOnInitialDataReceived,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnInitialDataReceived,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kOnMediaControllerAvailable,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnMediaControllerAvailable,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kOnMediaControllerClosed,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnMediaControllerClosed,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kPauseCurrentMedia,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnPauseCurrentMedia,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kPlayCurrentMedia,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnPlayCurrentMedia,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kSeekCurrentMedia,
      base::BindRepeating(&MediaRouterWebUIMessageHandler::OnSeekCurrentMedia,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kSelectLocalMediaFile,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnSelectLocalMediaFile,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kSetCurrentMediaMute,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnSetCurrentMediaMute,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kSetCurrentMediaVolume,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnSetCurrentMediaVolume,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kSetMediaRemotingEnabled,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnSetMediaRemotingEnabled,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kHangoutsSetLocalPresent,
      base::BindRepeating(
          &MediaRouterWebUIMessageHandler::OnSetHangoutsLocalPresent,
          base::Unretained(this)));
}

void MediaRouterWebUIMessageHandler::OnRequestInitialData(
    const base::ListValue* args) {
  DVLOG(1) << "OnRequestInitialData";
  media_router_ui_->OnUIInitiallyLoaded();
  base::DictionaryValue initial_data;

  // "No Cast devices found?" Chromecast help center page.
  initial_data.SetString("deviceMissingUrl",
                         base::StringPrintf(kHelpPageUrlPrefix, 3249268));

  std::unique_ptr<base::DictionaryValue> sinks_and_identity(
      SinksAndIdentityToValue(media_router_ui_->GetEnabledSinks(),
                              GetAccountInfo()));
  initial_data.Set("sinksAndIdentity", std::move(sinks_and_identity));

  std::unique_ptr<base::ListValue> routes(RoutesToValue(
      media_router_ui_->routes(), media_router_ui_->joinable_route_ids(),
      media_router_ui_->routes_and_cast_modes()));
  initial_data.Set("routes", std::move(routes));

  const std::set<MediaCastMode> cast_modes = media_router_ui_->cast_modes();
  std::unique_ptr<base::ListValue> cast_modes_list(CastModesToValue(
      cast_modes, media_router_ui_->GetPresentationRequestSourceName(),
      media_router_ui_->forced_cast_mode()));
  initial_data.Set("castModes", std::move(cast_modes_list));

  // If the cast mode last chosen for the current origin is tab mirroring,
  // that should be the cast mode initially selected in the dialog. Otherwise
  // the initial cast mode should be chosen automatically by the dialog.
  bool use_tab_mirroring =
      base::ContainsKey(cast_modes, MediaCastMode::TAB_MIRROR) &&
      media_router_ui_->UserSelectedTabMirroringForCurrentOrigin();
  initial_data.SetBoolean("useTabMirroring", use_tab_mirroring);

  web_ui()->CallJavascriptFunctionUnsafe(kSetInitialData, initial_data);
  media_router_ui_->OnUIInitialized();
}

void MediaRouterWebUIMessageHandler::OnCreateRoute(
    const base::ListValue* args) {
  DVLOG(1) << "OnCreateRoute";
  const base::DictionaryValue* args_dict = nullptr;
  std::string sink_id;
  int cast_mode_num = -1;
  if (!args->GetDictionary(0, &args_dict) ||
      !args_dict->GetString("sinkId", &sink_id) ||
      !args_dict->GetInteger("selectedCastMode", &cast_mode_num)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }

  if (sink_id.empty()) {
    DVLOG(1) << "Media Route UI did not respond with a "
             << "valid sink ID. Aborting.";
    return;
  }

  if (!IsValidCastModeNum(cast_mode_num)) {
    // TODO(imcheng): Record error condition with UMA.
    DVLOG(1) << "Invalid cast mode: " << cast_mode_num << ". Aborting.";
    return;
  }

  MediaRouterUI* media_router_ui =
      static_cast<MediaRouterUI*>(web_ui()->GetController());
  if (media_router_ui->HasPendingRouteRequest()) {
    DVLOG(1) << "UI already has pending route request. Ignoring.";
    IssueInfo issue(
        l10n_util::GetStringUTF8(IDS_MEDIA_ROUTER_ISSUE_PENDING_ROUTE),
        IssueInfo::Action::DISMISS, IssueInfo::Severity::NOTIFICATION);
    media_router_ui_->AddIssue(issue);
    return;
  }

  DVLOG(2) << __func__ << ": sink id: " << sink_id
           << ", cast mode: " << cast_mode_num;

  // TODO(haibinlu): Pass additional parameters into the CreateRoute request,
  // e.g. low-fps-mirror, user-override. (crbug.com/490364)
  if (!media_router_ui->CreateRoute(
          sink_id, static_cast<MediaCastMode>(cast_mode_num))) {
    DVLOG(1) << "Error initiating route request.";
  }
}

void MediaRouterWebUIMessageHandler::OnAcknowledgeFirstRunFlow(
    const base::ListValue* args) {
  DVLOG(1) << "OnAcknowledgeFirstRunFlow";
  Profile::FromWebUI(web_ui())->GetPrefs()->SetBoolean(
      prefs::kMediaRouterFirstRunFlowAcknowledged, true);

  bool enabled_cloud_services = false;
  // Do not set the relevant cloud services prefs if the user was not shown
  // the cloud services prompt.
  if (!args->GetBoolean(0, &enabled_cloud_services)) {
    DVLOG(1) << "User was not shown the enable cloud services prompt.";
    return;
  }

  PrefService* pref_service = Profile::FromWebUI(web_ui())->GetPrefs();
  pref_service->SetBoolean(prefs::kMediaRouterEnableCloudServices,
                           enabled_cloud_services);
  pref_service->SetBoolean(prefs::kMediaRouterCloudServicesPrefSet, true);
}

void MediaRouterWebUIMessageHandler::OnActOnIssue(const base::ListValue* args) {
  DVLOG(1) << "OnActOnIssue";
  const base::DictionaryValue* args_dict = nullptr;
  Issue::Id issue_id;
  int action_type_num = -1;
  if (!args->GetDictionary(0, &args_dict) ||
      !args_dict->GetInteger("issueId", &issue_id) ||
      !args_dict->GetInteger("actionType", &action_type_num)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  if (!IsValidIssueActionTypeNum(action_type_num)) {
    DVLOG(1) << "Invalid action type: " << action_type_num;
    return;
  }
  IssueInfo::Action action_type =
      static_cast<IssueInfo::Action>(action_type_num);
  if (ActOnIssueType(action_type, args_dict))
    DVLOG(1) << "ActOnIssueType failed for Issue ID " << issue_id;
  media_router_ui_->ClearIssue(issue_id);
}

void MediaRouterWebUIMessageHandler::OnJoinRoute(const base::ListValue* args) {
  DVLOG(1) << "OnJoinRoute";
  const base::DictionaryValue* args_dict = nullptr;
  std::string route_id;
  std::string sink_id;
  if (!args->GetDictionary(0, &args_dict) ||
      !args_dict->GetString("sinkId", &sink_id) ||
      !args_dict->GetString("routeId", &route_id)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }

  if (sink_id.empty()) {
    DVLOG(1) << "Media Route UI did not respond with a "
             << "valid sink ID. Aborting.";
    return;
  }

  if (route_id.empty()) {
    DVLOG(1) << "Media Route UI did not respond with a "
             << "valid route ID. Aborting.";
    return;
  }

  MediaRouterUI* media_router_ui =
      static_cast<MediaRouterUI*>(web_ui()->GetController());
  if (media_router_ui->HasPendingRouteRequest()) {
    DVLOG(1) << "UI already has pending route request. Ignoring.";
    IssueInfo issue(
        l10n_util::GetStringUTF8(IDS_MEDIA_ROUTER_ISSUE_PENDING_ROUTE),
        IssueInfo::Action::DISMISS, IssueInfo::Severity::NOTIFICATION);
    media_router_ui_->AddIssue(issue);
    return;
  }

  if (!media_router_ui_->ConnectRoute(sink_id, route_id)) {
    DVLOG(1) << "Error initiating route join request.";
  }
}

void MediaRouterWebUIMessageHandler::OnCloseRoute(const base::ListValue* args) {
  DVLOG(1) << "OnCloseRoute";
  const base::DictionaryValue* args_dict = nullptr;
  std::string route_id;
  bool is_local = false;
  if (!args->GetDictionary(0, &args_dict) ||
      !args_dict->GetString("routeId", &route_id) ||
      !args_dict->GetBoolean("isLocal", &is_local)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  media_router_ui_->TerminateRoute(route_id);
  UMA_HISTOGRAM_BOOLEAN("MediaRouter.Ui.Action.StopRoute", !is_local);
}

void MediaRouterWebUIMessageHandler::OnCloseDialog(
    const base::ListValue* args) {
  DVLOG(1) << "OnCloseDialog";
  if (dialog_closing_)
    return;

  bool used_esc_to_close_dialog = false;
  if (!args->GetBoolean(0, &used_esc_to_close_dialog)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }

  if (used_esc_to_close_dialog) {
    base::RecordAction(
        base::UserMetricsAction("MediaRouter_Ui_Dialog_ESCToClose"));
  }

  dialog_closing_ = true;
  media_router_ui_->Close();
}

void MediaRouterWebUIMessageHandler::OnReportBlur(const base::ListValue* args) {
  DVLOG(1) << "OnReportBlur";
  base::RecordAction(base::UserMetricsAction("MediaRouter_Ui_Dialog_Blur"));
}

void MediaRouterWebUIMessageHandler::OnReportClickedSinkIndex(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportClickedSinkIndex";
  int index;
  if (!args->GetInteger(0, &index)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  base::UmaHistogramSparse("MediaRouter.Ui.Action.StartLocalPosition",
                           std::min(index, 100));
}

void MediaRouterWebUIMessageHandler::OnReportFilter(const base::ListValue*) {
  DVLOG(1) << "OnReportFilter";
  base::RecordAction(base::UserMetricsAction("MediaRouter_Ui_Action_Filter"));
}

void MediaRouterWebUIMessageHandler::OnReportInitialAction(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportInitialAction";
  int action;
  if (!args->GetInteger(0, &action)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  media_router::MediaRouterMetrics::RecordMediaRouterInitialUserAction(
      static_cast<MediaRouterUserAction>(action));
}

void MediaRouterWebUIMessageHandler::OnReportInitialState(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportInitialState";
  std::string initial_view;
  if (!args->GetString(0, &initial_view)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  bool sink_list_state = initial_view == "sink-list";
  DCHECK(sink_list_state || (initial_view == "route-details"));
  UMA_HISTOGRAM_BOOLEAN("MediaRouter.Ui.InitialState", sink_list_state);
}

void MediaRouterWebUIMessageHandler::OnReportNavigateToView(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportNavigateToView";
  std::string view;
  if (!args->GetString(0, &view)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }

  if (view == "cast-mode-list") {
    base::RecordAction(
        base::UserMetricsAction("MediaRouter_Ui_Navigate_SinkListToSource"));
  } else if (view == "route-details") {
    base::RecordAction(base::UserMetricsAction(
        "MediaRouter_Ui_Navigate_SinkListToRouteDetails"));
  } else if (view == "sink-list") {
    base::RecordAction(base::UserMetricsAction(
        "MediaRouter_Ui_Navigate_RouteDetailsToSinkList"));
  }
}

void MediaRouterWebUIMessageHandler::OnReportRouteCreation(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportRouteCreation";
  bool route_created_successfully;
  if (!args->GetBoolean(0, &route_created_successfully)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }

  UMA_HISTOGRAM_BOOLEAN("MediaRouter.Ui.Action.StartLocalSessionSuccessful",
                        route_created_successfully);
}

void MediaRouterWebUIMessageHandler::OnReportRouteCreationOutcome(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportRouteCreationOutcome";
  int outcome;
  if (!args->GetInteger(0, &outcome)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }

  media_router::MediaRouterMetrics::RecordRouteCreationOutcome(
      static_cast<MediaRouterRouteCreationOutcome>(outcome));
}

void MediaRouterWebUIMessageHandler::OnReportSelectedCastMode(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportSelectedCastMode";
  int cast_mode_type;
  if (!args->GetInteger(0, &cast_mode_type)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  DCHECK(IsValidCastModeNum(cast_mode_type));
  base::UmaHistogramSparse("MediaRouter.Ui.Navigate.SourceSelection",
                           cast_mode_type);
  media_router_ui_->RecordCastModeSelection(
      static_cast<MediaCastMode>(cast_mode_type));
}

void MediaRouterWebUIMessageHandler::OnReportSinkCount(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportSinkCount";
  int sink_count;
  if (!args->GetInteger(0, &sink_count)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  UMA_HISTOGRAM_COUNTS_100("MediaRouter.Ui.Device.Count", sink_count);
}

void MediaRouterWebUIMessageHandler::OnReportTimeToClickSink(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportTimeToClickSink";
  double time_to_click;
  if (!args->GetDouble(0, &time_to_click)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  UMA_HISTOGRAM_TIMES("MediaRouter.Ui.Action.StartLocal.Latency",
                      base::TimeDelta::FromMillisecondsD(time_to_click));
}

void MediaRouterWebUIMessageHandler::OnReportWebUIRouteControllerLoaded(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportWebUIRouteControllerLoaded";
  double load_time;
  if (!args->GetDouble(0, &load_time)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  UMA_HISTOGRAM_TIMES("MediaRouter.Ui.Dialog.LoadedWebUiRouteController",
                      base::TimeDelta::FromMillisecondsD(load_time));
}

void MediaRouterWebUIMessageHandler::OnReportTimeToInitialActionClose(
    const base::ListValue* args) {
  DVLOG(1) << "OnReportTimeToInitialActionClose";
  double time_to_close;
  if (!args->GetDouble(0, &time_to_close)) {
    DVLOG(1) << "Unable to extract args.";
    return;
  }
  UMA_HISTOGRAM_TIMES("MediaRouter.Ui.Action.CloseLatency",
                      base::TimeDelta::FromMillisecondsD(time_to_close));
}

void MediaRouterWebUIMessageHandler::OnSearchSinksAndCreateRoute(
    const base::ListValue* args) {
  DVLOG(1) << "OnSearchSinksAndCreateRoute";
  const base::DictionaryValue* args_dict = nullptr;
  std::string sink_id;
  std::string search_criteria;
  std::string domain;
  int cast_mode_num = -1;
  if (!args->GetDictionary(0, &args_dict) ||
      !args_dict->GetString("sinkId", &sink_id) ||
      !args_dict->GetString("searchCriteria", &search_criteria) ||
      !args_dict->GetString("domain", &domain) ||
      !args_dict->GetInteger("selectedCastMode", &cast_mode_num)) {
    DVLOG(1) << "Unable to extract args";
    return;
  }

  if (search_criteria.empty()) {
    DVLOG(1) << "Media Router UI did not provide valid search criteria. "
                "Aborting.";
    return;
  }

  if (!IsValidCastModeNum(cast_mode_num)) {
    DVLOG(1) << "Invalid cast mode: " << cast_mode_num << ". Aborting.";
    return;
  }

  media_router_ui_->SearchSinksAndCreateRoute(
      sink_id, search_criteria, domain,
      static_cast<MediaCastMode>(cast_mode_num));
}

void MediaRouterWebUIMessageHandler::OnSelectLocalMediaFile(
    const base::ListValue* args) {
  media_router_ui_->OpenFileDialog();
}

void MediaRouterWebUIMessageHandler::OnInitialDataReceived(
    const base::ListValue* args) {
  DVLOG(1) << "OnInitialDataReceived";
  media_router_ui_->OnUIInitialDataReceived();
  MaybeUpdateFirstRunFlowData();
}

void MediaRouterWebUIMessageHandler::OnMediaControllerAvailable(
    const base::ListValue* args) {
  const base::DictionaryValue* args_dict = nullptr;
  std::string route_id;
  if (!args->GetDictionary(0, &args_dict) ||
      !args_dict->GetString("routeId", &route_id)) {
    DVLOG(1) << "Unable to extract media route ID";
    return;
  }
  media_router_ui_->OnMediaControllerUIAvailable(route_id);
}

void MediaRouterWebUIMessageHandler::OnMediaControllerClosed(
    const base::ListValue* args) {
  current_media_status_.reset();
  media_router_ui_->OnMediaControllerUIClosed();
}

void MediaRouterWebUIMessageHandler::OnPauseCurrentMedia(
    const base::ListValue* args) {
  MediaRouteController* route_controller =
      media_router_ui_->GetMediaRouteController();
  if (route_controller)
    route_controller->Pause();
}

void MediaRouterWebUIMessageHandler::OnPlayCurrentMedia(
    const base::ListValue* args) {
  MediaRouteController* route_controller =
      media_router_ui_->GetMediaRouteController();
  if (route_controller)
    route_controller->Play();
}

void MediaRouterWebUIMessageHandler::OnSeekCurrentMedia(
    const base::ListValue* args) {
  const base::DictionaryValue* args_dict = nullptr;
  int time;
  if (!args->GetDictionary(0, &args_dict) ||
      !args_dict->GetInteger("time", &time)) {
    DVLOG(1) << "Unable to extract time";
    return;
  }
  base::TimeDelta time_delta = base::TimeDelta::FromSeconds(time);
  MediaRouteController* route_controller =
      media_router_ui_->GetMediaRouteController();
  if (route_controller && current_media_status_ &&
      time_delta >= base::TimeDelta() &&
      time_delta <= current_media_status_->duration) {
    route_controller->Seek(time_delta);
  }
}

void MediaRouterWebUIMessageHandler::OnSetCurrentMediaMute(
    const base::ListValue* args) {
  const base::DictionaryValue* args_dict = nullptr;
  bool mute;
  if (!args->GetDictionary(0, &args_dict) ||
      !args_dict->GetBoolean("mute", &mute)) {
    DVLOG(1) << "Unable to extract mute";
    return;
  }
  MediaRouteController* route_controller =
      media_router_ui_->GetMediaRouteController();
  if (route_controller)
    route_controller->SetMute(mute);
}

void MediaRouterWebUIMessageHandler::OnSetCurrentMediaVolume(
    const base::ListValue* args) {
  const base::DictionaryValue* args_dict = nullptr;
  double volume;
  if (!args->GetDictionary(0, &args_dict) ||
      !args_dict->GetDouble("volume", &volume)) {
    DVLOG(1) << "Unable to extract volume";
    return;
  }
  MediaRouteController* route_controller =
      media_router_ui_->GetMediaRouteController();
  if (route_controller && volume >= 0 && volume <= 1)
    route_controller->SetVolume(volume);
}

void MediaRouterWebUIMessageHandler::OnSetMediaRemotingEnabled(
    const base::ListValue* args) {
  bool media_remoting_enabled;
  if (!args->GetBoolean(0, &media_remoting_enabled)) {
    DVLOG(1) << "Unable to extract media remoting value";
    return;
  }
  MirroringMediaRouteController* mirroring_controller =
      MirroringMediaRouteController::From(
          media_router_ui_->GetMediaRouteController());
  if (!mirroring_controller) {
    DVLOG(1) << "Unable to get mirroring controller";
    return;
  }

  mirroring_controller->SetMediaRemotingEnabled(media_remoting_enabled);
}

void MediaRouterWebUIMessageHandler::OnSetHangoutsLocalPresent(
    const base::ListValue* args) {
  bool local_present;
  if (!args->GetBoolean(0, &local_present)) {
    DVLOG(1) << "Unable to extract local present";
    return;
  }
  HangoutsMediaRouteController* hangouts_controller =
      HangoutsMediaRouteController::From(
          media_router_ui_->GetMediaRouteController());
  if (!hangouts_controller) {
    DVLOG(1) << "Unable to get hangouts controller";
    return;
  }

  hangouts_controller->SetLocalPresent(local_present);
}

bool MediaRouterWebUIMessageHandler::ActOnIssueType(
    IssueInfo::Action action_type,
    const base::DictionaryValue* args) {
  if (action_type == IssueInfo::Action::LEARN_MORE) {
    std::string learn_more_url = GetLearnMoreUrl(args);
    if (learn_more_url.empty())
      return false;
    auto open_args = std::make_unique<base::ListValue>();
    open_args->AppendString(learn_more_url);
    web_ui()->CallJavascriptFunctionUnsafe(kWindowOpen, *open_args);
    return true;
  } else {
    // Do nothing; no other issue action types require any other action.
    return true;
  }
}

void MediaRouterWebUIMessageHandler::MaybeUpdateFirstRunFlowData() {
  base::DictionaryValue first_run_flow_data;

  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* pref_service = profile->GetPrefs();

  bool first_run_flow_acknowledged =
      pref_service->GetBoolean(prefs::kMediaRouterFirstRunFlowAcknowledged);
  bool show_cloud_pref = false;
  // Cloud services preference is shown if user is logged in. If the user
  // enables sync after acknowledging the first run flow, this is treated as
  // the user opting into Google services, including cloud services, if the
  // browser is a Chrome branded build.
  if (!pref_service->GetBoolean(prefs::kMediaRouterCloudServicesPrefSet)) {
    SigninManagerBase* signin_manager =
        SigninManagerFactory::GetForProfile(profile);
    if (signin_manager && signin_manager->IsAuthenticated()) {
      // If the user had previously acknowledged the first run flow without
      // being shown the cloud services option, and is now logged in with sync
      // enabled, turn on cloud services.
      if (first_run_flow_acknowledged &&
          ProfileSyncServiceFactory::GetForProfile(profile)->IsSyncActive()) {
        pref_service->SetBoolean(prefs::kMediaRouterEnableCloudServices, true);
        pref_service->SetBoolean(prefs::kMediaRouterCloudServicesPrefSet, true);
        // Return early since the first run flow won't be surfaced.
        return;
      }

      show_cloud_pref = true;
      // "Casting to a Hangout from Chrome" Chromecast help center page.
      first_run_flow_data.SetString(
          "firstRunFlowCloudPrefLearnMoreUrl",
          base::StringPrintf(kHelpPageUrlPrefix, 6320939));
    }
  }

  // Return early if the first run flow won't be surfaced.
  if (first_run_flow_acknowledged && !show_cloud_pref)
    return;

  // General Chromecast learn more page.
  first_run_flow_data.SetString("firstRunFlowLearnMoreUrl",
                                kCastLearnMorePageUrl);
  first_run_flow_data.SetBoolean("wasFirstRunFlowAcknowledged",
                                 first_run_flow_acknowledged);
  first_run_flow_data.SetBoolean("showFirstRunFlowCloudPref", show_cloud_pref);
  web_ui()->CallJavascriptFunctionUnsafe(kSetFirstRunFlowData,
                                         first_run_flow_data);
}

AccountInfo MediaRouterWebUIMessageHandler::GetAccountInfo() {
  SigninManagerBase* signin_manager =
      SigninManagerFactory::GetForProfile(Profile::FromWebUI(web_ui()));
  return signin_manager ? signin_manager->GetAuthenticatedAccountInfo()
                        : AccountInfo();
}

int MediaRouterWebUIMessageHandler::CurrentCastModeForRouteId(
    const MediaRoute::Id& route_id,
    const std::unordered_map<MediaRoute::Id, MediaCastMode>& current_cast_modes)
    const {
  auto current_cast_mode_entry = current_cast_modes.find(route_id);
  int current_cast_mode = current_cast_mode_entry != current_cast_modes.end()
                              ? current_cast_mode_entry->second
                              : -1;
  return current_cast_mode;
}

std::unique_ptr<base::ListValue> MediaRouterWebUIMessageHandler::RoutesToValue(
    const std::vector<MediaRoute>& routes,
    const std::vector<MediaRoute::Id>& joinable_route_ids,
    const std::unordered_map<MediaRoute::Id, MediaCastMode>& current_cast_modes)
    const {
  auto value = std::make_unique<base::ListValue>();

  for (const MediaRoute& route : routes) {
    bool can_join =
        base::ContainsValue(joinable_route_ids, route.media_route_id());
    int current_cast_mode =
        CurrentCastModeForRouteId(route.media_route_id(), current_cast_modes);
    std::unique_ptr<base::DictionaryValue> route_val(
        RouteToValue(route, can_join, incognito_, current_cast_mode));
    value->Append(std::move(route_val));
  }

  return value;
}

void MediaRouterWebUIMessageHandler::SetWebUIForTest(content::WebUI* web_ui) {
  set_web_ui(web_ui);
}

}  // namespace media_router
