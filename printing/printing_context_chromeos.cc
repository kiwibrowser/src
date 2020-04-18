// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/printing_context_chromeos.h"

#include <cups/cups.h>
#include <stdint.h>
#include <unicode/ulocdata.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "printing/backend/cups_connection.h"
#include "printing/backend/cups_ipp_util.h"
#include "printing/backend/cups_printer.h"
#include "printing/metafile.h"
#include "printing/print_job_constants.h"
#include "printing/print_settings.h"
#include "printing/units.h"

namespace printing {

namespace {

using ScopedCupsOption = std::unique_ptr<cups_option_t, OptionDeleter>;

// convert from a ColorMode setting to a print-color-mode value from PWG 5100.13
const char* GetColorModelForMode(int color_mode) {
  const char* mode_string;
  switch (color_mode) {
    case COLOR:
    case CMYK:
    case CMY:
    case KCMY:
    case CMY_K:
    case RGB:
    case RGB16:
    case RGBA:
    case COLORMODE_COLOR:
    case BROTHER_CUPS_COLOR:
    case BROTHER_BRSCRIPT3_COLOR:
    case HP_COLOR_COLOR:
    case PRINTOUTMODE_NORMAL:
    case PROCESSCOLORMODEL_CMYK:
    case PROCESSCOLORMODEL_RGB:
      mode_string = CUPS_PRINT_COLOR_MODE_COLOR;
      break;
    case GRAY:
    case BLACK:
    case GRAYSCALE:
    case COLORMODE_MONOCHROME:
    case BROTHER_CUPS_MONO:
    case BROTHER_BRSCRIPT3_BLACK:
    case HP_COLOR_BLACK:
    case PRINTOUTMODE_NORMAL_GRAY:
    case PROCESSCOLORMODEL_GREYSCALE:
      mode_string = CUPS_PRINT_COLOR_MODE_MONOCHROME;
      break;
    default:
      mode_string = nullptr;
      LOG(WARNING) << "Unrecognized color mode";
      break;
  }

  return mode_string;
}

// Returns a new char buffer which is a null-terminated copy of |value|.  The
// caller owns the returned string.
char* DuplicateString(const base::StringPiece value) {
  char* dst = new char[value.size() + 1];
  value.copy(dst, value.size());
  dst[value.size()] = '\0';
  return dst;
}

ScopedCupsOption ConstructOption(const base::StringPiece name,
                                 const base::StringPiece value) {
  // ScopedCupsOption frees the name and value buffers on deletion
  ScopedCupsOption option = ScopedCupsOption(new cups_option_t);
  option->name = DuplicateString(name);
  option->value = DuplicateString(value);
  return option;
}

base::StringPiece GetCollateString(bool collate) {
  return collate ? kCollated : kUncollated;
}

std::vector<ScopedCupsOption> SettingsToCupsOptions(
    const PrintSettings& settings) {
  const char* sides = nullptr;
  switch (settings.duplex_mode()) {
    case SIMPLEX:
      sides = CUPS_SIDES_ONE_SIDED;
      break;
    case LONG_EDGE:
      sides = CUPS_SIDES_TWO_SIDED_PORTRAIT;
      break;
    case SHORT_EDGE:
      sides = CUPS_SIDES_TWO_SIDED_LANDSCAPE;
      break;
    default:
      NOTREACHED();
  }

  std::vector<ScopedCupsOption> options;
  options.push_back(
      ConstructOption(kIppColor,
                      GetColorModelForMode(settings.color())));  // color
  options.push_back(ConstructOption(kIppDuplex, sides));         // duplexing
  options.push_back(
      ConstructOption(kIppMedia,
                      settings.requested_media().vendor_id));  // paper size
  options.push_back(
      ConstructOption(kIppCopies,
                      base::IntToString(settings.copies())));  // copies
  options.push_back(
      ConstructOption(kIppCollate,
                      GetCollateString(settings.collate())));  // collate

  return options;
}

void SetPrintableArea(PrintSettings* settings,
                      const PrintSettings::RequestedMedia& media,
                      bool flip) {
  if (!media.size_microns.IsEmpty()) {
    float deviceMicronsPerDeviceUnit =
        (kHundrethsMMPerInch * 10.0f) / settings->device_units_per_inch();
    gfx::Size paper_size =
        gfx::Size(media.size_microns.width() / deviceMicronsPerDeviceUnit,
                  media.size_microns.height() / deviceMicronsPerDeviceUnit);

    gfx::Rect paper_rect(0, 0, paper_size.width(), paper_size.height());
    settings->SetPrinterPrintableArea(paper_size, paper_rect, flip);
  }
}

}  // namespace

// static
std::unique_ptr<PrintingContext> PrintingContext::Create(Delegate* delegate) {
  return std::make_unique<PrintingContextChromeos>(delegate);
}

PrintingContextChromeos::PrintingContextChromeos(Delegate* delegate)
    : PrintingContext(delegate),
      connection_(GURL(), HTTP_ENCRYPT_NEVER, true) {}

PrintingContextChromeos::~PrintingContextChromeos() {
  ReleaseContext();
}

void PrintingContextChromeos::AskUserForSettings(
    int max_pages,
    bool has_selection,
    bool is_scripted,
    PrintSettingsCallback callback) {
  // We don't want to bring up a dialog here.  Ever.  This should not be called.
  NOTREACHED();
}

PrintingContext::Result PrintingContextChromeos::UseDefaultSettings() {
  DCHECK(!in_print_job_);

  ResetSettings();

  std::string device_name = base::UTF16ToUTF8(settings_.device_name());
  if (device_name.empty())
    return OnError();

  // TODO(skau): https://crbug.com/613779. See UpdatePrinterSettings for more
  // info.
  if (settings_.dpi() == 0) {
    DVLOG(1) << "Using Default DPI";
    settings_.set_dpi(kDefaultPdfDpi);
  }

  // Retrieve device information and set it
  if (InitializeDevice(device_name) != OK) {
    LOG(ERROR) << "Could not initialize printer";
    return OnError();
  }

  // Set printable area
  DCHECK(printer_);
  PrinterSemanticCapsAndDefaults::Paper paper = DefaultPaper(*printer_);

  PrintSettings::RequestedMedia media;
  media.vendor_id = paper.vendor_id;
  media.size_microns = paper.size_um;
  settings_.set_requested_media(media);

  SetPrintableArea(&settings_, media, true /* flip landscape */);

  return OK;
}

gfx::Size PrintingContextChromeos::GetPdfPaperSizeDeviceUnits() {
  int32_t width = 0;
  int32_t height = 0;
  UErrorCode error = U_ZERO_ERROR;
  ulocdata_getPaperSize(delegate_->GetAppLocale().c_str(), &height, &width,
                        &error);
  if (error > U_ZERO_ERROR) {
    // If the call failed, assume a paper size of 8.5 x 11 inches.
    LOG(WARNING) << "ulocdata_getPaperSize failed, using 8.5 x 11, error: "
                 << error;
    width =
        static_cast<int>(kLetterWidthInch * settings_.device_units_per_inch());
    height =
        static_cast<int>(kLetterHeightInch * settings_.device_units_per_inch());
  } else {
    // ulocdata_getPaperSize returns the width and height in mm.
    // Convert this to pixels based on the dpi.
    float multiplier = 100 * settings_.device_units_per_inch();
    multiplier /= kHundrethsMMPerInch;
    width *= multiplier;
    height *= multiplier;
  }
  return gfx::Size(width, height);
}

PrintingContext::Result PrintingContextChromeos::UpdatePrinterSettings(
    bool external_preview,
    bool show_system_dialog,
    int page_count) {
  DCHECK(!show_system_dialog);

  if (InitializeDevice(base::UTF16ToUTF8(settings_.device_name())) != OK)
    return OnError();

  // TODO(skau): Convert to DCHECK when https://crbug.com/613779 is resolved
  // Print quality suffers when this is set to the resolution reported by the
  // printer but print quality is fine at this resolution. UseDefaultSettings
  // exhibits the same problem.
  if (settings_.dpi() == 0) {
    DVLOG(1) << "Using Default DPI";
    settings_.set_dpi(kDefaultPdfDpi);
  }

  // compute paper size
  PrintSettings::RequestedMedia media = settings_.requested_media();

  if (media.IsDefault()) {
    DCHECK(printer_);
    PrinterSemanticCapsAndDefaults::Paper paper = DefaultPaper(*printer_);

    media.vendor_id = paper.vendor_id;
    media.size_microns = paper.size_um;
    settings_.set_requested_media(media);
  }

  SetPrintableArea(&settings_, media, true);

  return OK;
}

PrintingContext::Result PrintingContextChromeos::InitializeDevice(
    const std::string& device) {
  DCHECK(!in_print_job_);

  std::unique_ptr<CupsPrinter> printer = connection_.GetPrinter(device);
  if (!printer) {
    LOG(WARNING) << "Could not initialize device";
    return OnError();
  }

  printer_ = std::move(printer);

  return OK;
}

PrintingContext::Result PrintingContextChromeos::NewDocument(
    const base::string16& document_name) {
  DCHECK(!in_print_job_);
  in_print_job_ = true;

  std::string converted_name = base::UTF16ToUTF8(document_name);
  std::string title = base::UTF16ToUTF8(settings_.title());
  std::vector<ScopedCupsOption> cups_options = SettingsToCupsOptions(settings_);

  std::vector<cups_option_t> options;
  for (const ScopedCupsOption& option : cups_options) {
    if (printer_->CheckOptionSupported(option->name, option->value)) {
      options.push_back(*(option.get()));
    } else {
      DVLOG(1) << "Unsupported option skipped " << option->name << ", "
               << option->value;
    }
  }

  ipp_status_t create_status = printer_->CreateJob(&job_id_, title, options);

  if (job_id_ == 0) {
    DLOG(WARNING) << "Creating cups job failed"
                  << ippErrorString(create_status);
    return OnError();
  }

  // we only send one document, so it's always the last one
  if (!printer_->StartDocument(job_id_, converted_name, true, options)) {
    LOG(ERROR) << "Starting document failed";
    return OnError();
  }

  return OK;
}

PrintingContext::Result PrintingContextChromeos::NewPage() {
  if (abort_printing_)
    return CANCEL;

  DCHECK(in_print_job_);

  // Intentional No-op.

  return OK;
}

PrintingContext::Result PrintingContextChromeos::PageDone() {
  if (abort_printing_)
    return CANCEL;

  DCHECK(in_print_job_);

  // Intentional No-op.

  return OK;
}

PrintingContext::Result PrintingContextChromeos::DocumentDone() {
  if (abort_printing_)
    return CANCEL;

  DCHECK(in_print_job_);

  if (!printer_->FinishDocument()) {
    LOG(WARNING) << "Finishing document failed";
    return OnError();
  }

  ipp_status_t job_status = printer_->CloseJob(job_id_);
  job_id_ = 0;

  if (job_status != IPP_STATUS_OK) {
    LOG(WARNING) << "Closing job failed";
    return OnError();
  }

  ResetSettings();
  return OK;
}

void PrintingContextChromeos::Cancel() {
  abort_printing_ = true;
  in_print_job_ = false;
}

void PrintingContextChromeos::ReleaseContext() {
  printer_.reset();
}

printing::NativeDrawingContext PrintingContextChromeos::context() const {
  // Intentional No-op.
  return nullptr;
}

PrintingContext::Result PrintingContextChromeos::StreamData(
    const std::vector<char>& buffer) {
  if (abort_printing_)
    return CANCEL;

  DCHECK(in_print_job_);
  DCHECK(printer_);

  if (!printer_->StreamData(buffer))
    return OnError();

  return OK;
}

}  // namespace printing
