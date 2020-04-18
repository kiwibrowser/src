// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_SSL_INSECURE_CONTENT_H_
#define CHROME_COMMON_SSL_INSECURE_CONTENT_H_

class GURL;

// Insecure content types used in the SSL.InsecureContent histogram.
// This enum is histogrammed, so do not add, reorder, or remove values.
enum class SslInsecureContentType {
  DISPLAY = 0,
  DISPLAY_HOST_GOOGLE,      // deprecated
  DISPLAY_HOST_WWW_GOOGLE,  // deprecated
  DISPLAY_HTML,
  RUN,
  RUN_HOST_GOOGLE,      // deprecated
  RUN_HOST_WWW_GOOGLE,  // deprecated
  RUN_TARGET_YOUTUBE,   // deprecated
  RUN_JS,
  RUN_CSS,
  RUN_SWF,
  DISPLAY_HOST_YOUTUBE,           // deprecated
  RUN_HOST_YOUTUBE,               // deprecated
  RUN_HOST_GOOGLEUSERCONTENT,     // deprecated
  DISPLAY_HOST_MAIL_GOOGLE,       // deprecated
  RUN_HOST_MAIL_GOOGLE,           // deprecated
  DISPLAY_HOST_PLUS_GOOGLE,       // deprecated
  RUN_HOST_PLUS_GOOGLE,           // deprecated
  DISPLAY_HOST_DOCS_GOOGLE,       // deprecated
  RUN_HOST_DOCS_GOOGLE,           // deprecated
  DISPLAY_HOST_SITES_GOOGLE,      // deprecated
  RUN_HOST_SITES_GOOGLE,          // deprecated
  DISPLAY_HOST_PICASAWEB_GOOGLE,  // deprecated
  RUN_HOST_PICASAWEB_GOOGLE,      // deprecated
  DISPLAY_HOST_GOOGLE_READER,     // deprecated
  RUN_HOST_GOOGLE_READER,         // deprecated
  DISPLAY_HOST_CODE_GOOGLE,       // deprecated
  RUN_HOST_CODE_GOOGLE,           // deprecated
  DISPLAY_HOST_GROUPS_GOOGLE,     // deprecated
  RUN_HOST_GROUPS_GOOGLE,         // deprecated
  DISPLAY_HOST_MAPS_GOOGLE,       // deprecated
  RUN_HOST_MAPS_GOOGLE,           // deprecated
  DISPLAY_HOST_GOOGLE_SUPPORT,    // deprecated
  RUN_HOST_GOOGLE_SUPPORT,        // deprecated
  DISPLAY_HOST_GOOGLE_INTL,       // deprecated
  RUN_HOST_GOOGLE_INTL,           // deprecated
  NUM_EVENTS
};

// Reports insecure content to the SSL.InsecureContent histogram using the
// provided |signal|.
void ReportInsecureContent(SslInsecureContentType signal);

// Reports insecure content displayed or ran if |resource_URL| matches specific
// file types.
void FilteredReportInsecureContentDisplayed(const GURL& resource_gurl);
void FilteredReportInsecureContentRan(const GURL& resource_gurl);

#endif  // CHROME_COMMON_SSL_INSECURE_CONTENT_H_
