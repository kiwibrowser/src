// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/webui/web_ui_util.h"

#include <vector>

#include "base/base64.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "net/base/escape.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/template_expressions.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/font.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/resources/grit/webui_resources.h"
#include "ui/strings/grit/app_locale_settings.h"
#include "url/gurl.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#endif

namespace webui {

std::string GetBitmapDataUrl(const SkBitmap& bitmap) {
  TRACE_EVENT2("oobe", "GetImageDataUrl",
               "width", bitmap.width(), "height", bitmap.height());
  std::vector<unsigned char> output;
  gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, false, &output);
  return GetPngDataUrl(output.data(), output.size());
}

std::string GetPngDataUrl(const unsigned char* data, size_t size) {
  std::string str_url(reinterpret_cast<const char*>(data), size);
  base::Base64Encode(str_url, &str_url);
  str_url.insert(0, "data:image/png;base64,");
  return str_url;
}

WindowOpenDisposition GetDispositionFromClick(const base::ListValue* args,
                                              int start_index) {
  double button = 0.0;
  bool alt_key = false;
  bool ctrl_key = false;
  bool meta_key = false;
  bool shift_key = false;

  CHECK(args->GetDouble(start_index++, &button));
  CHECK(args->GetBoolean(start_index++, &alt_key));
  CHECK(args->GetBoolean(start_index++, &ctrl_key));
  CHECK(args->GetBoolean(start_index++, &meta_key));
  CHECK(args->GetBoolean(start_index++, &shift_key));
  return ui::DispositionFromClick(
      button == 1.0, alt_key, ctrl_key, meta_key, shift_key);
}

bool ParseScaleFactor(const base::StringPiece& identifier,
                      float* scale_factor) {
  *scale_factor = 1.0f;
  if (identifier.empty()) {
    LOG(WARNING) << "Invalid scale factor format: " << identifier;
    return false;
  }

  if (*identifier.rbegin() != 'x') {
    LOG(WARNING) << "Invalid scale factor format: " << identifier;
    return false;
  }

  double scale = 0;
  std::string stripped;
  identifier.substr(0, identifier.length() - 1).CopyToString(&stripped);
  if (!base::StringToDouble(stripped, &scale)) {
    LOG(WARNING) << "Invalid scale factor format: " << identifier;
    return false;
  }
  *scale_factor = static_cast<float>(scale);
  return true;
}

// Parse a formatted frame index string into int and sets to |frame_index|.
bool ParseFrameIndex(const base::StringPiece& identifier, int* frame_index) {
  *frame_index = -1;
  if (identifier.empty()) {
    LOG(WARNING) << "Invalid frame index format: " << identifier;
    return false;
  }

  if (*identifier.rbegin() != ']') {
    LOG(WARNING) << "Invalid frame index format: " << identifier;
    return false;
  }

  unsigned frame = 0;
  if (!base::StringToUint(identifier.substr(0, identifier.length() - 1),
                          &frame)) {
    LOG(WARNING) << "Invalid frame index format: " << identifier;
    return false;
  }
  *frame_index = static_cast<int>(frame);
  return true;
}

void ParsePathAndImageSpec(const GURL& url,
                           std::string* path,
                           float* scale_factor,
                           int* frame_index) {
  *path = net::UnescapeURLComponent(
      url.path().substr(1),
      net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS |
          net::UnescapeRule::SPACES);
  if (scale_factor)
    *scale_factor = 1.0f;
  if (frame_index)
    *frame_index = -1;

  // Detect and parse resource string ending in @<scale>x.
  std::size_t pos = path->rfind('@');
  if (pos != std::string::npos) {
    base::StringPiece stripped_path(*path);
    float factor;

    if (ParseScaleFactor(stripped_path.substr(
            pos + 1, stripped_path.length() - pos - 1), &factor)) {
      // Strip scale factor specification from path.
      stripped_path.remove_suffix(stripped_path.length() - pos);
      stripped_path.CopyToString(path);
    }
    if (scale_factor)
      *scale_factor = factor;
  }

  // Detect and parse resource string ending in [<frame>].
  pos = path->rfind('[');
  if (pos != std::string::npos) {
    base::StringPiece stripped_path(*path);
    int index;

    if (ParseFrameIndex(
            stripped_path.substr(pos + 1, stripped_path.length() - pos - 1),
            &index)) {
      // Strip frame index specification from path.
      stripped_path.remove_suffix(stripped_path.length() - pos);
      stripped_path.CopyToString(path);
    }
    if (frame_index)
      *frame_index = index;
  }
}

void ParsePathAndScale(const GURL& url,
                       std::string* path,
                       float* scale_factor) {
  ParsePathAndImageSpec(url, path, scale_factor, nullptr);
}

void ParsePathAndFrame(const GURL& url, std::string* path, int* frame_index) {
  ParsePathAndImageSpec(url, path, nullptr, frame_index);
}

void SetLoadTimeDataDefaults(const std::string& app_locale,
                             base::DictionaryValue* localized_strings) {
  localized_strings->SetString("fontfamily", GetFontFamily());
  localized_strings->SetString("fontsize", GetFontSize());
  localized_strings->SetString("language", l10n_util::GetLanguage(app_locale));
  localized_strings->SetString("textdirection", GetTextDirection());
}

void SetLoadTimeDataDefaults(const std::string& app_locale,
                             ui::TemplateReplacements* replacements) {
  (*replacements)["fontfamily"] = GetFontFamily();
  (*replacements)["fontsize"] = GetFontSize();
  (*replacements)["language"] = l10n_util::GetLanguage(app_locale);
  (*replacements)["textdirection"] = GetTextDirection();
}

std::string GetWebUiCssTextDefaults(base::StringPiece css_template) {
  ui::TemplateReplacements placeholders;
  placeholders["textDirection"] = GetTextDirection();
  placeholders["fontFamily"] = GetFontFamily();
  placeholders["fontSize"] = GetFontSize();
  return ui::ReplaceTemplateExpressions(css_template, placeholders);
}

std::string GetWebUiCssTextDefaults() {
  const ui::ResourceBundle& resource_bundle =
      ui::ResourceBundle::GetSharedInstance();
  return GetWebUiCssTextDefaults(
      resource_bundle.GetRawDataResource(IDR_WEBUI_CSS_TEXT_DEFAULTS));
}

std::string GetWebUiCssTextDefaultsMd() {
  const ui::ResourceBundle& resource_bundle =
      ui::ResourceBundle::GetSharedInstance();
  return GetWebUiCssTextDefaults(
      resource_bundle.GetRawDataResource(IDR_WEBUI_CSS_TEXT_DEFAULTS_MD));
}

void AppendWebUiCssTextDefaults(std::string* html) {
  html->append("<style>");
  html->append(GetWebUiCssTextDefaults());
  html->append("</style>");
}

std::string GetFontFamily() {
  std::string font_family = l10n_util::GetStringUTF8(IDS_WEB_FONT_FAMILY);

// TODO(dnicoara) Remove Ozone check when PlatformFont support is introduced
// into Ozone: crbug.com/320050
#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && !defined(USE_OZONE)
  font_family = ui::ResourceBundle::GetSharedInstance().GetFont(
      ui::ResourceBundle::BaseFont).GetFontName() + ", " + font_family;
#endif

  return font_family;
}

std::string GetFontSize() {
  return l10n_util::GetStringUTF8(IDS_WEB_FONT_SIZE);
}

std::string GetTextDirection() {
  return base::i18n::IsRTL() ? "rtl" : "ltr";
}

}  // namespace webui
