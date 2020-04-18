// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_TILES_TILE_VISUAL_TYPE_H_
#define COMPONENTS_NTP_TILES_TILE_VISUAL_TYPE_H_

namespace ntp_tiles {

// The visual type of an NTP tile.
//
// These values must stay in sync with the NTPTileVisualType enum in
// histograms/enums.xml.
//
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.suggestions
enum TileVisualType {
  // The icon or thumbnail hasn't loaded yet.
  NONE = 0,
  // The item displays a site's actual favicon or touch icon.
  ICON_REAL = 1,
  // The item displays a color derived from the site's favicon or touch icon.
  ICON_COLOR = 2,
  // The item displays a default gray box in place of an icon.
  ICON_DEFAULT = 3,
  // Deleted: THUMBNAIL_LOCAL = 4
  // Deleted: THUMBNAIL_SERVER = 5
  // Deleted: THUMBNAIL_DEFAULT = 6
  // The item displays a thumbnail of the page. Used on desktop.
  THUMBNAIL = 7,
  // The item wants to display a thumbnail of the page, but it failed to load.
  // Used on desktop.
  THUMBNAIL_FAILED = 8,
  // The maximum tile type value that gets recorded in UMA.
  LAST_RECORDED_TILE_TYPE = THUMBNAIL_FAILED,

  // The tile type has not been determined yet. Used on iOS, until we can detect
  // when all tiles have loaded.
  UNKNOWN_TILE_TYPE,

  TILE_TYPE_MAX = UNKNOWN_TILE_TYPE
};

}  // namespace ntp_tiles

#endif  // COMPONENTS_NTP_TILES_TILE_VISUAL_TYPE_H_
