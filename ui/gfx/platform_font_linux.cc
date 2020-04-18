// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/platform_font_linux.h"

#include <algorithm>
#include <string>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "third_party/skia/include/core/SkFontStyle.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkString.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/linux_font_delegate.h"
#include "ui/gfx/text_utils.h"

namespace gfx {
namespace {

// The font family name which is used when a user's application font for
// GNOME/KDE is a non-scalable one. The name should be listed in the
// IsFallbackFontAllowed function in skia/ext/SkFontHost_fontconfig_direct.cpp.
#if defined(OS_ANDROID)
const char* kFallbackFontFamilyName = "serif";
#else
const char* kFallbackFontFamilyName = "sans";
#endif

// The default font, used for the default constructor.
base::LazyInstance<scoped_refptr<PlatformFontLinux>>::Leaky g_default_font =
    LAZY_INSTANCE_INITIALIZER;

// Creates a SkTypeface for the passed-in Font::FontStyle and family. If a
// fallback typeface is used instead of the requested family, |family| will be
// updated to contain the fallback's family name.
sk_sp<SkTypeface> CreateSkTypeface(bool italic,
                                   gfx::Font::Weight weight,
                                   std::string* family,
                                   bool* out_success) {
  DCHECK(family);

  const int font_weight = (weight == Font::Weight::INVALID)
                              ? static_cast<int>(Font::Weight::NORMAL)
                              : static_cast<int>(weight);
  SkFontStyle sk_style(
      font_weight, SkFontStyle::kNormal_Width,
      italic ? SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant);
  sk_sp<SkTypeface> typeface =
      SkTypeface::MakeFromName(family->c_str(), sk_style);
  if (!typeface) {
    // A non-scalable font such as .pcf is specified. Fall back to a default
    // scalable font.
    typeface = sk_sp<SkTypeface>(SkTypeface::MakeFromName(
        kFallbackFontFamilyName, sk_style));
    if (!typeface) {
      *out_success = false;
      return nullptr;
    }
    *family = kFallbackFontFamilyName;
  }
  *out_success = true;
  return typeface;
}

}  // namespace

#if defined(OS_CHROMEOS)
std::string* PlatformFontLinux::default_font_description_ = NULL;
#endif

////////////////////////////////////////////////////////////////////////////////
// PlatformFontLinux, public:

PlatformFontLinux::PlatformFontLinux() {
  CHECK(InitDefaultFont()) << "Could not find the default font";
  InitFromPlatformFont(g_default_font.Get().get());
}

PlatformFontLinux::PlatformFontLinux(const std::string& font_name,
                                     int font_size_pixels) {
  FontRenderParamsQuery query;
  query.families.push_back(font_name);
  query.pixel_size = font_size_pixels;
  query.weight = Font::Weight::NORMAL;
  InitFromDetails(nullptr, font_name, font_size_pixels, Font::NORMAL,
                  query.weight, gfx::GetFontRenderParams(query, nullptr));
}

////////////////////////////////////////////////////////////////////////////////
// PlatformFontLinux, PlatformFont implementation:

// static
bool PlatformFontLinux::InitDefaultFont() {
  if (g_default_font.Get())
    return true;

  bool success = false;
  std::string family = kFallbackFontFamilyName;
  int size_pixels = 12;
  int style = Font::NORMAL;
  Font::Weight weight = Font::Weight::NORMAL;
  FontRenderParams params;

#if defined(OS_CHROMEOS)
  // On Chrome OS, a FontList font description string is stored as a
  // translatable resource and passed in via SetDefaultFontDescription().
  if (default_font_description_) {
    FontRenderParamsQuery query;
    CHECK(FontList::ParseDescription(*default_font_description_,
                                     &query.families, &query.style,
                                     &query.pixel_size, &query.weight))
        << "Failed to parse font description " << *default_font_description_;
    params = gfx::GetFontRenderParams(query, &family);
    size_pixels = query.pixel_size;
    style = query.style;
    weight = query.weight;
  }
#else
  // On Linux, LinuxFontDelegate is used to query the native toolkit (e.g.
  // GTK+) for the default UI font.
  const LinuxFontDelegate* delegate = LinuxFontDelegate::instance();
  if (delegate) {
    delegate->GetDefaultFontDescription(&family, &size_pixels, &style, &weight,
                                        &params);
  }
#endif

  sk_sp<SkTypeface> typeface =
      CreateSkTypeface(style & Font::ITALIC, weight, &family, &success);
  if (!success)
    return false;
  g_default_font.Get() = new PlatformFontLinux(
      std::move(typeface), family, size_pixels, style, weight, params);
  return true;
}

// static
void PlatformFontLinux::ReloadDefaultFont() {
  // Reset the scoped_refptr.
  g_default_font.Get() = nullptr;
}

#if defined(OS_CHROMEOS)
// static
void PlatformFontLinux::SetDefaultFontDescription(
    const std::string& font_description) {
  delete default_font_description_;
  default_font_description_ = new std::string(font_description);
}

#endif

Font PlatformFontLinux::DeriveFont(int size_delta,
                                   int style,
                                   Font::Weight weight) const {
  const int new_size = font_size_pixels_ + size_delta;
  DCHECK_GT(new_size, 0);

  // If the style changed, we may need to load a new face.
  std::string new_family = font_family_;
  bool success = true;
  sk_sp<SkTypeface> typeface =
      (weight == weight_ && style == style_)
          ? typeface_
          : CreateSkTypeface(style, weight, &new_family, &success);
  if (!success) {
    LOG(ERROR) << "Could not find any font: " << new_family << ", "
               << kFallbackFontFamilyName << ". Falling back to the default";
    return Font(new PlatformFontLinux);
  }

  FontRenderParamsQuery query;
  query.families.push_back(new_family);
  query.pixel_size = new_size;
  query.style = style;

  return Font(new PlatformFontLinux(std::move(typeface), new_family, new_size,
      style, weight, gfx::GetFontRenderParams(query, NULL)));
}

int PlatformFontLinux::GetHeight() {
  ComputeMetricsIfNecessary();
  return height_pixels_;
}

Font::Weight PlatformFontLinux::GetWeight() const {
  return weight_;
}

int PlatformFontLinux::GetBaseline() {
  ComputeMetricsIfNecessary();
  return ascent_pixels_;
}

int PlatformFontLinux::GetCapHeight() {
  ComputeMetricsIfNecessary();
  return cap_height_pixels_;
}

int PlatformFontLinux::GetExpectedTextWidth(int length) {
  ComputeMetricsIfNecessary();
  return round(static_cast<float>(length) * average_width_pixels_);
}

int PlatformFontLinux::GetStyle() const {
  return style_;
}

const std::string& PlatformFontLinux::GetFontName() const {
  return font_family_;
}

std::string PlatformFontLinux::GetActualFontNameForTesting() const {
  SkString family_name;
  typeface_->getFamilyName(&family_name);
  return family_name.c_str();
}

int PlatformFontLinux::GetFontSize() const {
  return font_size_pixels_;
}

const FontRenderParams& PlatformFontLinux::GetFontRenderParams() {
  float current_scale_factor = GetFontRenderParamsDeviceScaleFactor();
  if (current_scale_factor != device_scale_factor_) {
    FontRenderParamsQuery query;
    query.families.push_back(font_family_);
    query.pixel_size = font_size_pixels_;
    query.style = style_;
    query.weight = weight_;
    query.device_scale_factor = current_scale_factor;
    font_render_params_ = gfx::GetFontRenderParams(query, nullptr);
    device_scale_factor_ = current_scale_factor;
  }
  return font_render_params_;
}

////////////////////////////////////////////////////////////////////////////////
// PlatformFontLinux, private:

PlatformFontLinux::PlatformFontLinux(sk_sp<SkTypeface> typeface,
                                     const std::string& family,
                                     int size_pixels,
                                     int style,
                                     Font::Weight weight,
                                     const FontRenderParams& render_params) {
  InitFromDetails(std::move(typeface), family, size_pixels, style, weight,
      render_params);
}

PlatformFontLinux::~PlatformFontLinux() {}

void PlatformFontLinux::InitFromDetails(
    sk_sp<SkTypeface> typeface,
    const std::string& font_family,
    int font_size_pixels,
    int style,
    Font::Weight weight,
    const FontRenderParams& render_params) {
  DCHECK_GT(font_size_pixels, 0);

  font_family_ = font_family;
  bool success = true;
  typeface_ = typeface ? std::move(typeface)
                       : CreateSkTypeface(style & Font::ITALIC, weight,
                                          &font_family_, &success);

  if (!success) {
    LOG(ERROR) << "Could not find any font: " << font_family << ", "
               << kFallbackFontFamilyName << ". Falling back to the default";

    InitFromPlatformFont(g_default_font.Get().get());
    return;
  }

  font_size_pixels_ = font_size_pixels;
  style_ = style;
  weight_ = weight;
  device_scale_factor_ = GetFontRenderParamsDeviceScaleFactor();
  font_render_params_ = render_params;
}

void PlatformFontLinux::InitFromPlatformFont(const PlatformFontLinux* other) {
  typeface_ = other->typeface_;
  font_family_ = other->font_family_;
  font_size_pixels_ = other->font_size_pixels_;
  style_ = other->style_;
  weight_ = other->weight_;
  device_scale_factor_ = other->device_scale_factor_;
  font_render_params_ = other->font_render_params_;

  if (!other->metrics_need_computation_) {
    metrics_need_computation_ = false;
    ascent_pixels_ = other->ascent_pixels_;
    height_pixels_ = other->height_pixels_;
    cap_height_pixels_ = other->cap_height_pixels_;
    average_width_pixels_ = other->average_width_pixels_;
  }
}

void PlatformFontLinux::ComputeMetricsIfNecessary() {
  if (metrics_need_computation_) {
    metrics_need_computation_ = false;

    SkPaint paint;
    paint.setAntiAlias(false);
    paint.setSubpixelText(false);
    paint.setTextSize(font_size_pixels_);
    paint.setTypeface(typeface_);
    paint.setFakeBoldText(weight_ >= Font::Weight::BOLD &&
                          !typeface_->isBold());
    paint.setTextSkewX((Font::ITALIC & style_) && !typeface_->isItalic() ?
                        -SK_Scalar1/4 : 0);
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);
    ascent_pixels_ = SkScalarCeilToInt(-metrics.fAscent);
    height_pixels_ = ascent_pixels_ + SkScalarCeilToInt(metrics.fDescent);
    cap_height_pixels_ = SkScalarCeilToInt(metrics.fCapHeight);
    average_width_pixels_ = SkScalarToDouble(metrics.fAvgCharWidth);
  }
}

////////////////////////////////////////////////////////////////////////////////
// PlatformFont, public:

// static
PlatformFont* PlatformFont::CreateDefault() {
  return new PlatformFontLinux;
}

// static
PlatformFont* PlatformFont::CreateFromNameAndSize(const std::string& font_name,
                                                  int font_size) {
  return new PlatformFontLinux(font_name, font_size);
}

}  // namespace gfx
