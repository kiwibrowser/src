// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_CHROME_GTK_FRAME_H_
#define CHROME_BROWSER_UI_LIBGTKUI_CHROME_GTK_FRAME_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

// This file declares two subclasses of GtkWindow for easier gtk+ theme
// integration.
//
// The first is "MetaFrames," which is (was?) the name of a gobject class in
// the metacity window manager. To actually get at those values, we need to
// have an object whose gobject class name string matches the definitions in
// the gtkrc file. MetaFrames derives from GtkWindow.
//
// Metaframes can not be instantiated. It has no constructor; instantiate
// ChromeGtkFrame instead.
typedef struct _MetaFrames       MetaFrames;
typedef struct _MetaFramesClass  MetaFramesClass;

struct _MetaFrames {
  GtkWindow window;
};

struct _MetaFramesClass {
  GtkWindowClass parent_class;
};


// The second is ChromeGtkFrame, which defines a number of optional style
// properties so theme authors can control how chromium appears in gtk-theme
// mode.  It derives from MetaFrames in chrome so older themes that declare a
// MetaFrames theme will still work. New themes should target this class.
typedef struct _ChromeGtkFrame       ChromeGtkFrame;
typedef struct _ChromeGtkFrameClass  ChromeGtkFrameClass;

struct _ChromeGtkFrame {
  MetaFrames frames;
};

struct _ChromeGtkFrameClass {
  MetaFramesClass frames_class;
};

// Creates a GtkWindow object the the class name "ChromeGtkFrame".
GtkWidget* chrome_gtk_frame_new();

G_END_DECLS

#endif  // CHROME_BROWSER_UI_LIBGTKUI_CHROME_GTK_FRAME_H_
