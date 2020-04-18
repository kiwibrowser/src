// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background/background_contents.h"

#include <utility>

#include "chrome/browser/background/background_contents_service.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/data_use_measurement/data_use_web_contents_observer.h"
#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_preferences_util.h"
#include "chrome/browser/task_manager/web_contents_tags.h"
#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/session_storage_namespace.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/deferred_start_render_host_observer.h"
#include "extensions/browser/extension_host_delegate.h"
#include "extensions/browser/extension_host_queue.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/view_type_utils.h"
#include "ui/gfx/geometry/rect.h"

using content::SiteInstance;
using content::WebContents;

BackgroundContents::BackgroundContents(
    scoped_refptr<SiteInstance> site_instance,
    content::RenderFrameHost* opener,
    int32_t routing_id,
    int32_t main_frame_routing_id,
    int32_t main_frame_widget_routing_id,
    Delegate* delegate,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace)
    : delegate_(delegate),
      extension_host_delegate_(extensions::ExtensionsBrowserClient::Get()
                                   ->CreateExtensionHostDelegate()) {
  profile_ = Profile::FromBrowserContext(
      site_instance->GetBrowserContext());

  WebContents::CreateParams create_params(profile_, std::move(site_instance));
  create_params.opener_render_process_id =
      opener ? opener->GetProcess()->GetID() : MSG_ROUTING_NONE;
  create_params.opener_render_frame_id =
      opener ? opener->GetRoutingID() : MSG_ROUTING_NONE;
  create_params.routing_id = routing_id;
  create_params.main_frame_routing_id = main_frame_routing_id;
  create_params.main_frame_widget_routing_id = main_frame_widget_routing_id;
  create_params.renderer_initiated_creation = routing_id != MSG_ROUTING_NONE;
  if (session_storage_namespace) {
    content::SessionStorageNamespaceMap session_storage_namespace_map;
    session_storage_namespace_map.insert(
        std::make_pair(partition_id, session_storage_namespace));
    web_contents_ = WebContents::CreateWithSessionStorage(
        create_params, session_storage_namespace_map);
  } else {
    web_contents_ = WebContents::Create(create_params);
  }
  extensions::SetViewType(
      web_contents_.get(), extensions::VIEW_TYPE_BACKGROUND_CONTENTS);
  web_contents_->SetDelegate(this);
  content::WebContentsObserver::Observe(web_contents_.get());
  data_use_measurement::DataUseWebContentsObserver::CreateForWebContents(
      web_contents_.get());
  extensions::ChromeExtensionWebContentsObserver::CreateForWebContents(
      web_contents_.get());

  // Add the TaskManager-specific tag for the BackgroundContents.
  task_manager::WebContentsTags::CreateForBackgroundContents(
      web_contents_.get(), this);

  // Close ourselves when the application is shutting down.
  registrar_.Add(this, chrome::NOTIFICATION_APP_TERMINATING,
                 content::NotificationService::AllSources());

  // Register for our parent profile to shutdown, so we can shut ourselves down
  // as well (should only be called for OTR profiles, as we should receive
  // APP_TERMINATING before non-OTR profiles are destroyed).
  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                 content::Source<Profile>(profile_));
}

// Exposed to allow creating mocks.
BackgroundContents::BackgroundContents()
    : delegate_(NULL),
      profile_(NULL) {
}

BackgroundContents::~BackgroundContents() {
  if (!web_contents_.get())   // Will be null for unit tests.
    return;

  // Unregister for any notifications before notifying observers that we are
  // going away - this prevents any re-entrancy due to chained notifications
  // (http://crbug.com/237781).
  registrar_.RemoveAll();

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_BACKGROUND_CONTENTS_DELETED,
      content::Source<Profile>(profile_),
      content::Details<BackgroundContents>(this));
  for (auto& observer : deferred_start_render_host_observer_list_)
    observer.OnDeferredStartRenderHostDestroyed(this);

  extension_host_delegate_->GetExtensionHostQueue()->Remove(this);
}

const GURL& BackgroundContents::GetURL() const {
  return web_contents_.get() ? web_contents_->GetURL() : GURL::EmptyGURL();
}

void BackgroundContents::CreateRenderViewSoon(const GURL& url) {
  initial_url_ = url;
  extension_host_delegate_->GetExtensionHostQueue()->Add(this);
}

void BackgroundContents::CloseContents(WebContents* source) {
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_BACKGROUND_CONTENTS_CLOSED,
      content::Source<Profile>(profile_),
      content::Details<BackgroundContents>(this));
  delete this;
}

bool BackgroundContents::ShouldSuppressDialogs(WebContents* source) {
  return true;
}

void BackgroundContents::DidNavigateMainFramePostCommit(WebContents* tab) {
  // Note: because BackgroundContents are only available to extension apps,
  // navigation is limited to urls within the app's extent. This is enforced in
  // RenderView::decidePolicyForNavigation. If BackgroundContents become
  // available as a part of the web platform, it probably makes sense to have
  // some way to scope navigation of a background page to its opener's security
  // origin. Note: if the first navigation is to a URL outside the app's
  // extent a background page will be opened but will remain at about:blank.
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_BACKGROUND_CONTENTS_NAVIGATED,
      content::Source<Profile>(profile_),
      content::Details<BackgroundContents>(this));
}

// Forward requests to add a new WebContents to our delegate.
void BackgroundContents::AddNewContents(
    WebContents* source,
    std::unique_ptr<WebContents> new_contents,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture,
    bool* was_blocked) {
  delegate_->AddWebContents(std::move(new_contents), disposition, initial_rect,
                            was_blocked);
}

bool BackgroundContents::IsNeverVisible(content::WebContents* web_contents) {
  DCHECK_EQ(extensions::VIEW_TYPE_BACKGROUND_CONTENTS,
            extensions::GetViewType(web_contents));
  return true;
}

void BackgroundContents::RenderProcessGone(base::TerminationStatus status) {
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_BACKGROUND_CONTENTS_TERMINATED,
      content::Source<Profile>(profile_),
      content::Details<BackgroundContents>(this));

  // Our RenderView went away, so we should go away also, so killing the process
  // via the TaskManager doesn't permanently leave a BackgroundContents hanging
  // around the system, blocking future instances from being created
  // <http://crbug.com/65189>.
  delete this;
}

void BackgroundContents::DidStartLoading() {
  // BackgroundContents only loads once, so this can only be the first time it
  // has started loading.
  for (auto& observer : deferred_start_render_host_observer_list_)
    observer.OnDeferredStartRenderHostDidStartFirstLoad(this);
}

void BackgroundContents::DidStopLoading() {
  // BackgroundContents only loads once, so this can only be the first time
  // it has stopped loading.
  for (auto& observer : deferred_start_render_host_observer_list_)
    observer.OnDeferredStartRenderHostDidStopFirstLoad(this);
}

void BackgroundContents::Observe(int type,
                                 const content::NotificationSource& source,
                                 const content::NotificationDetails& details) {
  // TODO(rafaelw): Implement pagegroup ref-counting so that non-persistent
  // background pages are closed when the last referencing frame is closed.
  switch (type) {
    case chrome::NOTIFICATION_PROFILE_DESTROYED:
    case chrome::NOTIFICATION_APP_TERMINATING: {
      delete this;
      break;
    }
    default:
      NOTREACHED() << "Unexpected notification sent.";
      break;
  }
}

void BackgroundContents::CreateRenderViewNow() {
  web_contents()->GetController().LoadURL(initial_url_, content::Referrer(),
                                          ui::PAGE_TRANSITION_LINK,
                                          std::string());
}

void BackgroundContents::AddDeferredStartRenderHostObserver(
    extensions::DeferredStartRenderHostObserver* observer) {
  deferred_start_render_host_observer_list_.AddObserver(observer);
}

void BackgroundContents::RemoveDeferredStartRenderHostObserver(
    extensions::DeferredStartRenderHostObserver* observer) {
  deferred_start_render_host_observer_list_.RemoveObserver(observer);
}
