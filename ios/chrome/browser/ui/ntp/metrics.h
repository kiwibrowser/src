// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_METRICS_H_
#define IOS_CHROME_BROWSER_UI_NTP_METRICS_H_

#include "base/time/time.h"
#include "components/ntp_tiles/tile_source.h"
#include "components/ntp_tiles/tile_title_source.h"
#import "ios/chrome/browser/ui/favicon/favicon_attributes.h"
#include "url/gurl.h"

void RecordNTPTileImpression(int index,
                             ntp_tiles::TileSource source,
                             ntp_tiles::TileTitleSource title_source,
                             const FaviconAttributes* attributes,
                             base::Time data_generation_time,
                             const GURL& url);

void RecordNTPTileClick(int index,
                        ntp_tiles::TileSource source,
                        ntp_tiles::TileTitleSource title_source,
                        const FaviconAttributes* attributes,
                        base::Time data_generation_time,
                        const GURL& url);

#endif  // IOS_CHROME_BROWSER_UI_NTP_METRICS_H_
