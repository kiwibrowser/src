// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>
#include <stddef.h>

#include <memory>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/android/jni_weak_ref.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/values.h"
#include "chrome/browser/browsing_data/browsing_data_important_sites_util.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_delegate.h"
#include "chrome/browser/engagement/important_sites_util.h"
#include "chrome/browser/history/web_history_service_factory.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_features.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/browsing_data/core/history_notice_utils.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "chrome/browser/extensions/extension_action_manager.h"
#include "chrome/browser/extensions/extension_action_runner.h"
#include "chrome/browser/extensions/extension_action_icon_factory.h"
#include "chrome/browser/extensions/extension_context_menu_model.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "extensions/browser/extension_host_observer.h"
#include "ui/gfx/image/image.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/url_constants.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_util.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "chrome/browser/extensions/tab_helper.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/bindings_policy.h"
#include "extensions/browser/extension_icon_placeholder.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/image_loader.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_icon_set.h"
#include "extensions/common/extension_resource.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "net/base/file_stream.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/page_transition_types.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/image/image_skia.h"
#include "chrome/browser/extensions/extension_action_runner.h"
#include "chrome/browser/ui/extensions/icon_with_badge_image_source.h"
#include "chrome/browser/ui/extensions/extension_action_view_controller.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/skia_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "chrome/browser/extensions/api/extension_action/extension_action_api.h"

#include "jni/AppMenu_jni.h"

using base::android::JavaParamRef;

using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::HasException;
using base::android::JavaByteArrayToByteVector;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaByteArray;

bool PageActionWantsToRun(
    content::WebContents* web_contents, ExtensionAction* extension_action_) {
  return extension_action_->action_type() ==
             extensions::ActionInfo::TYPE_PAGE &&
         extension_action_->GetIsVisible(
             SessionTabHelper::IdForTab(web_contents).id());
}


bool HasBeenBlocked(
    content::WebContents* web_contents, const extensions::Extension* extension) {
  extensions::ExtensionActionRunner* action_runner =
    extensions::ExtensionActionRunner::GetForWebContents(web_contents);
  return action_runner && action_runner->WantsToRun(extension);
}

bool IsEnabled(
    content::WebContents* web_contents, const extensions::Extension* extension, ExtensionAction* extension_action_) {
  return extension_action_->GetIsVisible(
             SessionTabHelper::IdForTab(web_contents).id()) ||
         HasBeenBlocked(web_contents, extension);
}

gfx::Image GetIcon(int tab_id, ExtensionAction* extension_action_) {
  gfx::Image icon = extension_action_->GetExplicitlySetIcon(tab_id);
  if (!icon.IsEmpty())
    return icon;

  icon = extension_action_->GetDeclarativeIcon(tab_id);
  if (!icon.IsEmpty())
    return icon;

  return extension_action_->GetDefaultIconImage();
}

std::unique_ptr<IconWithBadgeImageSource> GetIconImageSource(
    const extensions::Extension* extension,
    ExtensionAction* extension_action_,
    content::WebContents* web_contents,
    const gfx::Size& size) {
  int tab_id = SessionTabHelper::IdForTab(web_contents).id();
  std::unique_ptr<IconWithBadgeImageSource> image_source(
      new IconWithBadgeImageSource(size));

  GetIcon(tab_id, extension_action_);
  image_source->SetIcon(GetIcon(tab_id, extension_action_));

  std::unique_ptr<IconWithBadgeImageSource::Badge> badge;
  std::string badge_text = extension_action_->GetBadgeText(tab_id);
  if (!badge_text.empty()) {
    badge.reset(new IconWithBadgeImageSource::Badge(
            badge_text,
            extension_action_->GetBadgeTextColor(tab_id),
            extension_action_->GetBadgeBackgroundColor(tab_id)));
  }
  image_source->SetBadge(std::move(badge));

  // If the extension doesn't want to run on the active web contents, we
  // grayscale it to indicate that.
  image_source->set_grayscale(!IsEnabled(web_contents, extension, extension_action_));

  // If the action *does* want to run on the active web contents and is also
  // overflowed, we add a decoration so that the user can see which overflowed
  // action wants to run (since they wouldn't be able to see the change from
  // grayscale to color).
  bool is_overflow = false;

  bool has_blocked_actions = HasBeenBlocked(web_contents, extension);
//  image_source->set_state(state);
  image_source->set_paint_blocked_actions_decoration(has_blocked_actions);
  image_source->set_paint_page_action_decoration(
      !has_blocked_actions && is_overflow &&
      PageActionWantsToRun(web_contents, extension_action_));

  return image_source;
}

// static
void JNI_AppMenu_CallExtension(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jobject>& jprofile,
    const base::android::JavaParamRef<jobject>& jweb_contents,
    const JavaParamRef<jstring>& j_extension_id) {
  std::string extension_to_call = ConvertJavaStringToUTF8(env, j_extension_id);
  LOG(INFO) << "[EXTENSIONS] Calling AppMenu::CallExtension: " << extension_to_call;
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  LOG(INFO) << "[EXTENSIONS] Captured profile: " << profile;

  // The object that will be used to get the browser action icon for us.
  // It may load the icon asynchronously (in which case the initial icon
  // returned by the factory will be transparent), so we have to observe it for
  // updates to the icon.
  // The associated ExtensionRegistry; cached for quick checking.
  extensions::ExtensionRegistry* registry;

  registry = extensions::ExtensionRegistry::Get(profile);

  ExtensionAction* extension_action_;
  extensions::ExtensionActionManager* manager =
      extensions::ExtensionActionManager::Get(profile);
  const extensions::ExtensionSet& enabled_extensions = registry->enabled_extensions();
  const extensions::Extension* extension_ptr = enabled_extensions.GetByID(extension_to_call);
  if (extension_ptr) {
    extension_action_ = manager->GetBrowserAction(*extension_ptr);
    if (!extension_action_) {
      extension_action_ = manager->GetPageAction(*extension_ptr);
    }
    if (extension_action_) {
       LOG(INFO) << "[EXTENSIONS] Dispatching extension_action_ for " << extension_to_call;
       extensions::ExtensionActionAPI* action_api = extensions::ExtensionActionAPI::Get(profile);
       content::WebContents* web_contents = content::WebContents::FromJavaWebContents(jweb_contents);
       if (web_contents != nullptr) {
         LOG(INFO) << "[EXTENSIONS] Granting tab access to: " << extension_to_call;
         extensions::TabHelper::FromWebContents(web_contents)
             ->active_tab_permission_granter()
             ->GrantIfRequested(extension_ptr);
       }
       action_api->DispatchExtensionActionClicked(*extension_action_, web_contents, extension_ptr);
       LOG(INFO) << "[EXTENSIONS] Dispatched JS extension_action_ for " << extension_to_call;
    }
  }
}

// static
void JNI_AppMenu_GrantExtensionActiveTab(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jobject>& jprofile,
    const base::android::JavaParamRef<jobject>& jweb_contents,
    const JavaParamRef<jstring>& j_extension_id) {
  std::string extension_to_call = ConvertJavaStringToUTF8(env, j_extension_id);
  LOG(INFO) << "[EXTENSIONS] Calling AppMenu::GrantExtensionActiveTab: " << extension_to_call;
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);

  extensions::ExtensionRegistry* registry;

  registry = extensions::ExtensionRegistry::Get(profile);

  ExtensionAction* extension_action_;
  extensions::ExtensionActionManager* manager =
      extensions::ExtensionActionManager::Get(profile);
  const extensions::ExtensionSet& enabled_extensions = registry->enabled_extensions();
  const extensions::Extension* extension_ptr = enabled_extensions.GetByID(extension_to_call);
  if (extension_ptr) {
    extension_action_ = manager->GetBrowserAction(*extension_ptr);
    if (!extension_action_) {
      extension_action_ = manager->GetPageAction(*extension_ptr);
    }
    if (extension_action_) {
       content::WebContents* web_contents = content::WebContents::FromJavaWebContents(jweb_contents);
       if (web_contents != nullptr) {
         LOG(INFO) << "[EXTENSIONS] Granting tab access to: " << extension_to_call;
         extensions::TabHelper::FromWebContents(web_contents)
             ->active_tab_permission_granter()
             ->GrantIfRequested(extension_ptr);
       }
    }
  }
}

ScopedJavaLocalRef<jstring> JNI_AppMenu_GetRunningExtensions(
    JNIEnv* env,
    const JavaParamRef<jclass>& jcaller,
    const JavaParamRef<jobject>& jprofile,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  LOG(INFO) << "[EXTENSIONS] Calling AppMenu::GetRunningExtensions";
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  LOG(INFO) << "[EXTENSIONS] Captured profile: " << profile;

  // The object that will be used to get the browser action icon for us.
  // It may load the icon asynchronously (in which case the initial icon
  // returned by the factory will be transparent), so we have to observe it for
  // updates to the icon.
  // The associated ExtensionRegistry; cached for quick checking.
  extensions::ExtensionRegistry* registry;

  registry = extensions::ExtensionRegistry::Get(profile);

  std::string result = "";

  LOG(INFO) << "[EXTENSIONS] Getting enabled extensions";
  const extensions::ExtensionSet& enabled_extensions = registry->enabled_extensions();

  for (const auto& extension : enabled_extensions) {
      LOG(INFO) << "[EXTENSIONS] Found extension: " << extension->id();
      LOG(INFO) << "[EXTENSIONS] Found extension with name: " << extension->name();
      LOG(INFO) << "[EXTENSIONS] Found extension with short name: " << extension->short_name();
//      if (ExtensionActionAPI::GetBrowserActionVisibility(extension->id())) {
      if (true) {
        LOG(INFO) << "[EXTENSIONS] Found extension: " << extension->id() << " IS VISIBLE";
        ExtensionAction* extension_action_;
        extensions::ExtensionActionManager* manager =
            extensions::ExtensionActionManager::Get(profile);
        const extensions::Extension* extension_ptr = enabled_extensions.GetByID(extension->id());
        if (extension_ptr) {
          extension_action_ = manager->GetBrowserAction(*extension_ptr);
          if (!extension_action_) {
            extension_action_ = manager->GetPageAction(*extension_ptr);
          }
          if (extension_action_) {
             LOG(INFO) << "[EXTENSIONS] Got extension_action_ for " << extension->id();
             content::WebContents* web_contents = content::WebContents::FromJavaWebContents(jweb_contents);
             LOG(INFO) << "[EXTENSIONS] Got access to web_contents: " << web_contents;
             std::unique_ptr<IconWithBadgeImageSource> icon_badge = GetIconImageSource(extension_ptr, extension_action_, web_contents, gfx::Size(48, 48));
             gfx::Canvas canvas(gfx::Size(48, 48), 1.0f, false);
             icon_badge->Draw(&canvas);
             LOG(INFO) << "[EXTENSIONS] Canvas drawn";
             SkBitmap bitmap = canvas.GetBitmap();
             std::string base64_image = webui::GetBitmapDataUrl(bitmap);
             LOG(INFO) << "[EXTENSIONS] Canvas converted to bitmap: " << base64_image << " on " << extension->short_name();
             if (extension_action_->HasPopup(SessionTabHelper::IdForTab(web_contents).id())) {
               GURL popup_url = extension_action_->GetPopupUrl(
                   SessionTabHelper::IdForTab(web_contents).id());
               result += extension->name() + "\x1E" + extension->id() + "\x1E" + popup_url.spec() + "\x1E" + base64_image + "\x1F";
             } else {
               // Record separator and Unit separator in ASCII table
               result += extension->name() + "\x1E" + extension->id() + "\x1E" + "" + "\x1E" + base64_image + "\x1F";
             }
          }
        }
      } else {
        LOG(INFO) << "[EXTENSIONS] (ignoring) extension: " << extension->id() << " IS NOT VISIBLE";
      }
  }
  LOG(INFO) << "[EXTENSIONS] Result is: " << result;
  return ConvertUTF8ToJavaString(env, result);
}
