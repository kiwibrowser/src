// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_CONFIGURER_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_CONFIGURER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "chromeos/printing/printer_configuration.h"

class Profile;

namespace chromeos {

// These values are written to logs.  New enum values can be added, but existing
// enums must never be renumbered or deleted and reused.
enum PrinterSetupResult {
  kFatalError = 0,                // Setup failed in an unrecognized way
  kSuccess = 1,                   // Printer set up successfully
  kPrinterUnreachable = 2,        // Could not reach printer
  kDbusError = 3,                 // Could not contact debugd
  kNativePrintersNotAllowed = 4,  // Tried adding/editing printers policy set
  kInvalidPrinterUpdate = 5,      // Tried updating printer with invalid values
  kComponentUnavailable = 6,      // Could not install component
  // Space left for additional errors

  // PPD errors
  kPpdTooLarge = 10,       // PPD exceeds size limit
  kInvalidPpd = 11,        // PPD rejected by cupstestppd
  kPpdNotFound = 12,       // Could not find PPD
  kPpdUnretrievable = 13,  // Could not download PPD
  kMaxValue                // Maximum value for histograms
};

using PrinterSetupCallback = base::OnceCallback<void(PrinterSetupResult)>;

// Configures printers by retrieving PPDs and registering the printer with CUPS.
// Class must be constructed and used on the UI thread.
class PrinterConfigurer {
 public:
  static std::unique_ptr<PrinterConfigurer> Create(Profile* profile);

  PrinterConfigurer(const PrinterConfigurer&) = delete;
  PrinterConfigurer& operator=(const PrinterConfigurer&) = delete;

  virtual ~PrinterConfigurer() = default;

  // Set up |printer| retrieving the appropriate PPD and registering the printer
  // with CUPS.  |callback| is called with the result of the operation.  This
  // method must be called on the UI thread and will run |callback| on the
  // UI thread.
  virtual void SetUpPrinter(const Printer& printer,
                            PrinterSetupCallback callback) = 0;

  // Return an opaque fingerprint of the fields used to set up a printer with
  // CUPS.  The idea here is that if this fingerprint changes for a printer, we
  // need to reconfigure CUPS.  This fingerprint is not guaranteed to be stable
  // across reboots.
  static std::string SetupFingerprint(const Printer& printer);

 protected:
  PrinterConfigurer() = default;
};

// Stream operator for ease of logging |result|.
std::ostream& operator<<(std::ostream& out, const PrinterSetupResult& result);

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_PRINTER_CONFIGURER_H_
