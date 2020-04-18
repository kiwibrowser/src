// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_CHROME_GTK_MENU_SUBCLASSES_H_
#define CHROME_BROWSER_UI_LIBGTKUI_CHROME_GTK_MENU_SUBCLASSES_H_

#include <gtk/gtk.h>

// This file declares two subclasses of Gtk's menu classes. We do this because
// when we were a GTK app proper, we had classes with the same names, and gtk
// theme authors started writing themes and styling chrome's menus by targeting
// these classes. We have to fetch our colors from these theme classes in
// specific because several newer GTK+2 themes are pixmap based and they
// specifically give real colors only to these classes.

G_BEGIN_DECLS

typedef struct _GtkCustomMenu GtkCustomMenu;
typedef struct _GtkCustomMenuClass GtkCustomMenuClass;

struct _GtkCustomMenu {
  GtkMenu menu;
};

struct _GtkCustomMenuClass {
  GtkMenuClass parent_class;
};

GtkWidget* gtk_custom_menu_new();

typedef struct _GtkCustomMenuItem GtkCustomMenuItem;
typedef struct _GtkCustomMenuItemClass GtkCustomMenuItemClass;

struct _GtkCustomMenuItem {
  GtkMenuItem menu_item;
};

struct _GtkCustomMenuItemClass {
  GtkMenuItemClass parent_class;
};

GtkWidget* gtk_custom_menu_item_new();

G_END_DECLS

#endif  // CHROME_BROWSER_UI_LIBGTKUI_CHROME_GTK_MENU_SUBCLASSES_H_
