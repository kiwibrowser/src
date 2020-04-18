// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_PUBLIC_PROVIDER_CHROME_BROWSER_IMAGES_BRANDED_IMAGE_ICON_TYPES_H_
#define IOS_PUBLIC_PROVIDER_CHROME_BROWSER_IMAGES_BRANDED_IMAGE_ICON_TYPES_H_

// Possible icons for the what's new promo.
enum WhatsNewIcon {
  WHATS_NEW_INFO = 0,  // A circled 'i'. Default value if no icon is specified
                       // in promo.
  WHATS_NEW_LOGO,      // The application's logo.
  WHATS_NEW_LOGO_ROUNDED_RECTANGLE,  // The application's logo with a rounded
                                     // corner rectangle surrounding it.
};

// Enum used to represent the different type of search engine icons.
enum SearchEngineIcon {
  SEARCH_ENGINE_ICON_OTHER,
  SEARCH_ENGINE_ICON_GOOGLE_SEARCH,
};

#endif  // IOS_PUBLIC_PROVIDER_CHROME_BROWSER_IMAGES_BRANDED_IMAGE_ICON_TYPES_H_
