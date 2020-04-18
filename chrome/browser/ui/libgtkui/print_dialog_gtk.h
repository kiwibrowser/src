// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_PRINT_DIALOG_GTK_H_
#define CHROME_BROWSER_UI_LIBGTKUI_PRINT_DIALOG_GTK_H_

#include <gtk/gtk.h>
#include <gtk/gtkunixprint.h>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner_helpers.h"
#include "chrome/browser/ui/libgtkui/gtk_signal.h"
#include "content/public/browser/browser_thread.h"
#include "printing/print_dialog_gtk_interface.h"
#include "printing/printing_context_linux.h"
#include "ui/aura/window_observer.h"

namespace printing {
class MetafilePlayer;
class PrintSettings;
}

using printing::PrintingContextLinux;

// Needs to be freed on the UI thread to clean up its GTK members variables.
class PrintDialogGtk2 : public printing::PrintDialogGtkInterface,
                        public base::RefCountedThreadSafe<
                            PrintDialogGtk2,
                            content::BrowserThread::DeleteOnUIThread>,
                        public aura::WindowObserver {
 public:
  // Creates and returns a print dialog.
  static printing::PrintDialogGtkInterface* CreatePrintDialog(
      PrintingContextLinux* context);

  // printing::PrintDialogGtkInterface implementation.
  void UseDefaultSettings() override;
  void UpdateSettings(printing::PrintSettings* settings) override;
  void ShowDialog(
      gfx::NativeView parent_view,
      bool has_selection,
      PrintingContextLinux::PrintSettingsCallback callback) override;
  void PrintDocument(const printing::MetafilePlayer& metafile,
                     const base::string16& document_name) override;
  void AddRefToDialog() override;
  void ReleaseDialog() override;

  // Handles print job response.
  void OnJobCompleted(GtkPrintJob* print_job, const GError* error);

 private:
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;
  friend class base::DeleteHelper<PrintDialogGtk2>;

  explicit PrintDialogGtk2(PrintingContextLinux* context);
  ~PrintDialogGtk2() override;

  // Handles dialog response.
  CHROMEGTK_CALLBACK_1(PrintDialogGtk2, void, OnResponse, int);

  // Prints document named |document_name|.
  void SendDocumentToPrinter(const base::string16& document_name);

  // Helper function for initializing |context_|'s PrintSettings with a given
  // |settings|.
  void InitPrintSettings(printing::PrintSettings* settings);

  // aura::WindowObserver implementation.
  void OnWindowDestroying(aura::Window* window) override;

  // Printing dialog callback.
  PrintingContextLinux::PrintSettingsCallback callback_;
  PrintingContextLinux* context_;

  // Print dialog settings. PrintDialogGtk2 owns |dialog_| and holds references
  // to the other objects.
  GtkWidget* dialog_;
  GtkPrintSettings* gtk_settings_;
  GtkPageSetup* page_setup_;
  GtkPrinter* printer_;

  base::FilePath path_to_pdf_;

  DISALLOW_COPY_AND_ASSIGN(PrintDialogGtk2);
};

#endif  // CHROME_BROWSER_UI_LIBGTKUI_PRINT_DIALOG_GTK_H_
