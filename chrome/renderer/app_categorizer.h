// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_APP_CATEGORIZER_H_
#define CHROME_RENDERER_APP_CATEGORIZER_H_

class GURL;

class AppCategorizer {
 public:
  static bool IsHangoutsUrl(const GURL& url);
  static bool IsWhitelistedApp(const GURL& manifest_url, const GURL& app_url);
};

#endif  // CHROME_RENDERER_APP_CATEGORIZER_H_
