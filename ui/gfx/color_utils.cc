// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/color_utils.h"

#include <stdint.h>

#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"

#if defined(OS_WIN)
#include <windows.h>
#include "skia/ext/skia_utils_win.h"
#endif

namespace color_utils {


// Helper functions -----------------------------------------------------------

namespace {

int calcHue(double temp1, double temp2, double hue) {
  if (hue < 0.0)
    ++hue;
  else if (hue > 1.0)
    --hue;

  double result = temp1;
  if (hue * 6.0 < 1.0)
    result = temp1 + (temp2 - temp1) * hue * 6.0;
  else if (hue * 2.0 < 1.0)
    result = temp2;
  else if (hue * 3.0 < 2.0)
    result = temp1 + (temp2 - temp1) * (2.0 / 3.0 - hue) * 6.0;

  return static_cast<int>(std::round(result * 255));
}

// Assumes sRGB.
double Linearize(double eight_bit_component) {
  const double component = eight_bit_component / 255.0;
  // The W3C link in the header uses 0.03928 here.  See
  // https://en.wikipedia.org/wiki/SRGB#Theory_of_the_transformation for
  // discussion of why we use this value rather than that one.
  return (component <= 0.04045) ?
      (component / 12.92) : pow((component + 0.055) / 1.055, 2.4);
}

SkColor LightnessInvertColor(SkColor color) {
  HSL hsl;
  SkColorToHSL(color, &hsl);
  hsl.l = 1.0 - hsl.l;
  return HSLToSkColor(hsl, SkColorGetA(color));
}

}  // namespace


// ----------------------------------------------------------------------------

double GetContrastRatio(SkColor color_a, SkColor color_b) {
  return GetContrastRatio(GetRelativeLuminance(color_a),
                          GetRelativeLuminance(color_b));
}

double GetContrastRatio(double luminance_a, double luminance_b) {
  DCHECK_GE(luminance_a, 0.0);
  DCHECK_GE(luminance_b, 0.0);
  luminance_a += 0.05;
  luminance_b += 0.05;
  return (luminance_a > luminance_b) ? (luminance_a / luminance_b)
                                     : (luminance_b / luminance_a);
}

double GetRelativeLuminance(SkColor color) {
  return (0.2126 * Linearize(SkColorGetR(color))) +
         (0.7152 * Linearize(SkColorGetG(color))) +
         (0.0722 * Linearize(SkColorGetB(color)));
}

uint8_t GetLuma(SkColor color) {
  return static_cast<uint8_t>(std::round((0.299 * SkColorGetR(color)) +
                                         (0.587 * SkColorGetG(color)) +
                                         (0.114 * SkColorGetB(color))));
}

void SkColorToHSL(SkColor c, HSL* hsl) {
  double r = static_cast<double>(SkColorGetR(c)) / 255.0;
  double g = static_cast<double>(SkColorGetG(c)) / 255.0;
  double b = static_cast<double>(SkColorGetB(c)) / 255.0;
  double vmax = std::max(std::max(r, g), b);
  double vmin = std::min(std::min(r, g), b);
  double delta = vmax - vmin;
  hsl->l = (vmax + vmin) / 2;
  if (SkColorGetR(c) == SkColorGetG(c) && SkColorGetR(c) == SkColorGetB(c)) {
    hsl->h = hsl->s = 0;
  } else {
    double dr = (((vmax - r) / 6.0) + (delta / 2.0)) / delta;
    double dg = (((vmax - g) / 6.0) + (delta / 2.0)) / delta;
    double db = (((vmax - b) / 6.0) + (delta / 2.0)) / delta;
    // We need to compare for the max value because comparing vmax to r, g, or b
    // can sometimes result in values overflowing registers.
    if (r >= g && r >= b)
      hsl->h = db - dg;
    else if (g >= r && g >= b)
      hsl->h = (1.0 / 3.0) + dr - db;
    else  // (b >= r && b >= g)
      hsl->h = (2.0 / 3.0) + dg - dr;

    if (hsl->h < 0.0)
      ++hsl->h;
    else if (hsl->h > 1.0)
      --hsl->h;

    hsl->s = delta / ((hsl->l < 0.5) ? (vmax + vmin) : (2 - vmax - vmin));
  }
}

SkColor HSLToSkColor(const HSL& hsl, SkAlpha alpha) {
  double hue = hsl.h;
  double saturation = hsl.s;
  double lightness = hsl.l;

  // If there's no color, we don't care about hue and can do everything based on
  // brightness.
  if (!saturation) {
    const uint8_t light =
        base::saturated_cast<uint8_t>(gfx::ToRoundedInt(lightness * 255));
    return SkColorSetARGB(alpha, light, light, light);
  }

  double temp2 = (lightness < 0.5) ?
      (lightness * (1.0 + saturation)) :
      (lightness + saturation - (lightness * saturation));
  double temp1 = 2.0 * lightness - temp2;
  return SkColorSetARGB(alpha, calcHue(temp1, temp2, hue + 1.0 / 3.0),
                        calcHue(temp1, temp2, hue),
                        calcHue(temp1, temp2, hue - 1.0 / 3.0));
}

bool IsWithinHSLRange(const HSL& hsl,
                      const HSL& lower_bound,
                      const HSL& upper_bound) {
  DCHECK(hsl.h >= 0 && hsl.h <= 1) << hsl.h;
  DCHECK(hsl.s >= 0 && hsl.s <= 1) << hsl.s;
  DCHECK(hsl.l >= 0 && hsl.l <= 1) << hsl.l;
  DCHECK(lower_bound.h < 0 || upper_bound.h < 0 ||
         (lower_bound.h <= 1 && upper_bound.h <= lower_bound.h + 1))
      << "lower_bound.h: " << lower_bound.h
      << ", upper_bound.h: " << upper_bound.h;
  DCHECK(lower_bound.s < 0 || upper_bound.s < 0 ||
         (lower_bound.s <= upper_bound.s && upper_bound.s <= 1))
      << "lower_bound.s: " << lower_bound.s
      << ", upper_bound.s: " << upper_bound.s;
  DCHECK(lower_bound.l < 0 || upper_bound.l < 0 ||
         (lower_bound.l <= upper_bound.l && upper_bound.l <= 1))
      << "lower_bound.l: " << lower_bound.l
      << ", upper_bound.l: " << upper_bound.l;

  // If the upper hue is >1, the given hue bounds wrap around at 1.
  bool matches_hue = upper_bound.h > 1
                         ? hsl.h >= lower_bound.h || hsl.h <= upper_bound.h - 1
                         : hsl.h >= lower_bound.h && hsl.h <= upper_bound.h;
  return (upper_bound.h < 0 || lower_bound.h < 0 || matches_hue) &&
         (upper_bound.s < 0 || lower_bound.s < 0 ||
          (hsl.s >= lower_bound.s && hsl.s <= upper_bound.s)) &&
         (upper_bound.l < 0 || lower_bound.l < 0 ||
          (hsl.l >= lower_bound.l && hsl.l <= upper_bound.l));
}

void MakeHSLShiftValid(HSL* hsl) {
  if (hsl->h < 0 || hsl->h > 1)
    hsl->h = -1;
  if (hsl->s < 0 || hsl->s > 1)
    hsl->s = -1;
  if (hsl->l < 0 || hsl->l > 1)
    hsl->l = -1;
}

SkColor HSLShift(SkColor color, const HSL& shift) {
  SkAlpha alpha = SkColorGetA(color);

  if (shift.h >= 0 || shift.s >= 0) {
    HSL hsl;
    SkColorToHSL(color, &hsl);

    // Replace the hue with the tint's hue.
    if (shift.h >= 0)
      hsl.h = shift.h;

    // Change the saturation.
    if (shift.s >= 0) {
      if (shift.s <= 0.5)
        hsl.s *= shift.s * 2.0;
      else
        hsl.s += (1.0 - hsl.s) * ((shift.s - 0.5) * 2.0);
    }

    color = HSLToSkColor(hsl, alpha);
  }

  if (shift.l < 0)
    return color;

  // Lightness shifts in the style of popular image editors aren't actually
  // represented in HSL - the L value does have some effect on saturation.
  double r = static_cast<double>(SkColorGetR(color));
  double g = static_cast<double>(SkColorGetG(color));
  double b = static_cast<double>(SkColorGetB(color));
  if (shift.l <= 0.5) {
    r *= (shift.l * 2.0);
    g *= (shift.l * 2.0);
    b *= (shift.l * 2.0);
  } else {
    r += (255.0 - r) * ((shift.l - 0.5) * 2.0);
    g += (255.0 - g) * ((shift.l - 0.5) * 2.0);
    b += (255.0 - b) * ((shift.l - 0.5) * 2.0);
  }
  return SkColorSetARGB(alpha,
                        static_cast<int>(std::round(r)),
                        static_cast<int>(std::round(g)),
                        static_cast<int>(std::round(b)));
}

void BuildLumaHistogram(const SkBitmap& bitmap, int histogram[256]) {
  DCHECK_EQ(kN32_SkColorType, bitmap.colorType());

  int pixel_width = bitmap.width();
  int pixel_height = bitmap.height();
  for (int y = 0; y < pixel_height; ++y) {
    for (int x = 0; x < pixel_width; ++x)
      ++histogram[GetLuma(bitmap.getColor(x, y))];
  }
}

double CalculateBoringScore(const SkBitmap& bitmap) {
  if (bitmap.isNull() || bitmap.empty())
    return 1.0;
  int histogram[256] = {0};
  BuildLumaHistogram(bitmap, histogram);

  int color_count = *std::max_element(histogram, histogram + 256);
  int pixel_count = bitmap.width() * bitmap.height();
  return static_cast<double>(color_count) / pixel_count;
}

SkColor AlphaBlend(SkColor foreground, SkColor background, SkAlpha alpha) {
  if (alpha == 0)
    return background;
  if (alpha == 255)
    return foreground;

  int f_alpha = SkColorGetA(foreground);
  int b_alpha = SkColorGetA(background);

  double normalizer = (f_alpha * alpha + b_alpha * (255 - alpha)) / 255.0;
  if (normalizer == 0.0)
    return SK_ColorTRANSPARENT;

  double f_weight = f_alpha * alpha / normalizer;
  double b_weight = b_alpha * (255 - alpha) / normalizer;

  double r = (SkColorGetR(foreground) * f_weight +
              SkColorGetR(background) * b_weight) / 255.0;
  double g = (SkColorGetG(foreground) * f_weight +
              SkColorGetG(background) * b_weight) / 255.0;
  double b = (SkColorGetB(foreground) * f_weight +
              SkColorGetB(background) * b_weight) / 255.0;

  return SkColorSetARGB(static_cast<int>(std::round(normalizer)),
                        static_cast<int>(std::round(r)),
                        static_cast<int>(std::round(g)),
                        static_cast<int>(std::round(b)));
}

SkColor GetResultingPaintColor(SkColor foreground, SkColor background) {
  return AlphaBlend(SkColorSetA(foreground, SK_AlphaOPAQUE), background,
                    SkColorGetA(foreground));
}

bool IsDark(SkColor color) {
  return GetLuma(color) < 128;
}

SkColor BlendTowardOppositeLuma(SkColor color, SkAlpha alpha) {
  return AlphaBlend(IsDark(color) ? SK_ColorWHITE : SK_ColorBLACK, color,
                    alpha);
}

SkColor GetReadableColor(SkColor foreground, SkColor background) {
  return PickContrastingColor(foreground, LightnessInvertColor(foreground),
                              background);
}

SkColor PickContrastingColor(SkColor foreground1,
                             SkColor foreground2,
                             SkColor background) {
  const double background_luminance = GetRelativeLuminance(background);
  return (GetContrastRatio(GetRelativeLuminance(foreground1),
                           background_luminance) >=
          GetContrastRatio(GetRelativeLuminance(foreground2),
                           background_luminance)) ?
      foreground1 : foreground2;
}

SkColor InvertColor(SkColor color) {
  return SkColorSetARGB(SkColorGetA(color), 255 - SkColorGetR(color),
                        255 - SkColorGetG(color), 255 - SkColorGetB(color));
}

SkColor GetSysSkColor(int which) {
#if defined(OS_WIN)
  return skia::COLORREFToSkColor(GetSysColor(which));
#else
  NOTIMPLEMENTED();
  return SK_ColorLTGRAY;
#endif
}

// OS_WIN implementation lives in sys_color_change_listener.cc
#if !defined(OS_WIN)
bool IsInvertedColorScheme() {
  return false;
}
#endif  // !defined(OS_WIN)

SkColor DeriveDefaultIconColor(SkColor text_color) {
  // Lighten dark colors and brighten light colors. The alpha value here (0x4c)
  // is chosen to generate a value close to GoogleGrey700 from GoogleGrey900.
  return BlendTowardOppositeLuma(text_color, 0x4c);
}

std::string SkColorToRgbaString(SkColor color) {
  // We convert the alpha using NumberToString because StringPrintf will use
  // locale specific formatters (e.g., use , instead of . in German).
  return base::StringPrintf(
      "rgba(%s,%s)", SkColorToRgbString(color).c_str(),
      base::NumberToString(SkColorGetA(color) / 255.0).c_str());
}

std::string SkColorToRgbString(SkColor color) {
  return base::StringPrintf("%d,%d,%d", SkColorGetR(color), SkColorGetG(color),
                            SkColorGetB(color));
}

}  // namespace color_utils
