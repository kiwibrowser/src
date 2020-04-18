// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Constants used for the declarativeContent API.

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_DECLARATIVE_CONSTANTS_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_DECLARATIVE_CONSTANTS_H_

namespace extensions {
namespace declarative_content_constants {

// Signals to which ContentRulesRegistries are registered.
extern const char kOnPageChanged[];

// Keys of dictionaries.
extern const char kAllFrames[];
extern const char kCss[];
extern const char kInstanceType[];
extern const char kIsBookmarked[];
extern const char kJs[];
extern const char kMatchAboutBlank[];
extern const char kPageUrl[];

// Values of dictionaries, in particular instance types
extern const char kPageStateMatcherType[];
extern const char kShowPageAction[];
extern const char kRequestContentScript[];
extern const char kSetIcon[];

}  // namespace declarative_content_constants
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_DECLARATIVE_CONSTANTS_H_
