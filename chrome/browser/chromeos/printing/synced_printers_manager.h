// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_SYNCED_PRINTERS_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_SYNCED_PRINTERS_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/chromeos/printing/printers_sync_bridge.h"
#include "chromeos/printing/printer_configuration.h"
#include "chromeos/printing/printer_translator.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace chromeos {

// Manages information about synced local printers classes (CONFIGURED
// and ENTERPRISE).  Provides an interface to a user's printers and
// printers provided by policy.  User printers are backed by the
// PrintersSyncBridge.
//
// This class is thread-safe.
class SyncedPrintersManager : public KeyedService {
 public:
  class Observer {
   public:
    virtual void OnConfiguredPrintersChanged(
        const std::vector<Printer>& printers) = 0;
    virtual void OnEnterprisePrintersChanged(
        const std::vector<Printer>& printers) = 0;
  };

  static std::unique_ptr<SyncedPrintersManager> Create(
      Profile* profile,
      std::unique_ptr<PrintersSyncBridge> sync_bridge);
  ~SyncedPrintersManager() override = default;

  // Register the printing preferences with the |registry|.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns the printers that are saved in preferences.
  virtual std::vector<Printer> GetConfiguredPrinters() const = 0;

  // Returns printers from enterprise policy.
  virtual std::vector<Printer> GetEnterprisePrinters() const = 0;

  // Returns the printer with id |printer_id|, or nullptr if no such printer
  // exists.  Searches both Configured and Enterprise printers.
  virtual std::unique_ptr<Printer> GetPrinter(
      const std::string& printer_id) const = 0;

  // Adds or updates a printer in profile preferences.  The |printer| is
  // identified by its id field.  Those with an empty id are treated as new
  // printers.
  virtual void UpdateConfiguredPrinter(const Printer& printer) = 0;

  // Remove printer from preferences with the id |printer_id|.  Returns true if
  // the printer was successfully removed.
  virtual bool RemoveConfiguredPrinter(const std::string& printer_id) = 0;

  // Attach |observer| for notification of events.  |observer| is expected to
  // live on the same thread (UI) as this object.  OnPrinter* methods are
  // invoked inline so calling RegisterPrinter in response to OnPrinterAdded is
  // forbidden.
  virtual void AddObserver(SyncedPrintersManager::Observer* observer) = 0;

  // Remove |observer| so that it no longer receives notifications.  After the
  // completion of this method, the |observer| can be safely destroyed.
  virtual void RemoveObserver(SyncedPrintersManager::Observer* observer) = 0;

  // Returns a ModelTypeSyncBridge for the sync client.
  virtual PrintersSyncBridge* GetSyncBridge() = 0;

  // Registers that the printer was installed in CUPS.  If |printer| is not an
  // already known printer (either a configured printer or an enterprise
  // printer), this will have the side effect of saving |printer| as a
  // configured printer.
  virtual void PrinterInstalled(const Printer& printer) = 0;

  // Returns true if |printer| is currently installed in CUPS.
  virtual bool IsConfigurationCurrent(const Printer& printer) const = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_SYNCED_PRINTERS_MANAGER_H_
