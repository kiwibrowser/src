// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_CUPS_PRINTERS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_CUPS_PRINTERS_HANDLER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/chromeos/printing/cups_printers_manager.h"
#include "chrome/browser/chromeos/printing/printer_configurer.h"
#include "chrome/browser/chromeos/printing/printer_event_tracker.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "chromeos/printing/ppd_provider.h"
#include "chromeos/printing/printer_configuration.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace base {
class ListValue;
}  // namespace base

class Profile;

namespace chromeos {

class PpdProvider;

namespace settings {

// Chrome OS CUPS printing settings page UI handler.
class CupsPrintersHandler : public ::settings::SettingsPageUIHandler,
                            public ui::SelectFileDialog::Listener,
                            public CupsPrintersManager::Observer {
 public:
  explicit CupsPrintersHandler(content::WebUI* webui);
  ~CupsPrintersHandler() override;

  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 private:
  // Gets all CUPS printers and return it to WebUI.
  void HandleGetCupsPrintersList(const base::ListValue* args);
  void HandleUpdateCupsPrinter(const base::ListValue* args);
  void HandleRemoveCupsPrinter(const base::ListValue* args);

  // For a CupsPrinterInfo in |args|, retrieves the relevant PrinterInfo object
  // using an IPP call to the printer.
  void HandleGetPrinterInfo(const base::ListValue* args);

  // Handles the callback for HandleGetPrinterInfo. |callback_id| is the
  // identifier to resolve the correct Promise. |success| indicates if the query
  // was successful. |make| is the detected printer manufacturer. |model| is the
  // detected model. |make_and_model| is the unparsed printer-make-and-model
  // string. |ipp_everywhere| indicates if configuration using the CUPS IPP
  // Everywhere driver should be attempted. If |success| is false, the values of
  // |make|, |model|, |make_and_model|, and |ipp_everywhere| are not specified.
  void OnAutoconfQueried(const std::string& callback_id,
                         bool success,
                         const std::string& make,
                         const std::string& model,
                         const std::string& make_and_model,
                         bool ipp_everywhere);

  // Handles the callback for HandleGetPrinterInfo for a discovered printer.
  void OnAutoconfQueriedDiscovered(std::unique_ptr<Printer> printer,
                                   bool success,
                                   const std::string& make,
                                   const std::string& model,
                                   const std::string& make_and_model,
                                   bool ipp_everywhere);

  void HandleAddCupsPrinter(const base::ListValue* args);

  // Handles the result of adding a printer which the user specified the
  // location of (i.e. a printer that was not 'discovered' automatically).
  void OnAddedSpecifiedPrinter(const Printer& printer,
                               PrinterSetupResult result);

  // Handles the result of failure to add a printer. |result_code| is used to
  // determine the reason for the failure.
  void OnAddPrinterError(PrinterSetupResult result_code);

  // Get a list of all manufacturers for which we have at least one model of
  // printer supported.  Takes one argument, the callback id for the result.
  // The callback will be invoked with {success: <boolean>, models:
  // <Array<string>>}.
  void HandleGetCupsPrinterManufacturers(const base::ListValue* args);

  // Given a manufacturer, get a list of all models of printers for which we can
  // get drivers.  Takes two arguments - the callback id and the manufacturer
  // name for which we want to list models.  The callback will be called with
  // {success: <boolean>, models: Array<string>}.
  void HandleGetCupsPrinterModels(const base::ListValue* args);

  void HandleSelectPPDFile(const base::ListValue* args);

  // PpdProvider callback handlers.
  void ResolveManufacturersDone(const std::string& js_callback,
                                PpdProvider::CallbackResultCode result_code,
                                const std::vector<std::string>& available);
  void ResolvePrintersDone(const std::string& manufacturer,
                           const std::string& js_callback,
                           PpdProvider::CallbackResultCode result_code,
                           const PpdProvider::ResolvedPrintersList& printers);

  void HandleStartDiscovery(const base::ListValue* args);
  void HandleStopDiscovery(const base::ListValue* args);

  // Logs printer set ups that are abandoned.
  void HandleSetUpCancel(const base::ListValue* args);

  // Given a printer id, find the corresponding ppdManufacturer and ppdModel.
  void HandleGetPrinterPpdManufacturerAndModel(const base::ListValue* args);
  void OnGetPrinterPpdManufacturerAndModel(
      const std::string& callback_id,
      PpdProvider::CallbackResultCode result_code,
      const std::string& manufacturer,
      const std::string& model);

  // Emits the updated discovered printer list after new printers are received.
  void UpdateDiscoveredPrinters();

  // Attempt to add a discovered printer.
  void HandleAddDiscoveredPrinter(const base::ListValue* args);

  // Post printer setup callback.
  void OnAddedDiscoveredPrinter(const Printer& printer,
                                PrinterSetupResult result_code);

  // Code common between the discovered and manual add printer code paths.
  void OnAddedPrinterCommon(const Printer& printer,
                            PrinterSetupResult result_code);

  // CupsPrintersManager::Observer override:
  void OnPrintersChanged(CupsPrintersManager::PrinterClass printer_class,
                         const std::vector<Printer>& printers) override;

  // ui::SelectFileDialog::Listener override:
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;

  // Used by FileSelected() in order to verify whether the beginning contents of
  // the selected file contain the magic number present in all PPD files. |path|
  // is used for display in the UI as this function calls back into javascript
  // with |path| as the result.
  void VerifyPpdContents(const base::FilePath& path,
                         const std::string& contents);

  // Fires the on-manually-add-discovered-printer event with the appropriate
  // parameters.  See https://crbug.com/835476
  void FireManuallyAddDiscoveredPrinter(const Printer& printer);

  Profile* profile_;

  // Discovery support.  discovery_active_ tracks whether or not the UI
  // currently wants updates about printer availability.  The two vectors track
  // the most recent list of printers in each class.
  bool discovery_active_ = false;
  std::vector<Printer> discovered_printers_;
  std::vector<Printer> automatic_printers_;

  // These must be initialized before printers_manager_, as they are
  // used by callbacks that may be issued immediately by printers_manager_.
  //
  // TODO(crbug/757887) - Remove this subtle initialization constraint.
  scoped_refptr<PpdProvider> ppd_provider_;
  std::unique_ptr<PrinterConfigurer> printer_configurer_;

  // Cached list of {printer name, PpdReference} pairs for each manufacturer
  // that has been resolved in the lifetime of this object.
  std::map<std::string, PpdProvider::ResolvedPrintersList> resolved_printers_;

  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  std::string webui_callback_id_;
  CupsPrintersManager* printers_manager_;

  ScopedObserver<CupsPrintersManager, CupsPrintersManager::Observer>
      printers_manager_observer_;

  base::WeakPtrFactory<CupsPrintersHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CupsPrintersHandler);
};

}  // namespace settings
}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_CUPS_PRINTERS_HANDLER_H_
