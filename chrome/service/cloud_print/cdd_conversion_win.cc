// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/cloud_print/cdd_conversion_win.h"

#include <stddef.h>

#include "base/memory/free_deleter.h"
#include "base/strings/string_number_conversions.h"
#include "components/cloud_devices/common/printer_description.h"
#include "printing/backend/win_helper.h"

namespace cloud_print {

bool IsValidCjt(const std::string& print_ticket_data) {
  cloud_devices::CloudDeviceDescription description;
  return description.InitFromString(print_ticket_data);
}

std::unique_ptr<DEVMODE, base::FreeDeleter> CjtToDevMode(
    const base::string16& printer_name,
    const std::string& print_ticket) {
  std::unique_ptr<DEVMODE, base::FreeDeleter> dev_mode;

  cloud_devices::CloudDeviceDescription description;
  if (!description.InitFromString(print_ticket))
    return dev_mode;

  using namespace cloud_devices::printer;
  printing::ScopedPrinterHandle printer;
  if (!printer.OpenPrinter(printer_name.c_str()))
    return dev_mode;

  {
    ColorTicketItem color;
    if (color.LoadFrom(description)) {
      bool is_color = color.value().type == STANDARD_COLOR;
      dev_mode = printing::CreateDevModeWithColor(printer.Get(), printer_name,
                                                  is_color);
    } else {
      dev_mode = printing::CreateDevMode(printer.Get(), NULL);
    }
  }

  if (!dev_mode)
    return dev_mode;

  ColorTicketItem color;
  DuplexTicketItem duplex;
  OrientationTicketItem orientation;
  MarginsTicketItem margins;
  DpiTicketItem dpi;
  FitToPageTicketItem fit_to_page;
  MediaTicketItem media;
  CopiesTicketItem copies;
  PageRangeTicketItem page_range;
  CollateTicketItem collate;
  ReverseTicketItem reverse;

  if (orientation.LoadFrom(description)) {
    dev_mode->dmFields |= DM_ORIENTATION;
    if (orientation.value() == LANDSCAPE) {
      dev_mode->dmOrientation = DMORIENT_LANDSCAPE;
    } else {
      dev_mode->dmOrientation = DMORIENT_PORTRAIT;
    }
  }

  if (color.LoadFrom(description)) {
    dev_mode->dmFields |= DM_COLOR;
    if (color.value().type == STANDARD_MONOCHROME) {
      dev_mode->dmColor = DMCOLOR_MONOCHROME;
    } else if (color.value().type == STANDARD_COLOR) {
      dev_mode->dmColor = DMCOLOR_COLOR;
    } else {
      NOTREACHED();
    }
  }

  if (duplex.LoadFrom(description)) {
    dev_mode->dmFields |= DM_DUPLEX;
    if (duplex.value() == NO_DUPLEX) {
      dev_mode->dmDuplex = DMDUP_SIMPLEX;
    } else if (duplex.value() == LONG_EDGE) {
      dev_mode->dmDuplex = DMDUP_VERTICAL;
    } else if (duplex.value() == SHORT_EDGE) {
      dev_mode->dmDuplex = DMDUP_HORIZONTAL;
    } else {
      NOTREACHED();
    }
  }

  if (copies.LoadFrom(description)) {
    dev_mode->dmFields |= DM_COPIES;
    dev_mode->dmCopies = copies.value();
  }

  if (dpi.LoadFrom(description)) {
    if (dpi.value().horizontal > 0) {
      dev_mode->dmFields |= DM_PRINTQUALITY;
      dev_mode->dmPrintQuality = dpi.value().horizontal;
    }
    if (dpi.value().vertical > 0) {
      dev_mode->dmFields |= DM_YRESOLUTION;
      dev_mode->dmYResolution = dpi.value().vertical;
    }
  }

  if (collate.LoadFrom(description)) {
    dev_mode->dmFields |= DM_COLLATE;
    dev_mode->dmCollate = (collate.value() ? DMCOLLATE_TRUE : DMCOLLATE_FALSE);
  }

  if (media.LoadFrom(description)) {
    static const size_t kFromUm = 100;  // Windows uses 0.1mm.
    int width = media.value().width_um / kFromUm;
    int height = media.value().height_um / kFromUm;
    unsigned id = 0;
    if (base::StringToUint(media.value().vendor_id, &id) && id) {
      dev_mode->dmFields |= DM_PAPERSIZE;
      dev_mode->dmPaperSize = static_cast<short>(id);
    } else if (width > 0 && height > 0) {
      dev_mode->dmFields |= DM_PAPERWIDTH;
      dev_mode->dmPaperWidth = width;
      dev_mode->dmFields |= DM_PAPERLENGTH;
      dev_mode->dmPaperLength = height;
    }
  }

  return printing::CreateDevMode(printer.Get(), dev_mode.get());
}

}  // namespace cloud_print
