// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/chrome_gtk_menu_subclasses.h"

G_DEFINE_TYPE(GtkCustomMenu, gtk_custom_menu, GTK_TYPE_MENU)

static void gtk_custom_menu_init(GtkCustomMenu* menu) {
}

static void gtk_custom_menu_class_init(GtkCustomMenuClass* klass) {
}

GtkWidget* gtk_custom_menu_new() {
  return GTK_WIDGET(g_object_new(gtk_custom_menu_get_type(), nullptr));
}

G_DEFINE_TYPE(GtkCustomMenuItem, gtk_custom_menu_item, GTK_TYPE_MENU_ITEM)

static void gtk_custom_menu_item_init(GtkCustomMenuItem* item) {
}

static void gtk_custom_menu_item_class_init(GtkCustomMenuItemClass* klass) {
}

GtkWidget* gtk_custom_menu_item_new() {
  return GTK_WIDGET(g_object_new(gtk_custom_menu_item_get_type(), nullptr));
}
