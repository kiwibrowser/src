// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/tab_helper.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/activity_log/activity_log.h"
#include "chrome/browser/extensions/api/bookmark_manager_private/bookmark_manager_private_api.h"
#include "chrome/browser/extensions/api/declarative_content/chrome_content_rules_registry.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"
#include "chrome/browser/extensions/bookmark_app_helper.h"
#include "chrome/browser/extensions/chrome_extension_web_contents_observer.h"
#include "chrome/browser/extensions/extension_action_runner.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/install_observer.h"
#include "chrome/browser/extensions/install_tracker.h"
#include "chrome/browser/extensions/install_tracker_factory.h"
#include "chrome/browser/extensions/webstore_inline_installer.h"
#include "chrome/browser/extensions/webstore_inline_installer_factory.h"
#include "chrome/browser/installable/installable_metrics.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/extensions/api/webstore/webstore_api_constants.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/frame_navigate_params.h"
#include "extensions/browser/api/declarative/rules_registry_service.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_web_contents_observer.h"
#include "extensions/browser/image_loader.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_icon_set.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/extension_resource.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/url_constants.h"

#if defined(OS_WIN)
#include "chrome/browser/web_applications/web_app_win.h"
#endif

using content::NavigationController;
using content::NavigationEntry;
using content::WebContents;

DEFINE_WEB_CONTENTS_USER_DATA_KEY(extensions::TabHelper);

namespace extensions {

// A helper class to watch the progress of inline installation and update the
// renderer. Owned by the TabHelper.
class TabHelper::InlineInstallObserver : public InstallObserver {
 public:
  InlineInstallObserver(TabHelper* tab_helper,
                        content::BrowserContext* browser_context,
                        const ExtensionId& extension_id,
                        bool observe_download_progress,
                        bool observe_install_stage)
      : tab_helper_(tab_helper),
        extension_id_(extension_id),
        observe_download_progress_(observe_download_progress),
        observe_install_stage_(observe_install_stage),
        install_observer_(this) {
    DCHECK(tab_helper);
    DCHECK(observe_download_progress || observe_install_stage);
    InstallTracker* install_tracker =
        InstallTrackerFactory::GetForBrowserContext(browser_context);
    if (install_tracker)
      install_observer_.Add(install_tracker);
  }
  ~InlineInstallObserver() override {}

 private:
  // InstallObserver:
  void OnBeginExtensionDownload(const ExtensionId& extension_id) override {
    SendInstallStageChangedMessage(extension_id,
                                   api::webstore::INSTALL_STAGE_DOWNLOADING);
  }
  void OnDownloadProgress(const ExtensionId& extension_id,
                          int percent_downloaded) override {
    if (observe_download_progress_ && extension_id == extension_id_) {
      auto iter =
          tab_helper_->inline_install_progress_listeners_.find(extension_id);
      DCHECK(iter != tab_helper_->inline_install_progress_listeners_.end());
      iter->second->InlineInstallDownloadProgress(percent_downloaded);
    }
  }
  void OnBeginCrxInstall(const ExtensionId& extension_id) override {
    SendInstallStageChangedMessage(extension_id,
                                   api::webstore::INSTALL_STAGE_INSTALLING);
  }
  void OnShutdown() override { install_observer_.RemoveAll(); }

  void SendInstallStageChangedMessage(const ExtensionId& extension_id,
                                      api::webstore::InstallStage stage) {
    if (observe_install_stage_ && extension_id == extension_id_) {
      auto iter =
          tab_helper_->inline_install_progress_listeners_.find(extension_id);
      DCHECK(iter != tab_helper_->inline_install_progress_listeners_.end());
      iter->second->InlineInstallStageChanged(stage);
    }
  }

  // The owning TabHelper (guaranteed to be valid).
  TabHelper* const tab_helper_;

  // The id of the extension to observe.
  ExtensionId extension_id_;

  // Whether or not to observe download/install progress.
  const bool observe_download_progress_;
  const bool observe_install_stage_;

  ScopedObserver<InstallTracker, InstallObserver> install_observer_;

  DISALLOW_COPY_AND_ASSIGN(InlineInstallObserver);
};

TabHelper::~TabHelper() {
  RemoveScriptExecutionObserver(ActivityLog::GetInstance(profile_));
}

TabHelper::TabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      profile_(Profile::FromBrowserContext(web_contents->GetBrowserContext())),
      extension_app_(NULL),
      pending_web_app_action_(NONE),
      last_committed_nav_entry_unique_id_(0),
      script_executor_(
          new ScriptExecutor(web_contents, &script_execution_observers_)),
      extension_action_runner_(new ExtensionActionRunner(web_contents)),
      webstore_inline_installer_factory_(new WebstoreInlineInstallerFactory()),
      registry_observer_(this),
      bindings_(web_contents, this),
      image_loader_ptr_factory_(this),
      weak_ptr_factory_(this) {
  // The ActiveTabPermissionManager requires a session ID; ensure this
  // WebContents has one.
  SessionTabHelper::CreateForWebContents(web_contents);
  // The Unretained() is safe because ForEachFrame() is synchronous.
  web_contents->ForEachFrame(
      base::BindRepeating(&TabHelper::SetTabId, base::Unretained(this)));
  active_tab_permission_granter_.reset(new ActiveTabPermissionGranter(
      web_contents, SessionTabHelper::IdForTab(web_contents).id(), profile_));

  AddScriptExecutionObserver(ActivityLog::GetInstance(profile_));

  InvokeForContentRulesRegistries([this](ContentRulesRegistry* registry) {
    registry->MonitorWebContentsForRuleEvaluation(this->web_contents());
  });

  // We need an ExtensionWebContentsObserver, so make sure one exists (this is
  // a no-op if one already does).
  ChromeExtensionWebContentsObserver::CreateForWebContents(web_contents);
  ExtensionWebContentsObserver::GetForWebContents(web_contents)->dispatcher()->
      set_delegate(this);

  BookmarkManagerPrivateDragEventRouter::CreateForWebContents(web_contents);
}

void TabHelper::CreateHostedAppFromWebContents() {
  DCHECK(CanCreateBookmarkApp());
  if (pending_web_app_action_ != NONE)
    return;

  // Start fetching web app info for CreateApplicationShortcut dialog and show
  // the dialog when the data is available in OnDidGetWebApplicationInfo.
  GetApplicationInfo(CREATE_HOSTED_APP);
}

bool TabHelper::CanCreateBookmarkApp() const {
  return !profile_->IsGuestSession() && !profile_->IsOffTheRecord() &&
         !profile_->IsSystemProfile() &&
         IsValidBookmarkAppUrl(web_contents()->GetURL());
}

void TabHelper::AddScriptExecutionObserver(ScriptExecutionObserver* observer) {
  script_execution_observers_.AddObserver(observer);
}

void TabHelper::RemoveScriptExecutionObserver(
    ScriptExecutionObserver* observer) {
  script_execution_observers_.RemoveObserver(observer);
}

void TabHelper::SetExtensionApp(const Extension* extension) {
  DCHECK(!extension || AppLaunchInfo::GetFullLaunchURL(extension).is_valid());
  if (extension_app_ == extension)
    return;

  extension_app_ = extension;

  if (extension_app_) {
    registry_observer_.Add(
        ExtensionRegistry::Get(web_contents()->GetBrowserContext()));
  } else {
    registry_observer_.RemoveAll();
  }

  UpdateExtensionAppIcon(extension_app_);

  if (extension_app_) {
    SessionTabHelper::FromWebContents(web_contents())
        ->SetTabExtensionAppID(extension_app_->id());
  }
}

void TabHelper::SetExtensionAppById(const ExtensionId& extension_app_id) {
  const Extension* extension = GetExtension(extension_app_id);
  if (extension)
    SetExtensionApp(extension);
}

SkBitmap* TabHelper::GetExtensionAppIcon() {
  if (extension_app_icon_.empty())
    return NULL;

  return &extension_app_icon_;
}

// Encapsulates the logic to decide which ContentRulesRegistries need to be
// invoked, depending on whether this WebContents is associated with an Original
// or OffTheRecord profile. In the latter case, we need to invoke on both the
// Original and OffTheRecord ContentRulesRegistries since the Original registry
// handles spanning-mode incognito extensions.
template <class Func>
void TabHelper::InvokeForContentRulesRegistries(const Func& func) {
  RulesRegistryService* rules_registry_service =
      RulesRegistryService::Get(profile_);
  if (rules_registry_service) {
    func(rules_registry_service->content_rules_registry());
    if (profile_->IsOffTheRecord()) {
      // The original profile's content rules registry handles rules for
      // spanning extensions in incognito profiles, so invoke it also.
      RulesRegistryService* original_profile_rules_registry_service =
          RulesRegistryService::Get(profile_->GetOriginalProfile());
      DCHECK_NE(rules_registry_service,
                original_profile_rules_registry_service);
      if (original_profile_rules_registry_service)
        func(original_profile_rules_registry_service->content_rules_registry());
    }
  }
}

void TabHelper::FinishCreateBookmarkApp(
    const Extension* extension,
    const WebApplicationInfo& web_app_info) {
  pending_web_app_action_ = NONE;
}

void TabHelper::RenderFrameCreated(content::RenderFrameHost* host) {
  SetTabId(host);
}

void TabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() || !navigation_handle->HasCommitted())
    return;

  InvokeForContentRulesRegistries(
      [this, navigation_handle](ContentRulesRegistry* registry) {
    registry->DidFinishNavigation(web_contents(), navigation_handle);
  });

  content::BrowserContext* context = web_contents()->GetBrowserContext();
  ExtensionRegistry* registry = ExtensionRegistry::Get(context);
  const ExtensionSet& enabled_extensions = registry->enabled_extensions();

  if (util::IsNewBookmarkAppsEnabled()) {
    Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
    if (browser && browser->is_app()) {
      const Extension* extension = registry->GetExtensionById(
          web_app::GetExtensionIdFromApplicationName(browser->app_name()),
          ExtensionRegistry::EVERYTHING);
      if (extension && AppLaunchInfo::GetFullLaunchURL(extension).is_valid())
        SetExtensionApp(extension);
    } else {
      UpdateExtensionAppIcon(
          enabled_extensions.GetExtensionOrAppByURL(
              navigation_handle->GetURL()));
    }
  } else {
    UpdateExtensionAppIcon(
        enabled_extensions.GetExtensionOrAppByURL(navigation_handle->GetURL()));
  }

  if (!navigation_handle->IsSameDocument())
    ExtensionActionAPI::Get(context)->ClearAllValuesForTab(web_contents());
}

bool TabHelper::OnMessageReceived(const IPC::Message& message,
                                  content::RenderFrameHost* sender) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(TabHelper, message, sender)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_GetAppInstallState,
                        OnGetAppInstallState)
    IPC_MESSAGE_HANDLER(ExtensionHostMsg_ContentScriptsExecuting,
                        OnContentScriptsExecuting)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void TabHelper::DidCloneToNewWebContents(WebContents* old_web_contents,
                                         WebContents* new_web_contents) {
  // When the WebContents that this is attached to is cloned, give the new clone
  // a TabHelper and copy state over.
  CreateForWebContents(new_web_contents);
  TabHelper* new_helper = FromWebContents(new_web_contents);

  new_helper->SetExtensionApp(extension_app());
  new_helper->extension_app_icon_ = extension_app_icon_;
}

void TabHelper::WebContentsDestroyed() {
  ExtensionActionAPI::Get(profile_)->ClearAllValuesForTab(web_contents());
}

void TabHelper::OnDidGetWebApplicationInfo(
    chrome::mojom::ChromeRenderFrameAssociatedPtr chrome_render_frame,
    const WebApplicationInfo& info) {
  web_app_info_ = info;

  content::WebContents* contents = web_contents();
  NavigationEntry* entry = contents->GetController().GetLastCommittedEntry();
  if (!entry || last_committed_nav_entry_unique_id_ != entry->GetUniqueID())
    return;
  last_committed_nav_entry_unique_id_ = 0;

  switch (pending_web_app_action_) {
    case CREATE_HOSTED_APP: {
      if (web_app_info_.app_url.is_empty())
        web_app_info_.app_url = contents->GetLastCommittedURL();

      if (web_app_info_.title.empty())
        web_app_info_.title = contents->GetTitle();
      if (web_app_info_.title.empty())
        web_app_info_.title = base::UTF8ToUTF16(web_app_info_.app_url.spec());

      bookmark_app_helper_.reset(
          new BookmarkAppHelper(profile_, web_app_info_, contents,
                                InstallableMetrics::GetInstallSource(
                                    contents, InstallTrigger::MENU)));
      bookmark_app_helper_->Create(base::Bind(
          &TabHelper::FinishCreateBookmarkApp, weak_ptr_factory_.GetWeakPtr()));
      break;
    }
    default:
      NOTREACHED();
      break;
  }

  // The hosted app action will be cleared once the installation completes or
  // fails.
  if (pending_web_app_action_ != CREATE_HOSTED_APP)
    pending_web_app_action_ = NONE;
}

void TabHelper::DoInlineInstall(
    const std::string& webstore_item_id,
    int listeners_mask,
    mojom::InlineInstallProgressListenerPtr install_progress_listener,
    DoInlineInstallCallback callback) {
  content::RenderFrameHost* host = web_contents()->GetMainFrame();
  if (bindings_.GetCurrentTargetFrame() != host) {
    NOTREACHED();
    return;
  }

  GURL requestor_url(host->GetLastCommittedURL());
  // Check that the listener is reasonable. We should never get anything other
  // than an install stage listener, a download listener, or both.
  // The requestor_url should also be valid, and the renderer should disallow
  // child frames from sending the IPC.
  if ((listeners_mask & ~(api::webstore::INSTALL_STAGE_LISTENER |
                          api::webstore::DOWNLOAD_PROGRESS_LISTENER)) != 0 ||
      !requestor_url.is_valid() || requestor_url == url::kAboutBlankURL) {
    NOTREACHED();
    return;
  }

  if (base::ContainsKey(install_callbacks_, webstore_item_id)) {
    std::move(callback).Run(false, webstore_install::kInstallInProgressError,
                            webstore_install::INSTALL_IN_PROGRESS);
    return;
  }

  install_callbacks_[webstore_item_id] = std::move(callback);
  inline_install_progress_listeners_[webstore_item_id] =
      std::move(install_progress_listener);
  // Inform the Webstore API that an inline install is happening, in case the
  // page requested status updates.
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile_);
  if (registry->disabled_extensions().Contains(webstore_item_id) &&
      (ExtensionPrefs::Get(profile_)->GetDisableReasons(webstore_item_id) &
       disable_reason::DISABLE_PERMISSIONS_INCREASE) != 0) {
    // The extension was disabled due to permissions increase. Prompt for
    // re-enable.
    // TODO(devlin): We should also prompt for re-enable for other reasons,
    // like user-disabled.
    // For clarity, explicitly end any prior reenable process.
    extension_reenabler_.reset();
    extension_reenabler_ = ExtensionReenabler::PromptForReenable(
        registry->disabled_extensions().GetByID(webstore_item_id), profile_,
        web_contents(), requestor_url,
        base::Bind(&TabHelper::OnReenableComplete,
                   weak_ptr_factory_.GetWeakPtr(), webstore_item_id));
  } else {
    // TODO(devlin): We should adddress the case of the extension already
    // being installed and enabled.
    bool observe_download_progress =
        (listeners_mask & api::webstore::DOWNLOAD_PROGRESS_LISTENER) != 0;
    bool observe_install_stage =
        (listeners_mask & api::webstore::INSTALL_STAGE_LISTENER) != 0;
    if (observe_install_stage || observe_download_progress) {
      DCHECK_EQ(0u, install_observers_.count(webstore_item_id));
      install_observers_[webstore_item_id] =
          std::make_unique<InlineInstallObserver>(
              this, web_contents()->GetBrowserContext(), webstore_item_id,
              observe_download_progress, observe_install_stage);
    }

    WebstoreStandaloneInstaller::Callback callback =
        base::Bind(&TabHelper::OnInlineInstallComplete,
                   weak_ptr_factory_.GetWeakPtr(), webstore_item_id);
    scoped_refptr<WebstoreInlineInstaller> installer(
        webstore_inline_installer_factory_->CreateInstaller(
            web_contents(), host, webstore_item_id, requestor_url, callback));
    installer->BeginInstall();
  }
}

void TabHelper::OnGetAppInstallState(content::RenderFrameHost* host,
                                     const GURL& requestor_url,
                                     int return_route_id,
                                     int callback_id) {
  ExtensionRegistry* registry =
      ExtensionRegistry::Get(web_contents()->GetBrowserContext());
  const ExtensionSet& extensions = registry->enabled_extensions();
  const ExtensionSet& disabled_extensions = registry->disabled_extensions();

  std::string state;
  if (extensions.GetHostedAppByURL(requestor_url))
    state = extension_misc::kAppStateInstalled;
  else if (disabled_extensions.GetHostedAppByURL(requestor_url))
    state = extension_misc::kAppStateDisabled;
  else
    state = extension_misc::kAppStateNotInstalled;

  // We use the |host| to send the message because using
  // WebContentsObserver::Send() defaults to using the main RenderView, which
  // might be in a different process if the request came from a frame.
  host->Send(new ExtensionMsg_GetAppInstallStateResponse(return_route_id, state,
                                                         callback_id));
}

void TabHelper::OnContentScriptsExecuting(
    content::RenderFrameHost* host,
    const ScriptExecutionObserver::ExecutingScriptsMap& executing_scripts_map,
    const GURL& on_url) {
  for (auto& observer : script_execution_observers_)
    observer.OnScriptsExecuted(web_contents(), executing_scripts_map, on_url);
}

const Extension* TabHelper::GetExtension(const ExtensionId& extension_app_id) {
  if (extension_app_id.empty())
    return NULL;

  content::BrowserContext* context = web_contents()->GetBrowserContext();
  return ExtensionRegistry::Get(context)->enabled_extensions().GetByID(
      extension_app_id);
}

void TabHelper::UpdateExtensionAppIcon(const Extension* extension) {
  extension_app_icon_.reset();
  // Ensure previously enqueued callbacks are ignored.
  image_loader_ptr_factory_.InvalidateWeakPtrs();

  // Enqueue OnImageLoaded callback.
  if (extension) {
    ImageLoader* loader = ImageLoader::Get(profile_);
    loader->LoadImageAsync(
        extension,
        IconsInfo::GetIconResource(extension,
                                   extension_misc::EXTENSION_ICON_SMALL,
                                   ExtensionIconSet::MATCH_BIGGER),
        gfx::Size(extension_misc::EXTENSION_ICON_SMALL,
                  extension_misc::EXTENSION_ICON_SMALL),
        base::Bind(&TabHelper::OnImageLoaded,
                   image_loader_ptr_factory_.GetWeakPtr()));
  }
}

void TabHelper::SetWebstoreInlineInstallerFactoryForTests(
    WebstoreInlineInstallerFactory* factory) {
  webstore_inline_installer_factory_.reset(factory);
}

void TabHelper::OnImageLoaded(const gfx::Image& image) {
  if (!image.IsEmpty()) {
    extension_app_icon_ = *image.ToSkBitmap();
    web_contents()->NotifyNavigationStateChanged(content::INVALIDATE_TYPE_TAB);
  }
}

WindowController* TabHelper::GetExtensionWindowController() const  {
  return ExtensionTabUtil::GetWindowControllerOfTab(web_contents());
}

void TabHelper::OnReenableComplete(const ExtensionId& extension_id,
                                   ExtensionReenabler::ReenableResult result) {
  // Map the re-enable results to webstore-install results.
  webstore_install::Result webstore_result = webstore_install::SUCCESS;
  std::string error;
  switch (result) {
    case ExtensionReenabler::REENABLE_SUCCESS:
      break;  // already set
    case ExtensionReenabler::USER_CANCELED:
      webstore_result = webstore_install::USER_CANCELLED;
      error = "User canceled install.";
      break;
    case ExtensionReenabler::NOT_ALLOWED:
      webstore_result = webstore_install::NOT_PERMITTED;
      error = "Install not permitted.";
      break;
    case ExtensionReenabler::ABORTED:
      webstore_result = webstore_install::ABORTED;
      error = "Aborted due to tab closing.";
      break;
  }

  OnInlineInstallComplete(extension_id,
                          result == ExtensionReenabler::REENABLE_SUCCESS, error,
                          webstore_result);
  // Note: ExtensionReenabler contained the callback with the curried-in
  // |extension_id|; delete it last.
  extension_reenabler_.reset();
}

void TabHelper::OnInlineInstallComplete(const ExtensionId& extension_id,
                                        bool success,
                                        const std::string& error,
                                        webstore_install::Result result) {
  install_observers_.erase(extension_id);
  auto iter = install_callbacks_.find(extension_id);
  DCHECK(iter != install_callbacks_.end());
  std::move(iter->second).Run(success, success ? std::string() : error, result);
  install_callbacks_.erase(iter);
}

WebContents* TabHelper::GetAssociatedWebContents() const {
  return web_contents();
}

void TabHelper::OnExtensionUnloaded(content::BrowserContext* browser_context,
                                    const Extension* extension,
                                    UnloadedExtensionReason reason) {
  DCHECK(extension_app_);
  if (extension == extension_app_)
    SetExtensionApp(nullptr);
}

void TabHelper::GetApplicationInfo(WebAppAction action) {
  NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (!entry)
    return;

  // This DCHECK added to ensure this function is called only once.
  DCHECK_EQ(pending_web_app_action_, NONE);

  pending_web_app_action_ = action;
  last_committed_nav_entry_unique_id_ = entry->GetUniqueID();

  chrome::mojom::ChromeRenderFrameAssociatedPtr chrome_render_frame;
  web_contents()->GetMainFrame()->GetRemoteAssociatedInterfaces()->GetInterface(
      &chrome_render_frame);
  // Bind the InterfacePtr into the callback so that it's kept alive
  // until there's either a connection error or a response.
  auto* web_app_info_proxy = chrome_render_frame.get();
  web_app_info_proxy->GetWebApplicationInfo(
      base::Bind(&TabHelper::OnDidGetWebApplicationInfo, base::Unretained(this),
                 base::Passed(&chrome_render_frame)));
}

void TabHelper::SetTabId(content::RenderFrameHost* render_frame_host) {
  render_frame_host->Send(new ExtensionMsg_SetTabId(
      render_frame_host->GetRoutingID(),
      SessionTabHelper::IdForTab(web_contents()).id()));
}

}  // namespace extensions
