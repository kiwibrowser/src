// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/search/answer_card/answer_card_web_contents.h"

#include <string>

#include "ash/public/cpp/app_list/answer_card_contents_registry.h"
#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "ui/app_list/views/app_list_view.h"
#include "ui/aura/window.h"
#include "ui/views/controls/native/native_view_host.h"
#include "ui/views/controls/webview/web_contents_set_background_color.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/mus/remote_view/remote_view_provider.h"
#include "ui/views/widget/widget.h"

namespace app_list {

namespace {

constexpr char kSearchAnswerHasResult[] = "SearchAnswer-HasResult";
constexpr char kSearchAnswerIssuedQuery[] = "SearchAnswer-IssuedQuery";
constexpr char kSearchAnswerTitle[] = "SearchAnswer-Title";

class SearchAnswerWebView : public views::WebView {
 public:
  explicit SearchAnswerWebView(content::BrowserContext* browser_context)
      : WebView(browser_context) {
    holder()->set_can_process_events_within_subtree(false);
  }

  void OnVisibilityEvent(bool is_removing) {
    // Need to check for |is_removing| because inside RemovedFromWidget()
    // callback, GetWidget() still returns a non-null value.
    if (!is_removing && GetWidget() && GetWidget()->IsActive() &&
        GetWidget()->IsVisible() && IsDrawn()) {
      if (shown_time_.is_null())
        shown_time_ = base::TimeTicks::Now();
    } else {
      if (!shown_time_.is_null()) {
        UMA_HISTOGRAM_MEDIUM_TIMES("SearchAnswer.AnswerVisibleTime",
                                   base::TimeTicks::Now() - shown_time_);
        shown_time_ = base::TimeTicks();
      }
    }
  }

  // views::WebView overrides:
  void AddedToWidget() override {
    WebView::AddedToWidget();

    // Find the root element that attached to the app list view.
    aura::Window* const app_list_window =
        web_contents()->GetTopLevelNativeWindow();
    aura::Window* window = web_contents()->GetNativeView();
    while (window->parent() != app_list_window)
      window = window->parent();
    AppListView::ExcludeWindowFromEventHandling(window);

    OnVisibilityEvent(false);
    // Focus Behavior is originally set in WebView::SetWebContents, but
    // overriden here because we do not want the webview to get focus.
    SetFocusBehavior(FocusBehavior::ACCESSIBLE_ONLY);
  }

  void RemovedFromWidget() override {
    OnVisibilityEvent(true);

    WebView::RemovedFromWidget();
  }

  void VisibilityChanged(View* starting_from, bool is_visible) override {
    WebView::VisibilityChanged(starting_from, is_visible);

    OnVisibilityEvent(false);
  }

  const char* GetClassName() const override { return "SearchAnswerWebView"; }

 private:
  // Time when the answer became visible to the user.
  base::TimeTicks shown_time_;

  DISALLOW_COPY_AND_ASSIGN(SearchAnswerWebView);
};

void ParseResponseHeaders(const net::HttpResponseHeaders* headers,
                          bool* has_answer_card,
                          std::string* result_title,
                          std::string* issued_query) {
  if (!headers) {
    LOG(ERROR) << "Failed to parse response headers: no headers";
    return;
  }
  if (headers->response_code() != net::HTTP_OK) {
    LOG(ERROR) << "Failed to parse response headers: response code="
               << headers->response_code();
    return;
  }
  if (!headers->HasHeaderValue(kSearchAnswerHasResult, "true")) {
    LOG(ERROR) << "Failed to parse response headers: " << kSearchAnswerHasResult
               << " header != true";
    return;
  }
  if (!headers->GetNormalizedHeader(kSearchAnswerTitle, result_title) ||
      result_title->empty()) {
    LOG(ERROR) << "Failed to parse response headers: " << kSearchAnswerTitle
               << " header is not present";
    return;
  }
  if (!headers->GetNormalizedHeader(kSearchAnswerIssuedQuery, issued_query) ||
      issued_query->empty()) {
    LOG(ERROR) << "Failed to parse response headers: "
               << kSearchAnswerIssuedQuery << " header is not present";
    return;
  }
  *has_answer_card = true;
}

}  // namespace

AnswerCardWebContents::AnswerCardWebContents(Profile* profile)
    : web_contents_(
          content::WebContents::Create(content::WebContents::CreateParams(
              profile,
              content::SiteInstance::Create(profile)))),
      profile_(profile),
      weak_ptr_factory_(this) {
  content::RendererPreferences* renderer_prefs =
      web_contents_->GetMutableRendererPrefs();
  renderer_prefs->can_accept_load_drops = false;
  // We need the OpenURLFromTab() to get called.
  renderer_prefs->browser_handles_all_top_level_requests = true;
  web_contents_->GetRenderViewHost()->SyncRendererPrefs();

  Observe(web_contents_.get());
  web_contents_->SetDelegate(this);

  // Make the webview transparent since it's going to be shown on top of a
  // highlightable button.
  views::WebContentsSetBackgroundColor::CreateForWebContentsWithColor(
      web_contents_.get(), SK_ColorTRANSPARENT);

  content::RenderViewHost* const rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    AttachToHost(rvh->GetWidget());

  if (AnswerCardContentsRegistry::Get()) {
    web_view_ = std::make_unique<SearchAnswerWebView>(profile);
    web_view_->set_owned_by_client();
    web_view_->SetWebContents(web_contents_.get());
    web_view_->SetResizeBackgroundColor(SK_ColorTRANSPARENT);

    token_ = AnswerCardContentsRegistry::Get()->Register(web_view_.get());
  }
}

AnswerCardWebContents::~AnswerCardWebContents() {
  if (AnswerCardContentsRegistry::Get() && !token_.is_empty())
    AnswerCardContentsRegistry::Get()->Unregister(token_);

  DetachFromHost();
  web_contents_->SetDelegate(nullptr);
  Observe(nullptr);
}

void AnswerCardWebContents::LoadURL(const GURL& url) {
  content::NavigationController::LoadURLParams load_params(url);
  load_params.transition_type = ui::PAGE_TRANSITION_AUTO_TOPLEVEL;
  load_params.should_clear_history_list = true;
  web_contents_->GetController().LoadURLWithParams(load_params);

  web_contents_->GetRenderWidgetHostView()->EnableAutoResize(
      gfx::Size(1, 1), gfx::Size(INT_MAX, INT_MAX));
}

const base::UnguessableToken& AnswerCardWebContents::GetToken() const {
  return token_;
}

gfx::Size AnswerCardWebContents::GetPreferredSize() const {
  return preferred_size_;
}

void AnswerCardWebContents::ResizeDueToAutoResize(
    content::WebContents* web_contents,
    const gfx::Size& new_size) {
  if (preferred_size_ == new_size)
    return;

  preferred_size_ = new_size;
  delegate()->UpdatePreferredSize(this);
}

content::WebContents* AnswerCardWebContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  if (!params.user_gesture)
    return WebContentsDelegate::OpenURLFromTab(source, params);

  // Open the user-clicked link in the browser taking into account the requested
  // disposition.
  NavigateParams new_tab_params(profile_, params.url, params.transition);

  new_tab_params.disposition = params.disposition;

  if (params.disposition == WindowOpenDisposition::NEW_BACKGROUND_TAB) {
    // When the user asks to open a link as a background tab, we show an
    // activated window with the new activated tab after the user closes the
    // launcher. So it's "background" relative to the launcher itself.
    new_tab_params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
    new_tab_params.window_action = NavigateParams::SHOW_WINDOW_INACTIVE;
  }

  Navigate(&new_tab_params);

  base::RecordAction(base::UserMetricsAction("SearchAnswer_OpenedUrl"));

  return new_tab_params.navigated_or_inserted_contents;
}

bool AnswerCardWebContents::HandleContextMenu(
    const content::ContextMenuParams& params) {
  // Disable showing the menu.
  return true;
}

void AnswerCardWebContents::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  bool has_answer_card = false;
  std::string result_title;
  std::string issued_query;

  const bool has_error = !navigation_handle->HasCommitted() ||
                         navigation_handle->IsErrorPage() ||
                         !navigation_handle->IsInMainFrame();
  if (has_error) {
    LOG(ERROR) << "Failed to navigate: HasCommitted="
               << navigation_handle->HasCommitted()
               << ", IsErrorPage=" << navigation_handle->IsErrorPage()
               << ", IsInMainFrame=" << navigation_handle->IsInMainFrame();
  } else {
    ParseResponseHeaders(navigation_handle->GetResponseHeaders(),
                         &has_answer_card, &result_title, &issued_query);
  }

  delegate()->DidFinishNavigation(this, navigation_handle->GetURL(), has_error,
                                  has_answer_card, result_title, issued_query);
}

void AnswerCardWebContents::DidStopLoading() {
  if (web_view_) {
    delegate()->OnContentsReady(this);
    return;
  }

  remote_view_provider_ = std::make_unique<views::RemoteViewProvider>(
      web_contents_->GetNativeView());
  remote_view_provider_->GetEmbedToken(
      base::BindOnce(&AnswerCardWebContents::OnGotEmbedTokenAndNotify,
                     weak_ptr_factory_.GetWeakPtr()));
}

void AnswerCardWebContents::DidGetUserInteraction(
    const blink::WebInputEvent::Type type) {
  base::RecordAction(base::UserMetricsAction("SearchAnswer_UserInteraction"));
}

void AnswerCardWebContents::RenderViewCreated(content::RenderViewHost* host) {
  if (!host_)
    AttachToHost(host->GetWidget());

  // Do not zoom for answer card web contents.
  content::HostZoomMap* zoom_map =
      content::HostZoomMap::GetForWebContents(web_contents());
  DCHECK(zoom_map);
  zoom_map->SetZoomLevelForHost(web_contents()->GetURL().host(), 0);
}

void AnswerCardWebContents::RenderViewDeleted(content::RenderViewHost* host) {
  if (host->GetWidget() == host_)
    DetachFromHost();
}

void AnswerCardWebContents::RenderViewHostChanged(
    content::RenderViewHost* old_host,
    content::RenderViewHost* new_host) {
  if ((old_host && old_host->GetWidget() == host_) || (!old_host && !host_)) {
    DetachFromHost();
    AttachToHost(new_host->GetWidget());
  }
}

void AnswerCardWebContents::AttachToHost(content::RenderWidgetHost* host) {
  host_ = host;
}

void AnswerCardWebContents::DetachFromHost() {
  if (!host_)
    return;

  host_ = nullptr;
}

void AnswerCardWebContents::OnGotEmbedTokenAndNotify(
    const base::UnguessableToken& token) {
  token_ = token;
  web_contents_->GetNativeView()->Show();
  delegate()->OnContentsReady(this);
}

}  // namespace app_list
