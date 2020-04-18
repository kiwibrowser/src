// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/platform_notification_service_impl.h"

#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/browser/notifications/metrics/notification_metrics_logger.h"
#include "chrome/browser/notifications/metrics/notification_metrics_logger_factory.h"
#include "chrome/browser/notifications/notification_common.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/permissions/permission_decision_auto_blocker.h"
#include "chrome/browser/permissions/permission_manager.h"
#include "chrome/browser/permissions/permission_result.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/safe_browsing/ping_manager.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_context.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_event_dispatcher.h"
#include "content/public/common/notification_resources.h"
#include "content/public/common/platform_notification_data.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_features.h"
#include "ui/message_center/public/cpp/features.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/notifications/notifier_state_tracker.h"
#include "chrome/browser/notifications/notifier_state_tracker_factory.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/info_map.h"
#include "extensions/common/constants.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/permissions_data.h"
#endif

using content::BrowserContext;
using content::BrowserThread;
using message_center::NotifierId;

namespace {

// Invalid id for a renderer process. Used in cases where we need to check for
// permission without having an associated renderer process yet.
const int kInvalidRenderProcessId = -1;

// Whether a web notification should be displayed when chrome is in full
// screen mode.
static bool ShouldDisplayWebNotificationOnFullScreen(Profile* profile,
                                                     const GURL& origin) {
#if defined(OS_ANDROID)
  NOTIMPLEMENTED();
  return false;
#endif  // defined(OS_ANDROID)

  // Check to see if this notification comes from a webpage that is displaying
  // fullscreen content.
  for (auto* browser : *BrowserList::GetInstance()) {
    // Only consider the browsers for the profile that created the notification
    if (browser->profile() != profile)
      continue;

    const content::WebContents* active_contents =
        browser->tab_strip_model()->GetActiveWebContents();
    if (!active_contents)
      continue;

    // Check to see if
    //  (a) the active tab in the browser shares its origin with the
    //      notification.
    //  (b) the browser is fullscreen
    //  (c) the browser has focus.
    if (active_contents->GetURL().GetOrigin() == origin &&
        browser->exclusive_access_manager()->context()->IsFullscreen() &&
        browser->window()->IsActive()) {
      return true;
    }
  }

  return false;
}

NotificationMetricsLogger* GetMetricsLogger(BrowserContext* browser_context) {
  return NotificationMetricsLoggerFactory::GetForBrowserContext(
      browser_context);
}

}  // namespace

// static
PlatformNotificationServiceImpl*
PlatformNotificationServiceImpl::GetInstance() {
  return base::Singleton<PlatformNotificationServiceImpl>::get();
}

PlatformNotificationServiceImpl::PlatformNotificationServiceImpl() {
#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  pending_click_dispatch_events_ = 0;
#endif
}

PlatformNotificationServiceImpl::~PlatformNotificationServiceImpl() {}

// TODO(miguelg): Move this to PersistentNotificationHandler
void PlatformNotificationServiceImpl::OnPersistentNotificationClick(
    BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const base::Optional<int>& action_index,
    const base::Optional<base::string16>& reply,
    base::OnceClosure completed_closure) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  blink::mojom::PermissionStatus permission_status =
      CheckPermissionOnUIThread(browser_context, origin,
                                kInvalidRenderProcessId);

  NotificationMetricsLogger* metrics_logger = GetMetricsLogger(browser_context);

  // TODO(peter): Change this to a CHECK() when Issue 555572 is resolved.
  // Also change this method to be const again.
  if (permission_status != blink::mojom::PermissionStatus::GRANTED) {
    metrics_logger->LogPersistentNotificationClickWithoutPermission();

    std::move(completed_closure).Run();
    return;
  }

  if (action_index.has_value()) {
    metrics_logger->LogPersistentNotificationActionButtonClick();
  } else {
    metrics_logger->LogPersistentNotificationClick();
  }

#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  // Ensure the browser stays alive while the event is processed.
  if (pending_click_dispatch_events_++ == 0) {
    click_dispatch_keep_alive_.reset(
        new ScopedKeepAlive(KeepAliveOrigin::PENDING_NOTIFICATION_CLICK_EVENT,
                            KeepAliveRestartOption::DISABLED));
  }
#endif

  RecordSiteEngagement(browser_context, origin);
  content::NotificationEventDispatcher::GetInstance()
      ->DispatchNotificationClickEvent(
          browser_context, notification_id, origin, action_index, reply,
          base::BindOnce(
              &PlatformNotificationServiceImpl::OnClickEventDispatchComplete,
              base::Unretained(this), std::move(completed_closure)));
}

// TODO(miguelg): Move this to PersistentNotificationHandler
void PlatformNotificationServiceImpl::OnPersistentNotificationClose(
    BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    bool by_user,
    base::OnceClosure completed_closure) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // If we programatically closed this notification, don't dispatch any event.
  if (closed_notifications_.erase(notification_id) != 0) {
    std::move(completed_closure).Run();
    return;
  }

  NotificationMetricsLogger* metrics_logger = GetMetricsLogger(browser_context);
  if (by_user)
    metrics_logger->LogPersistentNotificationClosedByUser();
  else
    metrics_logger->LogPersistentNotificationClosedProgrammatically();

  content::NotificationEventDispatcher::GetInstance()
      ->DispatchNotificationCloseEvent(
          browser_context, notification_id, origin, by_user,
          base::BindOnce(
              &PlatformNotificationServiceImpl::OnCloseEventDispatchComplete,
              base::Unretained(this), std::move(completed_closure)));
}

blink::mojom::PermissionStatus
PlatformNotificationServiceImpl::CheckPermissionOnUIThread(
    BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Profile* profile = Profile::FromBrowserContext(browser_context);
  DCHECK(profile);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Extensions support an API permission named "notification". This will grant
  // not only grant permission for using the Chrome App extension API, but also
  // for the Web Notification API.
  if (origin.SchemeIs(extensions::kExtensionScheme)) {
    extensions::ExtensionRegistry* registry =
        extensions::ExtensionRegistry::Get(browser_context);
    extensions::ProcessMap* process_map =
        extensions::ProcessMap::Get(browser_context);

    const extensions::Extension* extension =
        registry->GetExtensionById(origin.host(),
                                   extensions::ExtensionRegistry::ENABLED);

    if (extension &&
        extension->permissions_data()->HasAPIPermission(
            extensions::APIPermission::kNotifications) &&
        process_map->Contains(extension->id(), render_process_id)) {
      NotifierStateTracker* notifier_state_tracker =
          NotifierStateTrackerFactory::GetForProfile(profile);
      DCHECK(notifier_state_tracker);

      NotifierId notifier_id(NotifierId::APPLICATION, extension->id());
      if (notifier_state_tracker->IsNotifierEnabled(notifier_id))
        return blink::mojom::PermissionStatus::GRANTED;
    }
  }
#endif

  ContentSetting setting =
      PermissionManager::Get(profile)
          ->GetPermissionStatus(CONTENT_SETTINGS_TYPE_NOTIFICATIONS, origin,
                                origin)
          .content_setting;
  if (setting == CONTENT_SETTING_ALLOW)
    return blink::mojom::PermissionStatus::GRANTED;
  if (setting == CONTENT_SETTING_ASK)
    return blink::mojom::PermissionStatus::ASK;
  DCHECK_EQ(CONTENT_SETTING_BLOCK, setting);
  return blink::mojom::PermissionStatus::DENIED;
}

blink::mojom::PermissionStatus
PlatformNotificationServiceImpl::CheckPermissionOnIOThread(
    content::ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ProfileIOData* io_data = ProfileIOData::FromResourceContext(resource_context);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Extensions support an API permission named "notification". This will grant
  // not only grant permission for using the Chrome App extension API, but also
  // for the Web Notification API.
  if (origin.SchemeIs(extensions::kExtensionScheme)) {
    extensions::InfoMap* extension_info_map = io_data->GetExtensionInfoMap();
    const extensions::ProcessMap& process_map =
        extension_info_map->process_map();

    const extensions::Extension* extension =
        extension_info_map->extensions().GetByID(origin.host());

    if (extension &&
        extension->permissions_data()->HasAPIPermission(
            extensions::APIPermission::kNotifications) &&
        process_map.Contains(extension->id(), render_process_id)) {
      if (!extension_info_map->AreNotificationsDisabled(extension->id()))
        return blink::mojom::PermissionStatus::GRANTED;
    }
  }
#endif

  // No enabled extensions exist, so check the normal host content settings.
  HostContentSettingsMap* host_content_settings_map =
      io_data->GetHostContentSettingsMap();
  ContentSetting setting = host_content_settings_map->GetContentSetting(
      origin,
      origin,
      CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
      content_settings::ResourceIdentifier());

  if (setting == CONTENT_SETTING_ALLOW)
    return blink::mojom::PermissionStatus::GRANTED;
  if (setting == CONTENT_SETTING_BLOCK)
    return blink::mojom::PermissionStatus::DENIED;

  // Check whether the permission has been embargoed (automatically blocked).
  // TODO(crbug.com/658020): make PermissionManager::GetPermissionStatus thread
  // safe so it isn't necessary to do this HostContentSettingsMap and embargo
  // check outside of the permissions code.
  PermissionResult result = PermissionDecisionAutoBlocker::GetEmbargoResult(
      host_content_settings_map, origin, CONTENT_SETTINGS_TYPE_NOTIFICATIONS,
      base::Time::Now());
  DCHECK(result.content_setting == CONTENT_SETTING_ASK ||
         result.content_setting == CONTENT_SETTING_BLOCK);
  return result.content_setting == CONTENT_SETTING_ASK
             ? blink::mojom::PermissionStatus::ASK
             : blink::mojom::PermissionStatus::DENIED;
}

// TODO(awdf): Rename to DisplayNonPersistentNotification (Similar for Close)
void PlatformNotificationServiceImpl::DisplayNotification(
    BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Posted tasks can request notifications to be added, which would cause a
  // crash (see |ScopedKeepAlive|). We just do nothing here, the user would not
  // see the notification anyway, since we are shutting down.
  if (g_browser_process->IsShuttingDown())
    return;

  Profile* profile = Profile::FromBrowserContext(browser_context);
  DCHECK(profile);
  DCHECK_EQ(0u, notification_data.actions.size());
  DCHECK_EQ(0u, notification_resources.action_icons.size());

  message_center::Notification notification = CreateNotificationFromData(
      profile, origin, notification_id, notification_data,
      notification_resources, nullptr /* delegate */);

  NotificationDisplayServiceFactory::GetForProfile(profile)->Display(
      NotificationHandler::Type::WEB_NON_PERSISTENT, notification);
}

void PlatformNotificationServiceImpl::DisplayPersistentNotification(
    BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& service_worker_scope,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Posted tasks can request notifications to be added, which would cause a
  // crash (see |ScopedKeepAlive|). We just do nothing here, the user would not
  // see the notification anyway, since we are shutting down.
  if (g_browser_process->IsShuttingDown())
    return;

  Profile* profile = Profile::FromBrowserContext(browser_context);
  DCHECK(profile);

  message_center::Notification notification = CreateNotificationFromData(
      profile, origin, notification_id, notification_data,
      notification_resources, nullptr /* delegate */);
  auto metadata = std::make_unique<PersistentNotificationMetadata>();
  metadata->service_worker_scope = service_worker_scope;

  NotificationDisplayServiceFactory::GetForProfile(profile)->Display(
      NotificationHandler::Type::WEB_PERSISTENT, notification,
      std::move(metadata));

  GetMetricsLogger(browser_context)->LogPersistentNotificationShown();
}

void PlatformNotificationServiceImpl::CloseNotification(
    BrowserContext* browser_context,
    const std::string& notification_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Profile* profile = Profile::FromBrowserContext(browser_context);
  DCHECK(profile);

  NotificationDisplayServiceFactory::GetForProfile(profile)->Close(
      NotificationHandler::Type::WEB_NON_PERSISTENT, notification_id);
}

void PlatformNotificationServiceImpl::ClosePersistentNotification(
    BrowserContext* browser_context,
    const std::string& notification_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Profile* profile = Profile::FromBrowserContext(browser_context);
  DCHECK(profile);

  closed_notifications_.insert(notification_id);

  NotificationDisplayServiceFactory::GetForProfile(profile)->Close(
      NotificationHandler::Type::WEB_PERSISTENT, notification_id);
}

void PlatformNotificationServiceImpl::GetDisplayedNotifications(
    BrowserContext* browser_context,
    const DisplayedNotificationsCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Profile* profile = Profile::FromBrowserContext(browser_context);
  // Tests will not have a message center.
  if (!profile || profile->AsTestingProfile()) {
    auto displayed_notifications = std::make_unique<std::set<std::string>>();
    callback.Run(std::move(displayed_notifications),
                 false /* supports_synchronization */);
    return;
  }
  NotificationDisplayServiceFactory::GetForProfile(profile)->GetDisplayed(
      callback);
}

void PlatformNotificationServiceImpl::OnClickEventDispatchComplete(
    base::OnceClosure completed_closure,
    content::PersistentNotificationStatus status) {
  UMA_HISTOGRAM_ENUMERATION(
      "Notifications.PersistentWebNotificationClickResult", status,
      content::PersistentNotificationStatus::
          PERSISTENT_NOTIFICATION_STATUS_MAX);
#if BUILDFLAG(ENABLE_BACKGROUND_MODE)
  DCHECK_GT(pending_click_dispatch_events_, 0);
  if (--pending_click_dispatch_events_ == 0) {
    click_dispatch_keep_alive_.reset();
  }
#endif

  std::move(completed_closure).Run();
}

void PlatformNotificationServiceImpl::OnCloseEventDispatchComplete(
    base::OnceClosure completed_closure,
    content::PersistentNotificationStatus status) {
  UMA_HISTOGRAM_ENUMERATION(
      "Notifications.PersistentWebNotificationCloseResult", status,
      content::PersistentNotificationStatus::
          PERSISTENT_NOTIFICATION_STATUS_MAX);

  std::move(completed_closure).Run();
}

message_center::Notification
PlatformNotificationServiceImpl::CreateNotificationFromData(
    Profile* profile,
    const GURL& origin,
    const std::string& notification_id,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources,
    scoped_refptr<message_center::NotificationDelegate> delegate) const {
  DCHECK_EQ(notification_data.actions.size(),
            notification_resources.action_icons.size());

  message_center::RichNotificationData optional_fields;

  optional_fields.settings_button_handler =
      base::FeatureList::IsEnabled(message_center::kNewStyleNotifications)
          ? message_center::SettingsButtonHandler::INLINE
          : message_center::SettingsButtonHandler::DELEGATE;

  // TODO(peter): Handle different screen densities instead of always using the
  // 1x bitmap - crbug.com/585815.
  message_center::Notification notification(
      message_center::NOTIFICATION_TYPE_SIMPLE, notification_id,
      notification_data.title, notification_data.body,
      gfx::Image::CreateFrom1xBitmap(notification_resources.notification_icon),
      base::UTF8ToUTF16(origin.host()), origin, NotifierId(origin),
      optional_fields, delegate);

  notification.set_context_message(
      DisplayNameForContextMessage(profile, origin));
  notification.set_vibration_pattern(notification_data.vibration_pattern);
  notification.set_timestamp(notification_data.timestamp);
  notification.set_renotify(notification_data.renotify);
  notification.set_silent(notification_data.silent);
  if (ShouldDisplayWebNotificationOnFullScreen(profile, origin)) {
    notification.set_fullscreen_visibility(
        message_center::FullscreenVisibility::OVER_USER);
  }

  if (!notification_resources.image.drawsNothing()) {
    notification.set_type(message_center::NOTIFICATION_TYPE_IMAGE);
    notification.set_image(
        gfx::Image::CreateFrom1xBitmap(notification_resources.image));
    // n.b. this should only be posted once per notification.
    if (g_browser_process->safe_browsing_service() &&
        g_browser_process->safe_browsing_service()->enabled_by_prefs()) {
      g_browser_process->safe_browsing_service()
          ->ping_manager()
          ->ReportNotificationImage(
              profile,
              g_browser_process->safe_browsing_service()->database_manager(),
              origin, notification_resources.image);
    }
  }

  // Badges are only supported on Android, primarily because it's the only
  // platform that makes good use of them in the status bar.
#if defined(OS_ANDROID)
  // TODO(peter): Handle different screen densities instead of always using the
  // 1x bitmap - crbug.com/585815.
  notification.set_small_image(
      gfx::Image::CreateFrom1xBitmap(notification_resources.badge));
#endif  // defined(OS_ANDROID)

  // Developer supplied action buttons.
  std::vector<message_center::ButtonInfo> buttons;
  for (size_t i = 0; i < notification_data.actions.size(); ++i) {
    const content::PlatformNotificationAction& action =
        notification_data.actions[i];
    message_center::ButtonInfo button(action.title);
    // TODO(peter): Handle different screen densities instead of always using
    // the 1x bitmap - crbug.com/585815.
    button.icon =
        gfx::Image::CreateFrom1xBitmap(notification_resources.action_icons[i]);
    if (action.type == content::PLATFORM_NOTIFICATION_ACTION_TYPE_TEXT) {
      button.placeholder = action.placeholder.as_optional_string16().value_or(
          l10n_util::GetStringUTF16(IDS_NOTIFICATION_REPLY_PLACEHOLDER));
    }
    buttons.push_back(button);
  }
  notification.set_buttons(buttons);

  // On desktop, notifications with require_interaction==true stay on-screen
  // rather than minimizing to the notification center after a timeout.
  // On mobile, this is ignored (notifications are minimized at all times).
  if (notification_data.require_interaction)
    notification.set_never_timeout(true);

  return notification;
}

base::string16 PlatformNotificationServiceImpl::DisplayNameForContextMessage(
    Profile* profile,
    const GURL& origin) const {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // If the source is an extension, lookup the display name.
  if (origin.SchemeIs(extensions::kExtensionScheme)) {
    const extensions::Extension* extension =
        extensions::ExtensionRegistry::Get(profile)->GetExtensionById(
            origin.host(), extensions::ExtensionRegistry::EVERYTHING);
    DCHECK(extension);

    return base::UTF8ToUTF16(extension->name());
  }
#endif

  return base::string16();
}

void PlatformNotificationServiceImpl::RecordSiteEngagement(
    BrowserContext* browser_context,
    const GURL& origin) {
  // TODO(dominickn, peter): This would be better if the site engagement service
  // could directly observe each notification.
  SiteEngagementService* engagement_service =
      SiteEngagementService::Get(Profile::FromBrowserContext(browser_context));
  engagement_service->HandleNotificationInteraction(origin);
}
