// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_GTK_STATUS_ICON_H_
#define CHROME_BROWSER_UI_LIBGTKUI_GTK_STATUS_ICON_H_

#include <memory>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/libgtkui/gtk_signal.h"
#include "ui/base/glib/glib_integers.h"
#include "ui/base/glib/glib_signal.h"
#include "ui/views/linux_ui/status_icon_linux.h"

typedef struct _GtkStatusIcon GtkStatusIcon;

namespace gfx {
class ImageSkia;
}

namespace ui {
class MenuModel;
}

namespace libgtkui {
class AppIndicatorIconMenu;

// Status icon implementation which uses the system tray X11 spec (via
// GtkStatusIcon).
class Gtk2StatusIcon : public views::StatusIconLinux {
 public:
  Gtk2StatusIcon(const gfx::ImageSkia& image, const base::string16& tool_tip);
  ~Gtk2StatusIcon() override;

  // Overridden from views::StatusIconLinux:
  void SetImage(const gfx::ImageSkia& image) override;
  void SetToolTip(const base::string16& tool_tip) override;
  void UpdatePlatformContextMenu(ui::MenuModel* menu) override;
  void RefreshPlatformContextMenu() override;

 private:
  CHROMEG_CALLBACK_0(Gtk2StatusIcon, void, OnClick, GtkStatusIcon*);

  CHROMEG_CALLBACK_2(Gtk2StatusIcon,
                     void,
                     OnContextMenuRequested,
                     GtkStatusIcon*,
                     guint,
                     guint);

  GtkStatusIcon* gtk_status_icon_;

  std::unique_ptr<AppIndicatorIconMenu> menu_;

  DISALLOW_COPY_AND_ASSIGN(Gtk2StatusIcon);
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_GTK_STATUS_ICON_H_
