// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/protocol/page_handler.h"

#include "base/base64.h"
#include "content/public/browser/web_contents.h"
#include "printing/units.h"

namespace headless {
namespace protocol {

#if BUILDFLAG(ENABLE_PRINTING)
namespace {

// The max and min value should match the ones in scaling_settings.html.
// Update both files at the same time.
const double kScaleMaxVal = 200;
const double kScaleMinVal = 10;

void PDFCreated(std::unique_ptr<PageHandler::PrintToPDFCallback> callback,
                HeadlessPrintManager::PrintResult print_result,
                const std::string& data) {
  std::unique_ptr<base::DictionaryValue> response;
  if (print_result == HeadlessPrintManager::PRINT_SUCCESS) {
    std::string base_64_data;
    base::Base64Encode(data, &base_64_data);
    callback->sendSuccess(base_64_data);
  } else {
    callback->sendFailure(Response::Error(
        HeadlessPrintManager::PrintResultToString(print_result)));
  }
}

}  // namespace
#endif  // BUILDFLAG(ENABLE_PRINTING)

PageHandler::PageHandler(base::WeakPtr<HeadlessBrowserImpl> browser,
                         content::WebContents* web_contents)
    : DomainHandler(Page::Metainfo::domainName, browser),
      web_contents_(web_contents) {
  DCHECK(web_contents_);
}

PageHandler::~PageHandler() = default;

void PageHandler::Wire(UberDispatcher* dispatcher) {
  Page::Dispatcher::wire(dispatcher, this);
}

void PageHandler::PrintToPDF(Maybe<bool> landscape,
                             Maybe<bool> display_header_footer,
                             Maybe<bool> print_background,
                             Maybe<double> scale,
                             Maybe<double> paper_width,
                             Maybe<double> paper_height,
                             Maybe<double> margin_top,
                             Maybe<double> margin_bottom,
                             Maybe<double> margin_left,
                             Maybe<double> margin_right,
                             Maybe<String> page_ranges,
                             Maybe<bool> ignore_invalid_page_ranges,
                             Maybe<String> header_template,
                             Maybe<String> footer_template,
                             Maybe<bool> prefer_css_page_size,
                             std::unique_ptr<PrintToPDFCallback> callback) {
#if BUILDFLAG(ENABLE_PRINTING)
  HeadlessPrintSettings settings;
  settings.landscape = landscape.fromMaybe(false);
  settings.display_header_footer = display_header_footer.fromMaybe(false);
  settings.should_print_backgrounds = print_background.fromMaybe(false);
  settings.scale = scale.fromMaybe(1.0);
  if (settings.scale > kScaleMaxVal / 100 ||
      settings.scale < kScaleMinVal / 100) {
    callback->sendFailure(
        Response::InvalidParams("scale is outside [0.1 - 2] range"));
    return;
  }
  settings.page_ranges = page_ranges.fromMaybe("");
  settings.ignore_invalid_page_ranges =
      ignore_invalid_page_ranges.fromMaybe(false);

  double paper_width_in_inch =
      paper_width.fromMaybe(printing::kLetterWidthInch);
  double paper_height_in_inch =
      paper_height.fromMaybe(printing::kLetterHeightInch);
  if (paper_width_in_inch <= 0) {
    callback->sendFailure(
        Response::InvalidParams("paperWidth is zero or negative"));
    return;
  }
  if (paper_height_in_inch <= 0) {
    callback->sendFailure(
        Response::InvalidParams("paperHeight is zero or negative"));
    return;
  }
  settings.paper_size_in_points =
      gfx::Size(paper_width_in_inch * printing::kPointsPerInch,
                paper_height_in_inch * printing::kPointsPerInch);

  // Set default margin to 1.0cm = ~2/5 of an inch.
  double default_margin_in_inch = 1000.0 / printing::kHundrethsMMPerInch;
  double margin_top_in_inch = margin_top.fromMaybe(default_margin_in_inch);
  double margin_right_in_inch = margin_right.fromMaybe(default_margin_in_inch);
  double margin_bottom_in_inch =
      margin_bottom.fromMaybe(default_margin_in_inch);
  double margin_left_in_inch = margin_left.fromMaybe(default_margin_in_inch);

  settings.header_template = header_template.fromMaybe("");
  settings.footer_template = footer_template.fromMaybe("");

  if (margin_top_in_inch < 0) {
    callback->sendFailure(Response::InvalidParams("marginTop is negative"));
    return;
  }
  if (margin_bottom_in_inch < 0) {
    callback->sendFailure(Response::InvalidParams("marginBottom is negative"));
    return;
  }
  if (margin_left_in_inch < 0) {
    callback->sendFailure(Response::InvalidParams("marginLeft is negative"));
    return;
  }
  if (margin_right_in_inch < 0) {
    callback->sendFailure(Response::InvalidParams("marginRight is negative"));
    return;
  }
  settings.margins_in_points.top =
      margin_top_in_inch * printing::kPointsPerInch;
  settings.margins_in_points.bottom =
      margin_bottom_in_inch * printing::kPointsPerInch;
  settings.margins_in_points.left =
      margin_left_in_inch * printing::kPointsPerInch;
  settings.margins_in_points.right =
      margin_right_in_inch * printing::kPointsPerInch;
  settings.prefer_css_page_size = prefer_css_page_size.fromMaybe(false);

  HeadlessPrintManager::FromWebContents(web_contents_)
      ->GetPDFContents(web_contents_->GetMainFrame(), settings,
                       base::BindOnce(&PDFCreated, std::move(callback)));
#else
  callback->sendFailure(Response::Error("Printing is not enabled"));
  return;
#endif  // BUILDFLAG(ENABLE_PRINTING)
}

}  // namespace protocol
}  // namespace headless
