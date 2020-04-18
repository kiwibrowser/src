// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_PRINTING_GTK_UTIL_H_
#define CHROME_BROWSER_UI_LIBGTKUI_PRINTING_GTK_UTIL_H_

#include "ui/gfx/geometry/size.h"

namespace printing {
class PrintingContextLinux;
class PrintSettings;
}

typedef struct _GtkPrintSettings GtkPrintSettings;
typedef struct _GtkPageSetup GtkPageSetup;

// Obtains the paper size through Gtk.
gfx::Size GetPdfPaperSizeDeviceUnitsGtk(
    printing::PrintingContextLinux* context);

// Initializes a PrintSettings object from the provided Gtk printer objects.
void InitPrintSettingsGtk(GtkPrintSettings* settings,
                          GtkPageSetup* page_setup,
                          printing::PrintSettings* print_settings);

#endif  // CHROME_BROWSER_UI_LIBGTKUI_PRINTING_GTK_UTIL_H_
