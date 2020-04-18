// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/printing/common/cloud_print_cdd_conversion.h"

#include <stddef.h>

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "components/cloud_devices/common/printer_description.h"
#include "printing/backend/print_backend.h"

namespace cloud_print {

std::unique_ptr<base::DictionaryValue> PrinterSemanticCapsAndDefaultsToCdd(
    const printing::PrinterSemanticCapsAndDefaults& semantic_info) {
  using namespace cloud_devices::printer;
  cloud_devices::CloudDeviceDescription description;

  ContentTypesCapability content_types;
  content_types.AddOption("application/pdf");
  content_types.SaveTo(&description);

  if (semantic_info.collate_capable) {
    CollateCapability collate;
    collate.set_default_value(semantic_info.collate_default);
    collate.SaveTo(&description);
  }

  if (semantic_info.copies_capable) {
    CopiesCapability copies;
    copies.SaveTo(&description);
  }

  if (semantic_info.duplex_capable) {
    DuplexCapability duplex;
    duplex.AddDefaultOption(NO_DUPLEX,
                            semantic_info.duplex_default == printing::SIMPLEX);
    duplex.AddDefaultOption(
        LONG_EDGE, semantic_info.duplex_default == printing::LONG_EDGE);
    duplex.AddDefaultOption(
        SHORT_EDGE, semantic_info.duplex_default == printing::SHORT_EDGE);
    duplex.SaveTo(&description);
  }

  ColorCapability color;
  if (semantic_info.color_default || semantic_info.color_changeable) {
    Color standard_color(STANDARD_COLOR);
    standard_color.vendor_id = base::IntToString(semantic_info.color_model);
    color.AddDefaultOption(standard_color, semantic_info.color_default);
  }
  if (!semantic_info.color_default || semantic_info.color_changeable) {
    Color standard_monochrome(STANDARD_MONOCHROME);
    standard_monochrome.vendor_id = base::IntToString(semantic_info.bw_model);
    color.AddDefaultOption(standard_monochrome, !semantic_info.color_default);
  }
  color.SaveTo(&description);

  if (!semantic_info.papers.empty()) {
    Media default_media(semantic_info.default_paper.display_name,
                        semantic_info.default_paper.vendor_id,
                        semantic_info.default_paper.size_um.width(),
                        semantic_info.default_paper.size_um.height());
    default_media.MatchBySize();

    MediaCapability media;
    bool is_default_set = false;
    for (size_t i = 0; i < semantic_info.papers.size(); ++i) {
      gfx::Size paper_size = semantic_info.papers[i].size_um;
      if (paper_size.width() > paper_size.height())
        paper_size.SetSize(paper_size.height(), paper_size.width());
      Media new_media(semantic_info.papers[i].display_name,
                      semantic_info.papers[i].vendor_id, paper_size.width(),
                      paper_size.height());
      new_media.MatchBySize();
      if (new_media.IsValid() && !media.Contains(new_media)) {
        if (!default_media.IsValid())
          default_media = new_media;
        media.AddDefaultOption(new_media, new_media == default_media);
        is_default_set = is_default_set || (new_media == default_media);
      }
    }
    if (!is_default_set && default_media.IsValid())
      media.AddDefaultOption(default_media, true);

    if (media.IsValid()) {
      media.SaveTo(&description);
    } else {
      NOTREACHED();
    }
  }

  if (!semantic_info.dpis.empty()) {
    DpiCapability dpi;
    Dpi default_dpi(semantic_info.default_dpi.width(),
                    semantic_info.default_dpi.height());
    bool is_default_set = false;
    for (size_t i = 0; i < semantic_info.dpis.size(); ++i) {
      Dpi new_dpi(semantic_info.dpis[i].width(),
                  semantic_info.dpis[i].height());
      if (new_dpi.IsValid() && !dpi.Contains(new_dpi)) {
        if (!default_dpi.IsValid())
          default_dpi = new_dpi;
        dpi.AddDefaultOption(new_dpi, new_dpi == default_dpi);
        is_default_set = is_default_set || (new_dpi == default_dpi);
      }
    }
    if (!is_default_set && default_dpi.IsValid())
      dpi.AddDefaultOption(default_dpi, true);
    if (dpi.IsValid()) {
      dpi.SaveTo(&description);
    } else {
      NOTREACHED();
    }
  }

  OrientationCapability orientation;
  orientation.AddDefaultOption(PORTRAIT, true);
  orientation.AddOption(LANDSCAPE);
  orientation.AddOption(AUTO_ORIENTATION);
  orientation.SaveTo(&description);

  return base::WrapUnique(description.root().DeepCopy());
}

}  // namespace cloud_print
