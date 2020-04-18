// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_HELPER_H_
#define CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_HELPER_H_

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/installable/installable_metrics.h"
#include "chrome/common/web_application_info.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "third_party/blink/public/common/manifest/manifest.h"

class ExtensionService;
class FaviconDownloader;
struct InstallableData;
class InstallableManager;
class Profile;
class SkBitmap;

namespace content {
class BrowserContext;
class WebContents;
}

namespace extensions {
class CrxInstaller;
class Extension;

// A helper class for creating bookmark apps from a WebContents.
class BookmarkAppHelper : public content::NotificationObserver {
 public:
  enum class ForInstallableSite {
    kYes,
    kNo,
    kUnknown,
  };

  struct BitmapAndSource {
    BitmapAndSource();
    BitmapAndSource(const GURL& source_url_p, const SkBitmap& bitmap_p);
    ~BitmapAndSource();

    GURL source_url;
    SkBitmap bitmap;
  };

  typedef base::Callback<void(const Extension*, const WebApplicationInfo&)>
      CreateBookmarkAppCallback;

  // This helper class will create a bookmark app out of |web_app_info| and
  // install it to |service|. Icons will be downloaded from the URLs in
  // |web_app_info.icons| using |contents| if |contents| is not NULL.
  // All existing icons from WebApplicationInfo will also be used. The user
  // will then be prompted to edit the creation information via a bubble and
  // will have a chance to cancel the operation.
  // |install_source| indicates how the installation was triggered.
  BookmarkAppHelper(Profile* profile,
                    WebApplicationInfo web_app_info,
                    content::WebContents* contents,
                    WebappInstallSource install_source);
  ~BookmarkAppHelper() override;

  // Update the given WebApplicationInfo with information from the manifest.
  static void UpdateWebAppInfoFromManifest(const blink::Manifest& manifest,
                                           WebApplicationInfo* web_app_info,
                                           ForInstallableSite installable_site);

  // This finds the closest not-smaller bitmap in |bitmaps| for each size in
  // |sizes| and resizes it to that size. This returns a map of sizes to bitmaps
  // which contains only bitmaps of a size in |sizes| and at most one bitmap of
  // each size.
  static std::map<int, BitmapAndSource> ConstrainBitmapsToSizes(
      const std::vector<BitmapAndSource>& bitmaps,
      const std::set<int>& sizes);

  // Adds a square container icon of |output_size| and 2 * |output_size| pixels
  // to |bitmaps| by drawing the given |letter| into a rounded background of
  // |color|. For each size, if an icon of the requested size already exists in
  // |bitmaps|, nothing will happen.
  static void GenerateIcon(std::map<int, BitmapAndSource>* bitmaps,
                           int output_size,
                           SkColor color,
                           char letter);

  // Returns true if a bookmark or hosted app from a given URL is already
  // installed and enabled.
  static bool BookmarkOrHostedAppInstalled(
      content::BrowserContext* browser_context, const GURL& url);

  // Resize icons to the accepted sizes, and generate any that are missing. Does
  // not update |web_app_info| except to update |generated_icon_color|.
  static std::map<int, BitmapAndSource> ResizeIconsAndGenerateMissing(
      std::vector<BitmapAndSource> icons,
      std::set<int> sizes_to_generate,
      WebApplicationInfo* web_app_info);

  // It is important that the linked app information in any extension that
  // gets created from sync matches the linked app information that came from
  // sync. If there are any changes, they will be synced back to other devices
  // and could potentially create a never ending sync cycle.
  // This function updates |web_app_info| with the image data of any icon from
  // |bitmap_map| that has a URL and size matching that in |web_app_info|, as
  // well as adding any new images from |bitmap_map| that have no URL.
  static void UpdateWebAppIconsWithoutChangingLinks(
      std::map<int, BookmarkAppHelper::BitmapAndSource> bitmap_map,
      WebApplicationInfo* web_app_info);

  // Begins the asynchronous bookmark app creation.
  void Create(const CreateBookmarkAppCallback& callback);

 protected:
  // Protected methods for testing.

  // Called by the InstallableManager when the installability check is
  // completed.
  void OnDidPerformInstallableCheck(const InstallableData& data);

  // Performs post icon download tasks including installing the bookmark app.
  virtual void OnIconsDownloaded(
      bool success,
      const std::map<GURL, std::vector<SkBitmap>>& bitmaps);

  // Downloads icons from the given WebApplicationInfo using the given
  // WebContents.
  std::unique_ptr<FaviconDownloader> favicon_downloader_;

 private:
  FRIEND_TEST_ALL_PREFIXES(BookmarkAppHelperTest,
                           CreateWindowedPWAIntoAppWindow);

  // Called after the bubble has been shown, and the user has either accepted or
  // the dialog was dismissed.
  void OnBubbleCompleted(bool user_accepted,
                         const WebApplicationInfo& web_app_info);

  // Called when the installation of the app is complete to perform the final
  // installation steps.
  void FinishInstallation(const Extension* extension);

  // Overridden from content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // The profile that the bookmark app is being added to.
  Profile* profile_;

  // The web contents that the bookmark app is being created for.
  content::WebContents* contents_;

  // The WebApplicationInfo that the bookmark app is being created for.
  WebApplicationInfo web_app_info_;

  // Called on app creation or failure.
  CreateBookmarkAppCallback callback_;

  // Used to install the created bookmark app.
  scoped_refptr<extensions::CrxInstaller> crx_installer_;

  content::NotificationRegistrar registrar_;

  InstallableManager* installable_manager_;

  ForInstallableSite for_installable_site_ = ForInstallableSite::kUnknown;

  // The mechanism via which the app creation was triggered.
  WebappInstallSource install_source_;

  // With fast tab unloading enabled, shutting down can cause BookmarkAppHelper
  // to be destroyed before the bookmark creation bubble. Use weak pointers to
  // prevent a heap-use-after free in this instance (https://crbug.com/534994).
  base::WeakPtrFactory<BookmarkAppHelper> weak_factory_;
};

// Creates or updates a bookmark app from the given |web_app_info|. Icons will
// be downloaded from the icon URLs provided in |web_app_info|.
void CreateOrUpdateBookmarkApp(ExtensionService* service,
                               WebApplicationInfo* web_app_info);

// Returns whether the given |url| is a valid bookmark app url.
bool IsValidBookmarkAppUrl(const GURL& url);

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_HELPER_H_
