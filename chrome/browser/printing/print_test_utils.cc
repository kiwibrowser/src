// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_test_utils.h"

#include <string>
#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "chrome/browser/ui/webui/print_preview/print_preview_handler.h"

namespace printing {

const char kDummyPrinterName[] = "DefaultPrinter";

base::Value GetPrintTicket(PrinterType type, bool cloud) {
  bool is_privet_printer = !cloud && type == kPrivetPrinter;
  bool is_extension_printer = !cloud && type == kExtensionPrinter;

  base::Value ticket(base::Value::Type::DICTIONARY);

  // Letter
  base::Value media_size(base::Value::Type::DICTIONARY);
  media_size.SetKey(kSettingMediaSizeIsDefault, base::Value(true));
  media_size.SetKey(kSettingMediaSizeWidthMicrons, base::Value(215900));
  media_size.SetKey(kSettingMediaSizeHeightMicrons, base::Value(279400));
  ticket.SetKey(kSettingMediaSize, std::move(media_size));

  ticket.SetKey(kSettingPreviewPageCount, base::Value(1));
  ticket.SetKey(kSettingLandscape, base::Value(false));
  ticket.SetKey(kSettingColor, base::Value(2));  // color printing
  ticket.SetKey(kSettingHeaderFooterEnabled, base::Value(false));
  ticket.SetKey(kSettingMarginsType, base::Value(0));  // default margins
  ticket.SetKey(kSettingDuplexMode, base::Value(1));   // LONG_EDGE
  ticket.SetKey(kSettingCopies, base::Value(1));
  ticket.SetKey(kSettingCollate, base::Value(true));
  ticket.SetKey(kSettingShouldPrintBackgrounds, base::Value(false));
  ticket.SetKey(kSettingShouldPrintSelectionOnly, base::Value(false));
  ticket.SetKey(kSettingPreviewModifiable, base::Value(false));
  ticket.SetKey(kSettingPrintToPDF, base::Value(!cloud && type == kPdfPrinter));
  ticket.SetKey(kSettingCloudPrintDialog, base::Value(cloud));
  ticket.SetKey(kSettingPrintWithPrivet, base::Value(is_privet_printer));
  ticket.SetKey(kSettingPrintWithExtension, base::Value(is_extension_printer));
  ticket.SetKey(kSettingRasterizePdf, base::Value(false));
  ticket.SetKey(kSettingScaleFactor, base::Value(100));
  ticket.SetKey(kSettingPagesPerSheet, base::Value(1));
  ticket.SetKey(kSettingDpiHorizontal, base::Value(kTestPrinterDpi));
  ticket.SetKey(kSettingDpiVertical, base::Value(kTestPrinterDpi));
  ticket.SetKey(kSettingDeviceName, base::Value(kDummyPrinterName));
  ticket.SetKey(kSettingFitToPageEnabled, base::Value(true));
  ticket.SetKey(kSettingPageWidth, base::Value(215900));
  ticket.SetKey(kSettingPageHeight, base::Value(279400));
  ticket.SetKey(kSettingShowSystemDialog, base::Value(false));

  if (cloud)
    ticket.SetKey(kSettingCloudPrintId, base::Value(kDummyPrinterName));

  if (is_privet_printer || is_extension_printer) {
    base::Value capabilities(base::Value::Type::DICTIONARY);
    capabilities.SetKey("duplex", base::Value(true));  // non-empty
    std::string caps_string;
    base::JSONWriter::Write(capabilities, &caps_string);
    ticket.SetKey(kSettingCapabilities, base::Value(caps_string));
    base::Value print_ticket(base::Value::Type::DICTIONARY);
    print_ticket.SetKey("version", base::Value("1.0"));
    print_ticket.SetKey("print", base::Value());
    std::string ticket_string;
    base::JSONWriter::Write(print_ticket, &ticket_string);
    ticket.SetKey(kSettingTicket, base::Value(ticket_string));
  }

  return ticket;
}

}  // namespace printing
