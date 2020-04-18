// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_PRINTING_PRINTER_TRANSLATOR_H_
#define CHROMEOS_PRINTING_PRINTER_TRANSLATOR_H_

#include <memory>

#include "chromeos/chromeos_export.h"
#include "chromeos/printing/printer_configuration.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {

CHROMEOS_EXPORT extern const char kPrinterId[];

// Returns a new printer populated with the fields from |pref|.  Processes
// dictionaries from policy i.e. cPanel.
CHROMEOS_EXPORT std::unique_ptr<Printer> RecommendedPrinterToPrinter(
    const base::DictionaryValue& pref);

// Returns a JSON representation of |printer| as a CupsPrinterInfo. If the
// printer uri cannot be parsed, the relevant fields are populated with default
// values. CupsPrinterInfo is defined in cups_printers_browser_proxy.js.
CHROMEOS_EXPORT std::unique_ptr<base::DictionaryValue> GetCupsPrinterInfo(
    const Printer& printer);

}  // namespace chromeos

#endif  // CHROMEOS_PRINTING_PRINTER_TRANSLATOR_H_
