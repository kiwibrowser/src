// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/search/instant_types.h"

RGBAColor::RGBAColor()
    : r(0),
      g(0),
      b(0),
      a(0) {
}

RGBAColor::RGBAColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : r(r), g(g), b(b), a(a) {}

RGBAColor::~RGBAColor() {
}

bool RGBAColor::operator==(const RGBAColor& rhs) const {
  return r == rhs.r &&
      g == rhs.g &&
      b == rhs.b &&
      a == rhs.a;
}

ThemeBackgroundInfo::ThemeBackgroundInfo()
    : using_default_theme(true),
      custom_background_url(std::string()),
      background_color(),
      text_color(),
      link_color(),
      text_color_light(),
      header_color(),
      section_border_color(),
      image_horizontal_alignment(THEME_BKGRND_IMAGE_ALIGN_CENTER),
      image_vertical_alignment(THEME_BKGRND_IMAGE_ALIGN_CENTER),
      image_tiling(THEME_BKGRND_IMAGE_NO_REPEAT),
      has_attribution(false),
      logo_alternate(false) {}

ThemeBackgroundInfo::~ThemeBackgroundInfo() {
}

bool ThemeBackgroundInfo::operator==(const ThemeBackgroundInfo& rhs) const {
  return using_default_theme == rhs.using_default_theme &&
         custom_background_url == rhs.custom_background_url &&
         background_color == rhs.background_color &&
         text_color == rhs.text_color && link_color == rhs.link_color &&
         text_color_light == rhs.text_color_light &&
         header_color == rhs.header_color &&
         section_border_color == rhs.section_border_color &&
         theme_id == rhs.theme_id &&
         image_horizontal_alignment == rhs.image_horizontal_alignment &&
         image_vertical_alignment == rhs.image_vertical_alignment &&
         image_tiling == rhs.image_tiling &&
         has_attribution == rhs.has_attribution &&
         logo_alternate == rhs.logo_alternate;
}

InstantMostVisitedItem::InstantMostVisitedItem()
    : title_source(ntp_tiles::TileTitleSource::UNKNOWN),
      source(ntp_tiles::TileSource::TOP_SITES) {}

InstantMostVisitedItem::InstantMostVisitedItem(
    const InstantMostVisitedItem& other) = default;

InstantMostVisitedItem::~InstantMostVisitedItem() {}
