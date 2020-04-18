// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_CHROME_APP_ICON_H_
#define CHROME_BROWSER_EXTENSIONS_CHROME_APP_ICON_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "extensions/browser/extension_icon_image.h"
#include "ui/gfx/image/image_skia.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class Extension;
class ChromeAppIconDelegate;

// This represents how an extension app icon should finally look. As a base,
// extension icon is used and effects that depend on extension type, state and
// some external conditions are applied. Resulting image is sent via
// ChromeAppIconDelegate. Several updates are expected in case extension
// state or some external conditions are changed.
class ChromeAppIcon : public IconImage::Observer {
 public:
  using DestroyedCallback = base::OnceCallback<void(ChromeAppIcon*)>;

  ChromeAppIcon(ChromeAppIconDelegate* delegate,
                content::BrowserContext* browser_context,
                DestroyedCallback destroyed_callback,
                const std::string& app_id,
                int resource_size_in_dip);
  ~ChromeAppIcon() override;

  // Reloads icon.
  void Reload();

  // Returns true if the icon still refers to existing extension. Once extension
  // is disabled it is discarded from the icon.
  bool IsValid() const;

  // Re-applies app effects over the current extension icon and dispatches the
  // result via |delegate_|.
  void UpdateIcon();

  const gfx::ImageSkia& image_skia() const { return image_skia_; }
  const std::string& app_id() const { return app_id_; }
#if defined(OS_CHROMEOS)
  // Returns whether the icon is badged because it's an extension app that has
  // its Android analog installed.
  bool icon_is_badged() const { return icon_is_badged_; }
#endif

 private:
  const Extension* GetExtension();

  // IconImage::Observer:
  void OnExtensionIconImageChanged(IconImage* image) override;

  // Unowned pointers.
  ChromeAppIconDelegate* const delegate_;
  content::BrowserContext* const browser_context_;

  // Called when this instance of ChromeAppIcon is destroyed.
  DestroyedCallback destroyed_callback_;

  const std::string app_id_;

  // Contains current icon image. This is static image with applied effects and
  // it is updated each time when |icon_| is updated.
  gfx::ImageSkia image_skia_;

#if defined(OS_CHROMEOS)
  // Whether the icon got badged because it's an extension app that has its
  // Android analog installed.
  bool icon_is_badged_ = false;
#endif

  const int resource_size_in_dip_;

  std::unique_ptr<IconImage> icon_;

  DISALLOW_COPY_AND_ASSIGN(ChromeAppIcon);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_CHROME_APP_ICON_H_
