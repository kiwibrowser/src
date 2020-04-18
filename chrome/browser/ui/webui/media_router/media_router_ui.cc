// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/media_router/media_router_ui.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/guid.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "chrome/browser/media/router/issue_manager.h"
#include "chrome/browser/media/router/issues_observer.h"
#include "chrome/browser/media/router/media_router.h"
#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/browser/media/router/media_router_metrics.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/media_router/media_router_ui_helper.h"
#include "chrome/browser/ui/webui/media_router/media_router_localized_strings_provider.h"
#include "chrome/browser/ui/webui/media_router/media_router_resources_provider.h"
#include "chrome/browser/ui/webui/media_router/media_router_webui_message_handler.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/media_router/issue.h"
#include "chrome/common/media_router/media_route.h"
#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/fullscreen_video_element.mojom.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/web_dialogs/web_dialog_delegate.h"
#include "url/origin.h"

#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
#include "chrome/browser/media/router/providers/wired_display/wired_display_media_route_provider.h"
#include "ui/display/display.h"
#endif

namespace media_router {

// This class calls to refresh the UI when the highest priority issue is
// updated.
class MediaRouterUI::UIIssuesObserver : public IssuesObserver {
 public:
  UIIssuesObserver(IssueManager* issue_manager, MediaRouterUI* ui)
      : IssuesObserver(issue_manager), ui_(ui) {
    DCHECK(ui);
  }

  ~UIIssuesObserver() override {}

  // IssuesObserver implementation.
  void OnIssue(const Issue& issue) override { ui_->SetIssue(issue); }
  void OnIssuesCleared() override { ui_->ClearIssue(); }

 private:
  // Reference back to the owning MediaRouterUI instance.
  MediaRouterUI* ui_;

  DISALLOW_COPY_AND_ASSIGN(UIIssuesObserver);
};

// Observes a WebContents and requests fullscreening of its first
// video element.  The request is sent after the WebContents is loaded and tab
// capture has begun. Marked final to prevent inheritance so delete calls are
// contained to scenarios documented below.
class MediaRouterUI::WebContentsFullscreenOnLoadedObserver final
    : public content::WebContentsObserver {
 public:
  WebContentsFullscreenOnLoadedObserver(const GURL& file_url,
                                        content::WebContents* web_contents)
      : file_url_(file_url), capture_poll_timer_(false, false) {
    DCHECK(file_url_.SchemeIsFile());
    DCHECK(fullscreen_request_time_.is_null());

    // If the WebContents is loading, start listening, otherwise just call the
    // fullscreen function.

    // This class destroys itself in the following situations (at least one of
    // which will occur):
    //   * after loading is complete and,
    //   ** capture has begun and fullscreen requested,
    //   ** kMaxSecondsToWaitForCapture seconds have passed without capture,
    //   * another navigation is started,
    //   * the WebContents is destroyed.
    if (web_contents->IsLoading()) {
      Observe(web_contents);
    } else {
      FullScreenFirstVideoElement(web_contents);
    }
  }
  ~WebContentsFullscreenOnLoadedObserver() override {}

  // content::WebContentsObserver implementation.
  void DidStopLoading() override {
    FullScreenFirstVideoElement(web_contents());
  }

  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override {
    // If the user takes over and navigates away from the file, stop listening.
    // (It is possible however for this listener to be created before the
    // navigation to the requested file triggers, so provided we're still on the
    // same URL, go ahead and keep listening).
    if (file_url_ != navigation_handle->GetURL()) {
      delete this;
    }
  }

  void WebContentsDestroyed() override {
    // If the WebContents is destroyed we will never trigger and need to clean
    // up.
    delete this;
  }

 private:
  const GURL file_url_;

  // Time intervals used by the logic that detects if capture has started.
  const int kMaxSecondsToWaitForCapture = 10;
  const int kPollIntervalInSeconds = 1;

  // The time at which fullscreen was requested.
  base::TimeTicks fullscreen_request_time_;

  // Poll timer to monitor the capturer count when fullscreening local files.
  //
  // TODO(crbug.com/540965): Add a method to WebContentsObserver to report
  // capturer count changes and get rid of this polling-based approach.
  base::Timer capture_poll_timer_;

  // Sends a request for full screen to the WebContents targeted at the first
  // video element.  The request is only sent after capture has begun.
  void FullScreenFirstVideoElement(content::WebContents* web_contents) {
    if (file_url_ != web_contents->GetLastCommittedURL()) {
      // The user has navigated before the casting started. Do not attempt to
      // fullscreen and cleanup.
      return;
    }

    fullscreen_request_time_ = base::TimeTicks::Now();
    FullscreenIfContentCaptured(web_contents);
  }

  void FullscreenIfContentCaptured(content::WebContents* web_contents) {
    if (web_contents->IsBeingCaptured()) {
      content::mojom::FullscreenVideoElementHandlerAssociatedPtr client;
      web_contents->GetMainFrame()
          ->GetRemoteAssociatedInterfaces()
          ->GetInterface(&client);
      client->RequestFullscreenVideoElement();
      delete this;
      return;
    } else if (base::TimeTicks::Now() - fullscreen_request_time_ >
               base::TimeDelta::FromSeconds(kMaxSecondsToWaitForCapture)) {
      // If content capture hasn't started within the timeout skip fullscreen.
      DLOG(WARNING) << "Capture of local content did not start within timeout";
      delete this;
      return;
    }

    capture_poll_timer_.Start(
        FROM_HERE, base::TimeDelta::FromSeconds(kPollIntervalInSeconds),
        base::BindRepeating(
            &WebContentsFullscreenOnLoadedObserver::FullscreenIfContentCaptured,
            base::Unretained(this), web_contents));
  }
};

MediaRouterUI::MediaRouterUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui),
      ui_initialized_(false),
      weak_factory_(this) {
  auto handler = std::make_unique<MediaRouterWebUIMessageHandler>(this);
  handler_ = handler.get();

  // Create a WebUIDataSource containing the chrome://media-router page's
  // content.
  std::unique_ptr<content::WebUIDataSource> html_source(
      content::WebUIDataSource::Create(chrome::kChromeUIMediaRouterHost));

  AddLocalizedStrings(html_source.get());
  AddMediaRouterUIResources(html_source.get());
  // Ownership of |html_source| is transferred to the BrowserContext.
  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui),
                                html_source.release());

  web_ui->AddMessageHandler(std::move(handler));
}

MediaRouterUI::~MediaRouterUI() = default;

void MediaRouterUI::Close() {
  ConstrainedWebDialogDelegate* delegate = GetConstrainedDelegate();
  if (delegate) {
    delegate->GetWebDialogDelegate()->OnDialogClosed(std::string());
    delegate->OnDialogCloseFromWebUI();
  }
}

void MediaRouterUI::OnUIInitialized() {
  TRACE_EVENT_NESTABLE_ASYNC_END0("media_router", "UI", initiator());

  ui_initialized_ = true;

  // TODO(imcheng): We should be able to instantiate |issue_observer_| during
  // InitCommon by storing an initial Issue in this class.
  // Register for Issue updates.
  issues_observer_ =
      std::make_unique<UIIssuesObserver>(GetIssueManager(), this);
  issues_observer_->Init();
}

bool MediaRouterUI::CreateRoute(const MediaSink::Id& sink_id,
                                MediaCastMode cast_mode) {
  // Default the tab casting the content to the initiator, and change if
  // necessary.
  content::WebContents* tab_contents = initiator();

  base::Optional<RouteParameters> params;
  if (cast_mode == MediaCastMode::LOCAL_FILE) {
    GURL url = media_router_file_dialog_->GetLastSelectedFileUrl();
    tab_contents = OpenTabWithUrl(url);
    params = GetLocalFileRouteParameters(sink_id, url, tab_contents);
  } else {
    params = GetRouteParameters(sink_id, cast_mode);
  }
  if (!params) {
    SendIssueForUnableToCast(cast_mode);
    return false;
  }

  GetIssueManager()->ClearNonBlockingIssues();
  GetMediaRouter()->CreateRoute(params->source_id, sink_id, params->origin,
                                tab_contents,
                                std::move(params->route_response_callbacks),
                                params->timeout, params->incognito);
  return true;
}

bool MediaRouterUI::ConnectRoute(const MediaSink::Id& sink_id,
                                 const MediaRoute::Id& route_id) {
  base::Optional<RouteParameters> params =
      GetRouteParameters(sink_id, MediaCastMode::PRESENTATION);
  if (!params) {
    SendIssueForUnableToCast(MediaCastMode::PRESENTATION);
    return false;
  }
  GetIssueManager()->ClearNonBlockingIssues();
  GetMediaRouter()->ConnectRouteByRouteId(
      params->source_id, route_id, params->origin, initiator(),
      std::move(params->route_response_callbacks), params->timeout,
      params->incognito);
  return true;
}

void MediaRouterUI::AddIssue(const IssueInfo& issue) {
  GetIssueManager()->AddIssue(issue);
}

void MediaRouterUI::ClearIssue(const Issue::Id& issue_id) {
  GetIssueManager()->ClearIssue(issue_id);
}

void MediaRouterUI::OpenFileDialog() {
  if (!media_router_file_dialog_) {
    media_router_file_dialog_ = std::make_unique<MediaRouterFileDialog>(this);
  }

  media_router_file_dialog_->OpenFileDialog(GetBrowser());
}

void MediaRouterUI::SearchSinksAndCreateRoute(
    const MediaSink::Id& sink_id,
    const std::string& search_criteria,
    const std::string& domain,
    MediaCastMode cast_mode) {
  std::unique_ptr<MediaSource> source =
      query_result_manager()->GetSourceForCastModeAndSink(cast_mode, sink_id);
  const std::string source_id = source ? source->id() : "";

  // The CreateRoute() part of the function is accomplished in the callback
  // OnSearchSinkResponseReceived().
  GetMediaRouter()->SearchSinks(
      sink_id, source_id, search_criteria, domain,
      base::BindRepeating(&MediaRouterUI::OnSearchSinkResponseReceived,
                          weak_factory_.GetWeakPtr(), cast_mode));
}

bool MediaRouterUI::UserSelectedTabMirroringForCurrentOrigin() const {
  const base::ListValue* origins =
      Profile::FromWebUI(web_ui())->GetPrefs()->GetList(
          ::prefs::kMediaRouterTabMirroringSources);
  return origins->Find(base::Value(GetSerializedInitiatorOrigin())) !=
         origins->end();
}

void MediaRouterUI::RecordCastModeSelection(MediaCastMode cast_mode) {
  ListPrefUpdate update(Profile::FromWebUI(web_ui())->GetPrefs(),
                        ::prefs::kMediaRouterTabMirroringSources);

  switch (cast_mode) {
    case MediaCastMode::PRESENTATION:
      update->Remove(base::Value(GetSerializedInitiatorOrigin()), nullptr);
      break;
    case MediaCastMode::TAB_MIRROR:
      update->AppendIfNotPresent(
          std::make_unique<base::Value>(GetSerializedInitiatorOrigin()));
      break;
    case MediaCastMode::DESKTOP_MIRROR:
      // Desktop mirroring isn't domain-specific, so we don't record the
      // selection.
      break;
    case MediaCastMode::LOCAL_FILE:
      // Local media isn't domain-specific, so we don't record the selection.
      break;
    default:
      NOTREACHED();
      break;
  }
}

std::string MediaRouterUI::GetPresentationRequestSourceName() const {
  GURL gurl = GetFrameURL();
  return gurl.SchemeIs(extensions::kExtensionScheme)
             ? GetExtensionName(gurl, extensions::ExtensionRegistry::Get(
                                          Profile::FromWebUI(web_ui())))
             : GetHostFromURL(gurl);
}

const std::set<MediaCastMode>& MediaRouterUI::cast_modes() const {
  return cast_modes_;
}

void MediaRouterUI::SetUIInitializationTimer(const base::Time& start_time) {
  DCHECK(!start_time.is_null());
  start_time_ = start_time;
}

void MediaRouterUI::OnUIInitiallyLoaded() {
  if (!start_time_.is_null()) {
    MediaRouterMetrics::RecordMediaRouterDialogPaint(base::Time::Now() -
                                                     start_time_);
  }
}

void MediaRouterUI::OnUIInitialDataReceived() {
  if (!start_time_.is_null()) {
    MediaRouterMetrics::RecordMediaRouterDialogLoaded(base::Time::Now() -
                                                      start_time_);
    start_time_ = base::Time();
  }
}

void MediaRouterUI::UpdateMaxDialogHeight(int height) {
  if (ui_initialized_) {
    handler_->UpdateMaxDialogHeight(height);
  }
}

MediaRouteController* MediaRouterUI::GetMediaRouteController() const {
  return route_controller_observer_
             ? route_controller_observer_->controller().get()
             : nullptr;
}

void MediaRouterUI::OnMediaControllerUIAvailable(
    const MediaRoute::Id& route_id) {
  scoped_refptr<MediaRouteController> controller =
      GetMediaRouter()->GetRouteController(route_id);
  if (!controller) {
    DVLOG(1) << "Requested a route controller with an invalid route ID.";
    return;
  }
  DVLOG_IF(1, route_controller_observer_)
      << "Route controller observer unexpectedly exists.";
  route_controller_observer_ =
      std::make_unique<UIMediaRouteControllerObserver>(this, controller);
}

void MediaRouterUI::OnMediaControllerUIClosed() {
  route_controller_observer_.reset();
}

void MediaRouterUI::InitForTest(
    MediaRouter* router,
    content::WebContents* initiator,
    MediaRouterWebUIMessageHandler* handler,
    std::unique_ptr<StartPresentationContext> context,
    std::unique_ptr<MediaRouterFileDialog> file_dialog) {
  handler_ = handler;
  set_start_presentation_context_for_test(std::move(context));
  InitForTest(std::move(file_dialog));
  InitCommon(initiator);
  if (start_presentation_context()) {
    OnDefaultPresentationChanged(
        start_presentation_context()->presentation_request());
  }

  OnUIInitialized();
}

void MediaRouterUI::InitForTest(
    std::unique_ptr<MediaRouterFileDialog> file_dialog) {
  media_router_file_dialog_ = std::move(file_dialog);
}

MediaRouterUI::UIMediaRouteControllerObserver::UIMediaRouteControllerObserver(
    MediaRouterUI* ui,
    scoped_refptr<MediaRouteController> controller)
    : MediaRouteController::Observer(std::move(controller)), ui_(ui) {
  if (controller_->current_media_status())
    OnMediaStatusUpdated(controller_->current_media_status().value());
}

MediaRouterUI::UIMediaRouteControllerObserver::
    ~UIMediaRouteControllerObserver() {}

void MediaRouterUI::UIMediaRouteControllerObserver::OnMediaStatusUpdated(
    const MediaStatus& status) {
  ui_->UpdateMediaRouteStatus(status);
}

void MediaRouterUI::UIMediaRouteControllerObserver::OnControllerInvalidated() {
  ui_->OnRouteControllerInvalidated();
}

Browser* MediaRouterUI::GetBrowser() {
  CHECK(initiator());
  return chrome::FindBrowserWithWebContents(initiator());
}

content::WebContents* MediaRouterUI::OpenTabWithUrl(const GURL url) {
  // Check if the current page is a new tab. If so open file in current page.
  // If not then open a new page.
  if (initiator()->GetVisibleURL() == chrome::kChromeUINewTabURL) {
    content::NavigationController::LoadURLParams load_params(url);
    load_params.transition_type = ui::PAGE_TRANSITION_GENERATED;
    initiator()->GetController().LoadURLWithParams(load_params);
    return initiator();
  } else {
    return chrome::AddSelectedTabWithURL(GetBrowser(), url,
                                         ui::PAGE_TRANSITION_LINK);
  }
}

void MediaRouterUI::FileDialogFileSelected(
    const ui::SelectedFileInfo& file_info) {
  handler_->UserSelectedLocalMediaFile(file_info.display_name);
}

void MediaRouterUI::FileDialogSelectionFailed(const IssueInfo& issue) {
  AddIssue(issue);
}

void MediaRouterUI::SetIssue(const Issue& issue) {
  if (ui_initialized_)
    handler_->UpdateIssue(issue);
}

void MediaRouterUI::ClearIssue() {
  if (ui_initialized_)
    handler_->ClearIssue();
}

void MediaRouterUI::OnRoutesUpdated(
    const std::vector<MediaRoute>& routes,
    const std::vector<MediaRoute::Id>& joinable_route_ids) {
  MediaRouterUIBase::OnRoutesUpdated(routes, joinable_route_ids);
  joinable_route_ids_.clear();

  for (const MediaRoute& route : routes) {
    if (route.for_display() &&
        base::ContainsValue(joinable_route_ids, route.media_route_id())) {
      joinable_route_ids_.push_back(route.media_route_id());
    }
  }

  if (ui_initialized_) {
    handler_->UpdateRoutes(MediaRouterUIBase::routes(), joinable_route_ids_,
                           routes_and_cast_modes());
  }
  UpdateRoutesToCastModesMapping();
}

void MediaRouterUI::OnRouteResponseReceived(
    int route_request_id,
    const MediaSink::Id& sink_id,
    MediaCastMode cast_mode,
    const base::string16& presentation_request_source_name,
    const RouteRequestResult& result) {
  MediaRouterUIBase::OnRouteResponseReceived(
      route_request_id, sink_id, cast_mode, presentation_request_source_name,
      result);
  handler_->OnCreateRouteResponseReceived(sink_id, result.route());
  if (result.result_code() == RouteRequestResult::TIMED_OUT)
    SendIssueForRouteTimeout(cast_mode, presentation_request_source_name);
}

void MediaRouterUI::MaybeReportFileInformation(
    const RouteRequestResult& result) {
  if (result.result_code() == RouteRequestResult::OK)
    media_router_file_dialog_->MaybeReportLastSelectedFileInformation();
}

void MediaRouterUI::HandleCreateSessionRequestRouteResponse(
    const RouteRequestResult&) {
  Close();
}

void MediaRouterUI::OnSearchSinkResponseReceived(
    MediaCastMode cast_mode,
    const MediaSink::Id& found_sink_id) {
  DVLOG(1) << "OnSearchSinkResponseReceived";
  handler_->ReturnSearchResult(found_sink_id);

  CreateRoute(found_sink_id, cast_mode);
}

void MediaRouterUI::SendIssueForRouteTimeout(
    MediaCastMode cast_mode,
    const base::string16& presentation_request_source_name) {
  std::string issue_title;
  switch (cast_mode) {
    case PRESENTATION:
      DLOG_IF(ERROR, presentation_request_source_name.empty())
          << "Empty presentation request source name.";
      issue_title =
          l10n_util::GetStringFUTF8(IDS_MEDIA_ROUTER_ISSUE_CREATE_ROUTE_TIMEOUT,
                                    presentation_request_source_name);
      break;
    case TAB_MIRROR:
      issue_title = l10n_util::GetStringUTF8(
          IDS_MEDIA_ROUTER_ISSUE_CREATE_ROUTE_TIMEOUT_FOR_TAB);
      break;
    case DESKTOP_MIRROR:
      issue_title = l10n_util::GetStringUTF8(
          IDS_MEDIA_ROUTER_ISSUE_CREATE_ROUTE_TIMEOUT_FOR_DESKTOP);
      break;
    default:
      NOTREACHED();
  }

  AddIssue(IssueInfo(issue_title, IssueInfo::Action::DISMISS,
                     IssueInfo::Severity::NOTIFICATION));
}

void MediaRouterUI::SendIssueForUnableToCast(MediaCastMode cast_mode) {
  // For a generic error, claim a tab error unless it was specifically desktop
  // mirroring.
  std::string issue_title =
      (cast_mode == MediaCastMode::DESKTOP_MIRROR)
          ? l10n_util::GetStringUTF8(
                IDS_MEDIA_ROUTER_ISSUE_UNABLE_TO_CAST_DESKTOP)
          : l10n_util::GetStringUTF8(
                IDS_MEDIA_ROUTER_ISSUE_CREATE_ROUTE_TIMEOUT_FOR_TAB);
  AddIssue(IssueInfo(issue_title, IssueInfo::Action::DISMISS,
                     IssueInfo::Severity::WARNING));
}

void MediaRouterUI::InitCommon(content::WebContents* initiator) {
  MediaRouterUIBase::InitCommon(initiator);
  UpdateCastModes();
  // Presentation requests from content must show the origin requesting
  // presentation: crbug.com/704964
  if (start_presentation_context())
    forced_cast_mode_ = MediaCastMode::PRESENTATION;
}

void MediaRouterUI::OnDefaultPresentationChanged(
    const content::PresentationRequest& presentation_request) {
  MediaRouterUIBase::OnDefaultPresentationChanged(presentation_request);
  UpdateCastModes();
}

void MediaRouterUI::OnDefaultPresentationRemoved() {
  MediaRouterUIBase::OnDefaultPresentationRemoved();

  // This should not be set if the dialog was initiated with a default
  // presentation request from the top level frame.  However, clear it just to
  // be safe.
  forced_cast_mode_ = base::nullopt;
  UpdateCastModes();
}

base::Optional<RouteParameters> MediaRouterUI::GetLocalFileRouteParameters(
    const MediaSink::Id& sink_id,
    const GURL& file_url,
    content::WebContents* tab_contents) {
  RouteParameters params;
  SessionID::id_type tab_id = SessionTabHelper::IdForTab(tab_contents).id();
  params.source_id = MediaSourceForTab(tab_id).id();

  // Use a placeholder URL as origin for local file casting, which is
  // essentially mirroring.
  params.origin = url::Origin::Create(GURL(chrome::kChromeUIMediaRouterURL));

  params.route_response_callbacks.push_back(base::BindOnce(
      &MediaRouterUI::OnRouteResponseReceived, weak_factory_.GetWeakPtr(),
      current_route_request_id(), sink_id, MediaCastMode::LOCAL_FILE,
      base::UTF8ToUTF16(GetTruncatedPresentationRequestSourceName())));

  params.route_response_callbacks.push_back(
      base::BindOnce(&MediaRouterUIBase::MaybeReportCastingSource,
                     weak_factory_.GetWeakPtr(), MediaCastMode::LOCAL_FILE));

  params.route_response_callbacks.push_back(base::BindOnce(
      &MediaRouterUI::MaybeReportFileInformation, weak_factory_.GetWeakPtr()));

  params.route_response_callbacks.push_back(
      base::BindOnce(&MediaRouterUI::FullScreenFirstVideoElement,
                     weak_factory_.GetWeakPtr(), file_url, tab_contents));

  params.timeout = GetRouteRequestTimeout(MediaCastMode::LOCAL_FILE);
  CHECK(initiator());
  params.incognito = initiator()->GetBrowserContext()->IsOffTheRecord();

  return base::make_optional(std::move(params));
}

// TODO(crbug.com/792547): Refactor FullScreenFirstVideoElement() and
// MaybeReportFileInformation() into a local media casting specific location
// instead of here in the main ui.
void MediaRouterUI::FullScreenFirstVideoElement(
    const GURL& file_url,
    content::WebContents* web_contents,
    const RouteRequestResult& result) {
  if (result.result_code() == RouteRequestResult::OK) {
    new WebContentsFullscreenOnLoadedObserver(file_url, web_contents);
  }
}

void MediaRouterUI::UpdateCastModes() {
  // Gets updated cast modes from |query_result_manager()| and forwards it to
  // UI.
  cast_modes_ = query_result_manager()->GetSupportedCastModes();
  if (ui_initialized_) {
    handler_->UpdateCastModes(cast_modes(), GetPresentationRequestSourceName(),
                              forced_cast_mode());
  }
}

void MediaRouterUI::UpdateRoutesToCastModesMapping() {
  std::unordered_map<MediaSource::Id, MediaCastMode> available_source_map;
  for (const auto& cast_mode : cast_modes()) {
    for (const auto& source : GetSourcesForCastMode(cast_mode))
      available_source_map.insert(std::make_pair(source.id(), cast_mode));
  }

  routes_and_cast_modes_.clear();
  for (const auto& route : routes()) {
    auto source_entry = available_source_map.find(route.media_source().id());
    if (source_entry != available_source_map.end()) {
      routes_and_cast_modes_.insert(
          std::make_pair(route.media_route_id(), source_entry->second));
    }
  }
}

std::string MediaRouterUI::GetSerializedInitiatorOrigin() const {
  url::Origin origin =
      initiator() ? url::Origin::Create(initiator()->GetLastCommittedURL())
                  : url::Origin();
  return origin.Serialize();
}

void MediaRouterUI::OnRouteControllerInvalidated() {
  route_controller_observer_.reset();
  handler_->OnRouteControllerInvalidated();
}
void MediaRouterUI::UpdateMediaRouteStatus(const MediaStatus& status) {
  handler_->UpdateMediaRouteStatus(status);
}

IssueManager* MediaRouterUI::GetIssueManager() {
  return GetMediaRouter()->GetIssueManager();
}

void MediaRouterUI::UpdateSinks() {
  if (ui_initialized_)
    handler_->UpdateSinks(GetEnabledSinks());
}

MediaRouter* MediaRouterUI::GetMediaRouter() const {
  return MediaRouterFactory::GetApiForBrowserContext(
      web_ui()->GetWebContents()->GetBrowserContext());
}

}  // namespace media_router
