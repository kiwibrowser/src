// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/chrome_gtk_frame.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

G_BEGIN_DECLS

// MetaFrames declaration
G_DEFINE_TYPE(MetaFrames, meta_frames, GTK_TYPE_WINDOW)

static void meta_frames_class_init(MetaFramesClass* frames_class) {
  // Noop since we don't declare anything.
}

static void meta_frames_init(MetaFrames* button) {
}


// ChromeGtkFrame declaration
G_DEFINE_TYPE(ChromeGtkFrame, chrome_gtk_frame, meta_frames_get_type())

static void chrome_gtk_frame_class_init(ChromeGtkFrameClass* frame_class) {
  GtkWidgetClass* widget_class = reinterpret_cast<GtkWidgetClass*>(frame_class);

  // Frame tints:
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "frame-color",
          "Frame Color",
          "The color that the chrome frame will be. (If unspecified, "
            " Chrome will take ChromeGtkFrame::bg[SELECTED] and slightly darken"
            " it.)",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "inactive-frame-color",
          "Inactive Frame Color",
          "The color that the inactive chrome frame will be. (If"
            " unspecified, Chrome will take ChromeGtkFrame::bg[INSENSITIVE]"
            " and slightly darken it.)",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "incognito-frame-color",
          "Incognito Frame Color",
          "The color that the incognito frame will be. (If unspecified,"
            " Chrome will take the frame color and tint it by Chrome's default"
            " incognito tint.)",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "incognito-inactive-frame-color",
          "Incognito Inactive Frame Color",
          "The color that the inactive incognito frame will be. (If"
            " unspecified, Chrome will take the frame color and tint it by"
            " Chrome's default incognito tint.)",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));

  // Frame gradient control:
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_int(
          "frame-gradient-size",
          "Chrome Frame Gradient Size",
          "The size of the gradient on top of the frame image. Specify 0 to"
            " make the frame a solid color.",
          0,      // 0 disables the gradient
          128,    // The frame image is only up to 128 pixels tall.
          16,     // By default, gradients are 16 pixels high.
          G_PARAM_READABLE));
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "frame-gradient-color",
          "Frame Gradient Color",
          "The top color of the chrome frame gradient. (If unspecified,"
            " chrome will create a lighter tint of frame-color",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "inactive-frame-gradient-color",
          "Inactive Frame Gradient Color",
          "The top color of the inactive chrome frame gradient. (If"
            " unspecified, chrome will create a lighter tint of frame-color",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "incognito-frame-gradient-color",
          "Incognito Frame Gradient Color",
          "The top color of the incognito chrome frame gradient. (If"
            " unspecified, chrome will create a lighter tint of frame-color",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "incognito-inactive-frame-gradient-color",
          "Incognito Inactive Frame Gradient Color",
          "The top color of the incognito inactive chrome frame gradient. (If"
            " unspecified, chrome will create a lighter tint of frame-color",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));

  // Scrollbar color properties:
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "scrollbar-slider-prelight-color",
          "Scrollbar Slider Prelight Color",
          "The color applied to the mouse is above the tab",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "scrollbar-slider-normal-color",
          "Scrollbar Slider Normal Color",
          "The color applied to the slider normally",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));
  gtk_widget_class_install_style_property(
      widget_class,
      g_param_spec_boxed(
          "scrollbar-trough-color",
          "Scrollbar Trough Color",
          "The background color of the slider track",
          GDK_TYPE_COLOR,
          G_PARAM_READABLE));
}

static void chrome_gtk_frame_init(ChromeGtkFrame* frame) {
}

GtkWidget* chrome_gtk_frame_new(void) {
  return GTK_WIDGET(g_object_new(chrome_gtk_frame_get_type(), "type",
                                 GTK_WINDOW_TOPLEVEL, nullptr));
}

G_END_DECLS
