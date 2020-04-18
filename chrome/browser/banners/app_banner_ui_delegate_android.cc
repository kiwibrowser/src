// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/banners/app_banner_ui_delegate_android.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/android/shortcut_helper.h"
#include "chrome/browser/android/shortcut_info.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/android/webapk/webapk_install_service.h"
#include "chrome/browser/android/webapk/webapk_metrics.h"
#include "chrome/browser/banners/app_banner_manager.h"
#include "chrome/browser/banners/app_banner_metrics.h"
#include "chrome/browser/banners/app_banner_settings_helper.h"
#include "chrome/browser/browser_process.h"
#include "components/rappor/public/rappor_utils.h"
#include "components/rappor/rappor_service_impl.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/web_contents.h"
#include "jni/AppBannerUiDelegateAndroid_jni.h"
#include "ui/gfx/android/java_bitmap.h"
#include "url/gurl.h"

namespace banners {

// static
std::unique_ptr<AppBannerUiDelegateAndroid> AppBannerUiDelegateAndroid::Create(
    base::WeakPtr<AppBannerManager> weak_manager,
    std::unique_ptr<ShortcutInfo> shortcut_info,
    const SkBitmap& primary_icon,
    const SkBitmap& badge_icon,
    WebappInstallSource install_source,
    bool is_webapk) {
  return std::unique_ptr<AppBannerUiDelegateAndroid>(
      new AppBannerUiDelegateAndroid(weak_manager, std::move(shortcut_info),
                                     primary_icon, badge_icon, install_source,
                                     is_webapk));
}

// static
std::unique_ptr<AppBannerUiDelegateAndroid> AppBannerUiDelegateAndroid::Create(
    base::WeakPtr<AppBannerManager> weak_manager,
    const base::string16& app_title,
    const base::android::ScopedJavaLocalRef<jobject>& native_app_data,
    const SkBitmap& icon,
    const std::string& native_app_package_name,
    const std::string& referrer) {
  return std::unique_ptr<AppBannerUiDelegateAndroid>(
      new AppBannerUiDelegateAndroid(weak_manager, app_title, native_app_data,
                                     icon, native_app_package_name, referrer));
}

AppBannerUiDelegateAndroid::~AppBannerUiDelegateAndroid() {
  if (!has_user_interaction_) {
    AppType type = GetType();
    if (type == AppType::NATIVE) {
      TrackUserResponse(USER_RESPONSE_NATIVE_APP_IGNORED);
    } else {
      TrackUserResponse(USER_RESPONSE_WEB_APP_IGNORED);
      if (type == AppType::WEBAPK)
        webapk::TrackInstallEvent(webapk::INFOBAR_IGNORED);
    }
  }

  TrackDismissEvent(DISMISS_EVENT_DISMISSED);
  Java_AppBannerUiDelegateAndroid_destroy(base::android::AttachCurrentThread(),
                                          java_delegate_);
  java_delegate_.Reset();
}

const base::string16& AppBannerUiDelegateAndroid::GetAppTitle() const {
  return app_title_;
}

base::android::ScopedJavaLocalRef<jobject>
AppBannerUiDelegateAndroid::GetJavaObject() {
  return base::android::ScopedJavaLocalRef<jobject>(java_delegate_);
}

const base::android::ScopedJavaLocalRef<jobject>
AppBannerUiDelegateAndroid::GetNativeAppData() const {
  return base::android::ScopedJavaLocalRef<jobject>(native_app_data_);
}

int AppBannerUiDelegateAndroid::GetInstallState() const {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> jpackage_name(
      base::android::ConvertUTF8ToJavaString(env, package_name_));
  return Java_AppBannerUiDelegateAndroid_determineInstallState(
      env, java_delegate_, jpackage_name);
}

const SkBitmap& AppBannerUiDelegateAndroid::GetPrimaryIcon() const {
  return primary_icon_;
}

AppBannerUiDelegateAndroid::AppType AppBannerUiDelegateAndroid::GetType()
    const {
  return type_;
}

const GURL& AppBannerUiDelegateAndroid::GetWebAppUrl() const {
  return shortcut_info_->url;
}

void AppBannerUiDelegateAndroid::AddToHomescreen(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (!weak_manager_.get())
    return;

  InstallApp(weak_manager_->web_contents());
}

const base::android::ScopedJavaLocalRef<jobject>
AppBannerUiDelegateAndroid::GetAddToHomescreenDialogForTesting() const {
  return Java_AppBannerUiDelegateAndroid_getDialogForTesting(
      base::android::AttachCurrentThread(), java_delegate_);
}

bool AppBannerUiDelegateAndroid::InstallApp(
    content::WebContents* web_contents) {
  has_user_interaction_ = true;

  if (!web_contents) {
    TrackDismissEvent(DISMISS_EVENT_ERROR);
    return true;
  }

  bool should_close_banner = true;
  switch (GetType()) {
    case AppType::NATIVE:
      should_close_banner = InstallOrOpenNativeApp();
      break;
    case AppType::WEBAPK:
      InstallWebApk(web_contents);
      break;
    case AppType::LEGACY_WEBAPP:
      InstallLegacyWebApp(web_contents);
      break;
  }
  SendBannerAccepted();
  return should_close_banner;
}

void AppBannerUiDelegateAndroid::OnNativeAppInstallStarted(
    content::WebContents* web_contents) {
  AppBannerSettingsHelper::RecordBannerEvent(
      web_contents, web_contents->GetVisibleURL(), package_name_,
      AppBannerSettingsHelper::APP_BANNER_EVENT_DID_ADD_TO_HOMESCREEN,
      AppBannerManager::GetCurrentTime());

  TrackInstallEvent(INSTALL_EVENT_NATIVE_APP_INSTALL_STARTED);
  rappor::SampleDomainAndRegistryFromGURL(g_browser_process->rappor_service(),
                                          "AppBanner.NativeApp.Installed",
                                          web_contents->GetURL());
}

void AppBannerUiDelegateAndroid::OnNativeAppInstallFinished(bool success) {
  if (success)
    TrackInstallEvent(INSTALL_EVENT_NATIVE_APP_INSTALL_COMPLETED);
  else
    TrackDismissEvent(DISMISS_EVENT_INSTALL_TIMEOUT);
}

void AppBannerUiDelegateAndroid::OnUiCancelled(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  OnUiCancelled();
}

void AppBannerUiDelegateAndroid::OnUiCancelled() {
  if (!weak_manager_.get())
    return;

  weak_manager_->SendBannerDismissed();
  has_user_interaction_ = true;
  content::WebContents* web_contents = weak_manager_->web_contents();

  if (IsForNativeApp()) {
    DCHECK(!package_name_.empty());
    TrackUserResponse(USER_RESPONSE_NATIVE_APP_DISMISSED);
    AppBannerSettingsHelper::RecordBannerDismissEvent(
        web_contents, package_name_, AppBannerSettingsHelper::NATIVE);
  } else {
    DCHECK(GetType() == AppType::WEBAPK || GetType() == AppType::LEGACY_WEBAPP);

    if (GetType() == AppType::WEBAPK)
      webapk::TrackInstallEvent(webapk::INFOBAR_DISMISSED_BEFORE_INSTALLATION);
    TrackUserResponse(USER_RESPONSE_WEB_APP_DISMISSED);
    AppBannerSettingsHelper::RecordBannerDismissEvent(
        web_contents, shortcut_info_->url.spec(), AppBannerSettingsHelper::WEB);
  }
}

bool AppBannerUiDelegateAndroid::ShowDialog() {
  if (!weak_manager_.get())
    return false;

  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jstring> java_app_title =
      base::android::ConvertUTF16ToJavaString(env, app_title_);

  DCHECK(!primary_icon_.drawsNothing());
  base::android::ScopedJavaLocalRef<jobject> java_bitmap =
      gfx::ConvertToJavaBitmap(&primary_icon_);

  if (IsForNativeApp()) {
    return Java_AppBannerUiDelegateAndroid_showNativeAppDialog(
        env, java_delegate_, java_app_title, java_bitmap, native_app_data_);
  }

  // Trim down the app URL to the origin. Banners only show on secure origins,
  // so elide the scheme.
  base::android::ScopedJavaLocalRef<jstring> java_app_url =
      base::android::ConvertUTF16ToJavaString(
          env, url_formatter::FormatUrlForSecurityDisplay(
                   shortcut_info_->url,
                   url_formatter::SchemeDisplay::OMIT_CRYPTOGRAPHIC));

  return Java_AppBannerUiDelegateAndroid_showWebAppDialog(
      env, java_delegate_, java_app_title, java_bitmap, java_app_url);
}

void AppBannerUiDelegateAndroid::ShowNativeAppDetails() {
  if (native_app_data_.is_null())
    return;

  Java_AppBannerUiDelegateAndroid_showAppDetails(
      base::android::AttachCurrentThread(), java_delegate_, native_app_data_);

  TrackDismissEvent(DISMISS_EVENT_BANNER_CLICK);
}

void AppBannerUiDelegateAndroid::ShowNativeAppDetails(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  ShowNativeAppDetails();
}

AppBannerUiDelegateAndroid::AppBannerUiDelegateAndroid(
    base::WeakPtr<AppBannerManager> weak_manager,
    std::unique_ptr<ShortcutInfo> shortcut_info,
    const SkBitmap& primary_icon,
    const SkBitmap& badge_icon,
    WebappInstallSource install_source,
    bool is_webapk)
    : weak_manager_(weak_manager),
      app_title_(shortcut_info->name),
      shortcut_info_(std::move(shortcut_info)),
      primary_icon_(primary_icon),
      badge_icon_(badge_icon),
      type_(is_webapk ? AppType::WEBAPK : AppType::LEGACY_WEBAPP),
      install_source_(install_source),
      has_user_interaction_(false) {
  if (is_webapk)
    shortcut_info_->UpdateSource(ShortcutInfo::SOURCE_APP_BANNER_WEBAPK);
  else
    shortcut_info_->UpdateSource(ShortcutInfo::SOURCE_APP_BANNER);

  CreateJavaDelegate();
}

AppBannerUiDelegateAndroid::AppBannerUiDelegateAndroid(
    base::WeakPtr<AppBannerManager> weak_manager,
    const base::string16& app_title,
    const base::android::ScopedJavaLocalRef<jobject>& native_app_data,
    const SkBitmap& icon,
    const std::string& native_app_package_name,
    const std::string& referrer)
    : weak_manager_(weak_manager),
      app_title_(app_title),
      native_app_data_(native_app_data),
      primary_icon_(icon),
      package_name_(native_app_package_name),
      referrer_(referrer),
      type_(AppType::NATIVE),
      has_user_interaction_(false) {
  DCHECK(!native_app_data_.is_null());
  DCHECK(!package_name_.empty());
  CreateJavaDelegate();
}

void AppBannerUiDelegateAndroid::CreateInstallerDelegate(
    base::android::ScopedJavaLocalRef<jobject> jobserver) {
  DCHECK(!java_delegate_.is_null());

  Java_AppBannerUiDelegateAndroid_createInstallerDelegate(
      base::android::AttachCurrentThread(), java_delegate_, jobserver);
}

void AppBannerUiDelegateAndroid::CreateJavaDelegate() {
  TabAndroid* tab = TabAndroid::FromWebContents(weak_manager_->web_contents());

  java_delegate_.Reset(Java_AppBannerUiDelegateAndroid_create(
      base::android::AttachCurrentThread(), reinterpret_cast<intptr_t>(this),
      tab->GetJavaObject()));
}

bool AppBannerUiDelegateAndroid::InstallOrOpenNativeApp() {
  DCHECK(IsForNativeApp());
  TrackUserResponse(USER_RESPONSE_NATIVE_APP_ACCEPTED);
  JNIEnv* env = base::android::AttachCurrentThread();

  base::android::ScopedJavaLocalRef<jstring> jreferrer(
      base::android::ConvertUTF8ToJavaString(env, referrer_));

  bool was_opened = Java_AppBannerUiDelegateAndroid_installOrOpenNativeApp(
      env, java_delegate_, native_app_data_, jreferrer);

  if (was_opened)
    TrackDismissEvent(DISMISS_EVENT_APP_OPEN);
  else
    TrackInstallEvent(INSTALL_EVENT_NATIVE_APP_INSTALL_TRIGGERED);

  return was_opened;
}

void AppBannerUiDelegateAndroid::InstallWebApk(
    content::WebContents* web_contents) {
  TrackUserResponse(USER_RESPONSE_WEB_APP_ACCEPTED);
  AppBannerSettingsHelper::RecordBannerInstallEvent(
      web_contents, shortcut_info_->url.spec(), AppBannerSettingsHelper::WEB);

  WebApkInstallService::Get(web_contents->GetBrowserContext())
      ->InstallAsync(web_contents, *shortcut_info_, primary_icon_, badge_icon_,
                     install_source_);
}

void AppBannerUiDelegateAndroid::InstallLegacyWebApp(
    content::WebContents* web_contents) {
  DCHECK_EQ(AppType::LEGACY_WEBAPP, GetType());

  TrackUserResponse(USER_RESPONSE_WEB_APP_ACCEPTED);

  AppBannerSettingsHelper::RecordBannerInstallEvent(
      web_contents, shortcut_info_->url.spec(), AppBannerSettingsHelper::WEB);

  ShortcutHelper::AddToLauncherWithSkBitmap(web_contents, *shortcut_info_,
                                            primary_icon_);
}

void AppBannerUiDelegateAndroid::SendBannerAccepted() {
  if (!weak_manager_)
    return;

  weak_manager_->SendBannerAccepted();

  // Send the appinstalled event and perform install time logging. Note that
  // this is fired *before* the installation actually takes place (which can be
  // a significant amount of time later, especially if using WebAPKs).
  // TODO(mgiuca): Fire the event *after* the installation is completed.
  bool is_native = IsForNativeApp();
  weak_manager_->OnInstall(is_native, is_native
                                          ? blink::kWebDisplayModeUndefined
                                          : shortcut_info_->display);
}

}  // namespace banners
