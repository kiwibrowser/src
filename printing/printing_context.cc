// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/printing_context.h"

#include "base/logging.h"
#include "base/values.h"
#include "printing/buildflags/buildflags.h"
#include "printing/page_setup.h"
#include "printing/page_size_margins.h"
#include "printing/print_job_constants.h"
#include "printing/print_settings_conversion.h"
#include "printing/units.h"

namespace printing {

namespace {
const float kCloudPrintMarginInch = 0.25;
}

PrintingContext::PrintingContext(Delegate* delegate)
    : delegate_(delegate),
      in_print_job_(false),
      abort_printing_(false) {
  DCHECK(delegate_);
}

PrintingContext::~PrintingContext() = default;

void PrintingContext::set_margin_type(MarginType type) {
  DCHECK(type != CUSTOM_MARGINS);
  settings_.set_margin_type(type);
}

void PrintingContext::set_is_modifiable(bool is_modifiable) {
  settings_.set_is_modifiable(is_modifiable);
#if defined(OS_WIN)
  settings_.set_print_text_with_gdi(is_modifiable);
#endif
}

void PrintingContext::ResetSettings() {
  ReleaseContext();

  settings_.Clear();

  in_print_job_ = false;
  abort_printing_ = false;
}

PrintingContext::Result PrintingContext::OnError() {
  Result result = abort_printing_ ? CANCEL : FAILED;
  ResetSettings();
  return result;
}

PrintingContext::Result PrintingContext::UsePdfSettings() {
  std::unique_ptr<base::DictionaryValue> pdf_settings(
      new base::DictionaryValue);
  pdf_settings->SetBoolean(kSettingHeaderFooterEnabled, false);
  pdf_settings->SetBoolean(kSettingShouldPrintBackgrounds, false);
  pdf_settings->SetBoolean(kSettingShouldPrintSelectionOnly, false);
  pdf_settings->SetInteger(kSettingMarginsType, printing::NO_MARGINS);
  pdf_settings->SetBoolean(kSettingCollate, true);
  pdf_settings->SetInteger(kSettingCopies, 1);
  pdf_settings->SetInteger(kSettingColor, printing::COLOR);
  pdf_settings->SetInteger(kSettingDpiHorizontal, kPointsPerInch);
  pdf_settings->SetInteger(kSettingDpiVertical, kPointsPerInch);
  pdf_settings->SetInteger(kSettingDuplexMode, printing::SIMPLEX);
  pdf_settings->SetBoolean(kSettingLandscape, false);
  pdf_settings->SetString(kSettingDeviceName, "");
  pdf_settings->SetBoolean(kSettingPrintToPDF, true);
  pdf_settings->SetBoolean(kSettingCloudPrintDialog, false);
  pdf_settings->SetBoolean(kSettingPrintWithPrivet, false);
  pdf_settings->SetBoolean(kSettingPrintWithExtension, false);
  pdf_settings->SetInteger(kSettingScaleFactor, 100);
  pdf_settings->SetBoolean(kSettingRasterizePdf, false);
  pdf_settings->SetInteger(kSettingPagesPerSheet, 1);
  return UpdatePrintSettings(*pdf_settings);
}

PrintingContext::Result PrintingContext::UpdatePrintSettings(
    const base::DictionaryValue& job_settings) {
  ResetSettings();

  if (!PrintSettingsFromJobSettings(job_settings, &settings_)) {
    NOTREACHED();
    return OnError();
  }

  bool print_to_pdf = false;
  bool is_cloud_dialog = false;
  bool print_with_privet = false;
  bool print_with_extension = false;

  if (!job_settings.GetBoolean(kSettingPrintToPDF, &print_to_pdf) ||
      !job_settings.GetBoolean(kSettingCloudPrintDialog, &is_cloud_dialog) ||
      !job_settings.GetBoolean(kSettingPrintWithPrivet, &print_with_privet) ||
      !job_settings.GetBoolean(kSettingPrintWithExtension,
                               &print_with_extension)) {
    NOTREACHED();
    return OnError();
  }

  bool print_to_cloud = job_settings.HasKey(kSettingCloudPrintId);
  bool open_in_external_preview =
      job_settings.HasKey(kSettingOpenPDFInPreview);

  if (!open_in_external_preview &&
      (print_to_pdf || print_to_cloud || is_cloud_dialog || print_with_privet ||
       print_with_extension)) {
    settings_.set_dpi(kDefaultPdfDpi);
    gfx::Size paper_size(GetPdfPaperSizeDeviceUnits());
    if (!settings_.requested_media().size_microns.IsEmpty()) {
      float deviceMicronsPerDeviceUnit =
          (kHundrethsMMPerInch * 10.0f) / settings_.device_units_per_inch();
      paper_size = gfx::Size(settings_.requested_media().size_microns.width() /
                                 deviceMicronsPerDeviceUnit,
                             settings_.requested_media().size_microns.height() /
                                 deviceMicronsPerDeviceUnit);
    }
    gfx::Rect paper_rect(0, 0, paper_size.width(), paper_size.height());
    if (print_to_cloud || print_with_privet) {
      paper_rect.Inset(
          kCloudPrintMarginInch * settings_.device_units_per_inch(),
          kCloudPrintMarginInch * settings_.device_units_per_inch());
    }
    settings_.SetPrinterPrintableArea(paper_size, paper_rect, true);
    return OK;
  }

  bool show_system_dialog = false;
  job_settings.GetBoolean(kSettingShowSystemDialog, &show_system_dialog);

  int page_count = 0;
  job_settings.GetInteger(kSettingPreviewPageCount, &page_count);

  return UpdatePrinterSettings(open_in_external_preview, show_system_dialog,
                               page_count);
}

#if defined(OS_CHROMEOS)
PrintingContext::Result PrintingContext::UpdatePrintSettingsFromPOD(
    std::unique_ptr<PrintSettings> job_settings) {
  ResetSettings();
  settings_ = *job_settings;

  return UpdatePrinterSettings(false /* external_preview */,
                               false /* show_system_dialog */,
                               0 /* page_count is only used on Android */);
}
#endif

}  // namespace printing
