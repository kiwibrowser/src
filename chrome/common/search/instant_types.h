// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_SEARCH_INSTANT_TYPES_H_
#define CHROME_COMMON_SEARCH_INSTANT_TYPES_H_

#include <stdint.h>

#include <string>
#include <utility>

#include "base/strings/string16.h"
#include "base/time/time.h"
#include "components/ntp_tiles/tile_source.h"
#include "components/ntp_tiles/tile_title_source.h"
#include "url/gurl.h"

// ID used by Instant code to refer to objects (e.g. Autocomplete results, Most
// Visited items) that the Instant page needs access to.
typedef int InstantRestrictedID;

// The alignment of the theme background image.
enum ThemeBackgroundImageAlignment {
  THEME_BKGRND_IMAGE_ALIGN_CENTER,
  THEME_BKGRND_IMAGE_ALIGN_LEFT,
  THEME_BKGRND_IMAGE_ALIGN_TOP,
  THEME_BKGRND_IMAGE_ALIGN_RIGHT,
  THEME_BKGRND_IMAGE_ALIGN_BOTTOM,

  THEME_BKGRND_IMAGE_ALIGN_LAST = THEME_BKGRND_IMAGE_ALIGN_BOTTOM,
};

// The tiling of the theme background image.
enum ThemeBackgroundImageTiling {
  THEME_BKGRND_IMAGE_NO_REPEAT,
  THEME_BKGRND_IMAGE_REPEAT_X,
  THEME_BKGRND_IMAGE_REPEAT_Y,
  THEME_BKGRND_IMAGE_REPEAT,

  THEME_BKGRND_IMAGE_LAST = THEME_BKGRND_IMAGE_REPEAT,
};

// The RGBA color components for the text and links of the theme.
struct RGBAColor {
  RGBAColor();
  RGBAColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
  ~RGBAColor();

  bool operator==(const RGBAColor& rhs) const;

  // The color in RGBA format where the R, G, B and A values
  // are between 0 and 255 inclusive and always valid.
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
};

// Theme background settings for the NTP.
struct ThemeBackgroundInfo {
  ThemeBackgroundInfo();
  ~ThemeBackgroundInfo();

  bool operator==(const ThemeBackgroundInfo& rhs) const;

  // True if the default theme is selected.
  bool using_default_theme;

  // Url of the custom background selected by the user.
  GURL custom_background_url;

  // The theme background color in RGBA format always valid.
  RGBAColor background_color;

  // The theme text color in RGBA format.
  RGBAColor text_color;

  // The theme link color in RGBA format.
  RGBAColor link_color;

  // The theme text color light in RGBA format.
  RGBAColor text_color_light;

  // The theme color for the header in RGBA format.
  RGBAColor header_color;

  // The theme color for the section border in RGBA format.
  RGBAColor section_border_color;

  // The theme id for the theme background image.
  // Value is only valid if there's a custom theme background image.
  std::string theme_id;

  // The theme background image horizontal alignment is only valid if |theme_id|
  // is valid.
  ThemeBackgroundImageAlignment image_horizontal_alignment;

  // The theme background image vertical alignment is only valid if |theme_id|
  // is valid.
  ThemeBackgroundImageAlignment image_vertical_alignment;

  // The theme background image tiling is only valid if |theme_id| is valid.
  ThemeBackgroundImageTiling image_tiling;

  // True if theme has attribution logo.
  // Value is only valid if |theme_id| is valid.
  bool has_attribution;

  // True if theme has an alternate logo.
  bool logo_alternate;
};

struct InstantMostVisitedItem {
  InstantMostVisitedItem();
  InstantMostVisitedItem(const InstantMostVisitedItem& other);
  ~InstantMostVisitedItem();

  // The URL of the Most Visited item.
  GURL url;

  // The title of the Most Visited page.  May be empty, in which case the |url|
  // is used as the title.
  base::string16 title;

  // The external URL of the thumbnail associated with this page.
  GURL thumbnail;

  // The external URL of the favicon associated with this page.
  GURL favicon;

  // The source of the item's |title|.
  ntp_tiles::TileTitleSource title_source;

  // The source of the item, e.g. server-side or client-side.
  ntp_tiles::TileSource source;

  // The timestamp representing when the tile data (e.g. URL) was generated
  // originally, regardless of the impression timestamp.
  base::Time data_generation_time;
};

// An InstantMostVisitedItem along with its assigned restricted ID.
typedef std::pair<InstantRestrictedID, InstantMostVisitedItem>
    InstantMostVisitedItemIDPair;

#endif  // CHROME_COMMON_SEARCH_INSTANT_TYPES_H_
