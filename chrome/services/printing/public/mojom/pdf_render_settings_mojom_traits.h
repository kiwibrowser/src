// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_PRINTING_PUBLIC_MOJOM_PDF_RENDER_SETTINGS_MOJOM_TRAITS_H_
#define CHROME_SERVICES_PRINTING_PUBLIC_MOJOM_PDF_RENDER_SETTINGS_MOJOM_TRAITS_H_

#include "build/build_config.h"
#include "chrome/services/printing/public/mojom/pdf_render_settings.mojom.h"
#include "printing/pdf_render_settings.h"

namespace mojo {

template <>
struct EnumTraits<printing::mojom::PdfRenderSettings::Mode,
                  printing::PdfRenderSettings::Mode> {
  static printing::mojom::PdfRenderSettings::Mode ToMojom(
      printing::PdfRenderSettings::Mode mode) {
    switch (mode) {
      case printing::PdfRenderSettings::Mode::NORMAL:
        return printing::mojom::PdfRenderSettings::Mode::NORMAL;
#if defined(OS_WIN)
      case printing::PdfRenderSettings::Mode::TEXTONLY:
        return printing::mojom::PdfRenderSettings::Mode::TEXTONLY;
      case printing::PdfRenderSettings::Mode::GDI_TEXT:
        return printing::mojom::PdfRenderSettings::Mode::GDI_TEXT;
      case printing::PdfRenderSettings::Mode::POSTSCRIPT_LEVEL2:
        return printing::mojom::PdfRenderSettings::Mode::POSTSCRIPT_LEVEL2;
      case printing::PdfRenderSettings::Mode::POSTSCRIPT_LEVEL3:
        return printing::mojom::PdfRenderSettings::Mode::POSTSCRIPT_LEVEL3;
#endif
    }
    NOTREACHED() << "Unknown mode " << static_cast<int>(mode);
    return printing::mojom::PdfRenderSettings::Mode::NORMAL;
  }

  static bool FromMojom(printing::mojom::PdfRenderSettings::Mode input,
                        printing::PdfRenderSettings::Mode* output) {
    switch (input) {
      case printing::mojom::PdfRenderSettings::Mode::NORMAL:
        *output = printing::PdfRenderSettings::Mode::NORMAL;
        return true;
#if defined(OS_WIN)
      case printing::mojom::PdfRenderSettings::Mode::TEXTONLY:
        *output = printing::PdfRenderSettings::Mode::TEXTONLY;
        return true;
      case printing::mojom::PdfRenderSettings::Mode::GDI_TEXT:
        *output = printing::PdfRenderSettings::Mode::GDI_TEXT;
        return true;
      case printing::mojom::PdfRenderSettings::Mode::POSTSCRIPT_LEVEL2:
        *output = printing::PdfRenderSettings::Mode::POSTSCRIPT_LEVEL2;
        return true;
      case printing::mojom::PdfRenderSettings::Mode::POSTSCRIPT_LEVEL3:
        *output = printing::PdfRenderSettings::Mode::POSTSCRIPT_LEVEL3;
        return true;
#endif
    }
    NOTREACHED() << "Unknown mode " << static_cast<int>(input);
    return false;
  }
};

template <>
class StructTraits<printing::mojom::PdfRenderSettingsDataView,
                   printing::PdfRenderSettings> {
 public:
  static gfx::Rect area(const printing::PdfRenderSettings& settings) {
    return settings.area;
  }
  static gfx::Point offsets(const printing::PdfRenderSettings& settings) {
    return settings.offsets;
  }
  static gfx::Size dpi(const printing::PdfRenderSettings& settings) {
    return settings.dpi;
  }
  static bool autorotate(const printing::PdfRenderSettings& settings) {
    return settings.autorotate;
  }
  static bool use_color(const printing::PdfRenderSettings& settings) {
    return settings.use_color;
  }
  static printing::PdfRenderSettings::Mode mode(
      const printing::PdfRenderSettings& settings) {
    return settings.mode;
  }

  static bool Read(printing::mojom::PdfRenderSettingsDataView data,
                   printing::PdfRenderSettings* out_settings);
};

}  // namespace mojo

#endif  // CHROME_SERVICES_PRINTING_PUBLIC_MOJOM_PDF_RENDER_SETTINGS_MOJOM_TRAITS_H_
