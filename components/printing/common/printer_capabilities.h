// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PRINTING_COMMON_PRINTER_CAPABILITIES_H_
#define COMPONENTS_PRINTING_COMMON_PRINTER_CAPABILITIES_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/scoped_refptr.h"

namespace base {
class DictionaryValue;
}

namespace printing {

class PrintBackend;
struct PrinterBasicInfo;

extern const char kPrinter[];

// Extracts the printer display name and description from the
// appropriate fields in |printer| for the platform.
std::pair<std::string, std::string> GetPrinterNameAndDescription(
    const PrinterBasicInfo& printer);

// Returns the JSON representing printer capabilities for the device registered
// as |device_name| in the PrinterBackend.  The returned dictionary is suitable
// for passage to the WebUI. The settings are obtained using |print_backend| if
// it is provided. If |print_backend| is null, uses a new PrintBackend instance
// with default settings.
std::unique_ptr<base::DictionaryValue> GetSettingsOnBlockingPool(
    const std::string& device_name,
    const PrinterBasicInfo& basic_info,
    scoped_refptr<PrintBackend> print_backend);

}  // namespace printing

#endif  // COMPONENTS_PRINTING_COMMON_PRINTER_CAPABILITIES_H_
