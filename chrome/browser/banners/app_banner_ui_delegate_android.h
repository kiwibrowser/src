// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BANNERS_APP_BANNER_UI_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_BANNERS_APP_BANNER_UI_DELEGATE_ANDROID_H_

#include <memory>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "chrome/browser/installable/installable_metrics.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace content {
class WebContents;
}

struct ShortcutInfo;

namespace banners {

class AppBannerManager;

// Delegate provided to the app banner UI surfaces to install a web app or
// native app.
class AppBannerUiDelegateAndroid {
 public:
  // Describes the type of app for which this class holds data.
  enum class AppType {
    NATIVE,
    WEBAPK,
    LEGACY_WEBAPP,
  };

  // Creates a delegate for promoting the installation of a web app.
  static std::unique_ptr<AppBannerUiDelegateAndroid> Create(
      base::WeakPtr<AppBannerManager> weak_manager,
      std::unique_ptr<ShortcutInfo> info,
      const SkBitmap& primary_icon,
      const SkBitmap& badge_icon,
      WebappInstallSource install_source,
      bool is_webapk);

  // Creates a delegate for promoting the installation of an Android app.
  static std::unique_ptr<AppBannerUiDelegateAndroid> Create(
      base::WeakPtr<AppBannerManager> weak_manager,
      const base::string16& app_title,
      const base::android::ScopedJavaLocalRef<jobject>& native_app_data,
      const SkBitmap& icon,
      const std::string& native_app_package_name,
      const std::string& referrer);

  ~AppBannerUiDelegateAndroid();

  const base::string16& GetAppTitle() const;
  base::android::ScopedJavaLocalRef<jobject> GetJavaObject();
  const base::android::ScopedJavaLocalRef<jobject> GetNativeAppData() const;

  int GetInstallState() const;
  const SkBitmap& GetPrimaryIcon() const;
  AppType GetType() const;
  const GURL& GetWebAppUrl() const;

  // Creates the Java-side InstallerDelegate, passing |jobserver| to receive
  // progress updates on the installation of a native app.
  void CreateInstallerDelegate(
      base::android::ScopedJavaLocalRef<jobject> jobserver);

  // Called through the JNI to add the app described by this class to home
  // screen.
  void AddToHomescreen(JNIEnv* env,
                       const base::android::JavaParamRef<jobject>& obj);

  // Returns a reference to the Java-side AddToHomescreenDialog owned by this
  // object, or null if it does not exist.
  const base::android::ScopedJavaLocalRef<jobject>
  GetAddToHomescreenDialogForTesting() const;

  // Installs the app referenced by the data in this object. Returns |true| if
  // the installation UI should be dismissed.
  bool InstallApp(content::WebContents* web_contents);

  // Called by the UI layer to indicate that a native app has begun
  // installation.
  void OnNativeAppInstallStarted(content::WebContents* web_contents);

  // Called by the UI layer to indicate that a native app has finished
  // installation.
  void OnNativeAppInstallFinished(bool success);

  // Called through the JNI to indicate that the user has dismissed the
  // installation UI.
  void OnUiCancelled(JNIEnv* env,
                     const base::android::JavaParamRef<jobject>& obj);

  // Called by the UI layer to indicate that the user has dismissed the
  // installation UI.
  void OnUiCancelled();

  // Called to show a modal app banner. Returns true if the dialog is
  // successfully shown.
  bool ShowDialog();

  // Called by the UI layer to display the details for a native app.
  void ShowNativeAppDetails();

  void ShowNativeAppDetails(JNIEnv* env,
                            const base::android::JavaParamRef<jobject>& obj);

 private:
  // Delegate for promoting a web app.
  AppBannerUiDelegateAndroid(base::WeakPtr<AppBannerManager> weak_manager,
                             std::unique_ptr<ShortcutInfo> info,
                             const SkBitmap& primary_icon,
                             const SkBitmap& badge_icon,
                             WebappInstallSource install_source,
                             bool is_webapk);

  // Delegate for promoting an Android app.
  AppBannerUiDelegateAndroid(
      base::WeakPtr<AppBannerManager> weak_manager,
      const base::string16& app_title,
      const base::android::ScopedJavaLocalRef<jobject>& native_app_data,
      const SkBitmap& icon,
      const std::string& native_app_package_name,
      const std::string& referrer);

  bool IsForNativeApp() const { return GetType() == AppType::NATIVE; }

  void CreateJavaDelegate();
  bool InstallOrOpenNativeApp();
  void InstallWebApk(content::WebContents* web_contents);
  void InstallLegacyWebApp(content::WebContents* web_contents);

  // Called when the user accepts the banner to install the app. (Not called
  // when the "Open" button is pressed on the banner that is shown after
  // installation for WebAPK banners.)
  void SendBannerAccepted();
  base::android::ScopedJavaGlobalRef<jobject> java_delegate_;

  base::WeakPtr<AppBannerManager> weak_manager_;

  base::string16 app_title_;
  std::unique_ptr<ShortcutInfo> shortcut_info_;

  base::android::ScopedJavaGlobalRef<jobject> native_app_data_;

  const SkBitmap primary_icon_;
  const SkBitmap badge_icon_;

  std::string package_name_;
  std::string referrer_;

  AppType type_;
  WebappInstallSource install_source_;
  bool has_user_interaction_;

  DISALLOW_COPY_AND_ASSIGN(AppBannerUiDelegateAndroid);
};

}  // namespace banners

#endif  // CHROME_BROWSER_BANNERS_APP_BANNER_UI_DELEGATE_ANDROID_H_
