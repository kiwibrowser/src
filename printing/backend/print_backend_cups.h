// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_BACKEND_PRINT_BACKEND_CUPS_H_
#define PRINTING_BACKEND_PRINT_BACKEND_CUPS_H_

#include <string>

#include "base/files/file_util.h"
#include "printing/backend/cups_helper.h"
#include "printing/backend/print_backend.h"
#include "url/gurl.h"

namespace printing {

class PrintBackendCUPS : public PrintBackend {
 public:
  PrintBackendCUPS(const GURL& print_server_url,
                   http_encryption_t encryption,
                   bool blocking);

 private:
  ~PrintBackendCUPS() override {}

  // PrintBackend implementation.
  bool EnumeratePrinters(PrinterList* printer_list) override;
  std::string GetDefaultPrinterName() override;
  bool GetPrinterBasicInfo(const std::string& printer_name,
                           PrinterBasicInfo* printer_info) override;
  bool GetPrinterSemanticCapsAndDefaults(
      const std::string& printer_name,
      PrinterSemanticCapsAndDefaults* printer_info) override;
  bool GetPrinterCapsAndDefaults(const std::string& printer_name,
                                 PrinterCapsAndDefaults* printer_info) override;
  std::string GetPrinterDriverInfo(const std::string& printer_name) override;
  bool IsValidPrinter(const std::string& printer_name) override;

  // The following functions are wrappers around corresponding CUPS functions.
  // <functions>2() are called when print server is specified, and plain version
  // in another case. There is an issue specifying CUPS_HTTP_DEFAULT in the
  // functions>2(), it does not work in CUPS prior to 1.4.
  int GetDests(cups_dest_t** dests);
  base::FilePath GetPPD(const char* name);

  // Wrapper around cupsGetNamedDest(). Returned result should be freed with
  // cupsFreeDests().
  cups_dest_t* GetNamedDest(const std::string& printer_name);

  GURL print_server_url_;
  http_encryption_t cups_encryption_;
  bool blocking_;
};

}  // namespace printing

#endif  // PRINTING_BACKEND_PRINT_BACKEND_CUPS_H_
