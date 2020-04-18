// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_LAUNCHER_SEARCH_LAUNCHER_SEARCH_ICON_IMAGE_LOADER_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_LAUNCHER_SEARCH_LAUNCHER_SEARCH_ICON_IMAGE_LOADER_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/chromeos/launcher_search_provider/error_reporter.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/common/extension.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "url/gurl.h"

namespace app_list {

// Loads icons of launcher search results.
class LauncherSearchIconImageLoader
    : public base::RefCounted<LauncherSearchIconImageLoader> {
 public:
  class Observer {
   public:
    // Called when icon image is changed. To obtain the new image, call
    // GetIconImage method.
    virtual void OnIconImageChanged(
        LauncherSearchIconImageLoader* image_loader) = 0;

    // Called when badge icon image is changed. To obtain the new image, call
    // GetBadgeIconImage method.
    virtual void OnBadgeIconImageChanged(
        LauncherSearchIconImageLoader* image_loader) = 0;
  };

  // If |custom_icon_url| is empty, uses the extension icon.
  LauncherSearchIconImageLoader(
      const GURL& custom_icon_url,
      Profile* profile,
      const extensions::Extension* extension,
      const int icon_dimension,
      std::unique_ptr<chromeos::launcher_search_provider::ErrorReporter>
          error_reporter);

  // Load resources caller must call this function to generate icon image.
  void LoadResources();

  // Adds |observer| to listen icon image changed event. To get fresh icon
  // image, you need to add observer before you call GetIconImage.
  void AddObserver(Observer* observer);

  // Removes |observer|.
  void RemoveObserver(Observer* observer);

  // Returns icon image.
  const gfx::ImageSkia& GetIconImage() const;

  // Returns badge icon image.
  const gfx::ImageSkia& GetBadgeIconImage() const;

 protected:
  // Ref counted class.
  virtual ~LauncherSearchIconImageLoader();

  // Loads |extension| icon and returns it as sync if possible. When it loads
  // icon as async, it calls OnExtensionIconImageChanged.
  virtual const gfx::ImageSkia& LoadExtensionIcon() = 0;

  // Loads |icon_url_| as async. When it loads an image, OnCustomIconLoaded will
  // be called with an image. When it fails to load an image, OnCustomIconLoaded
  // will be called with an empty image.
  virtual void LoadIconResourceFromExtension() = 0;

  // Called when extension icon image is changed.
  void OnExtensionIconChanged(const gfx::ImageSkia& image);

  // Called when custom icon image is loaded.
  void OnCustomIconLoaded(const gfx::ImageSkia& image);

  Profile* profile_;
  const extensions::Extension* extension_;
  const GURL icon_url_;
  const gfx::Size icon_size_;

 private:
  friend class base::RefCounted<LauncherSearchIconImageLoader>;

  // Notifies to observers.
  void NotifyObserversIconImageChange();
  void NotifyObserversBadgeIconImageChange();

  // Returns truncated icon url. Since max_size includes trailing ..., it should
  // be larger than 3.
  std::string GetTruncatedIconUrl(const uint32_t max_size);

  std::unique_ptr<chromeos::launcher_search_provider::ErrorReporter>
      error_reporter_;

  gfx::ImageSkia extension_icon_image_;
  gfx::ImageSkia custom_icon_image_;

  std::set<Observer*> observers_;

  DISALLOW_COPY_AND_ASSIGN(LauncherSearchIconImageLoader);
};

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_LAUNCHER_SEARCH_LAUNCHER_SEARCH_ICON_IMAGE_LOADER_H_
