// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PRERENDER_FINAL_STATUS_H_
#define IOS_CHROME_BROWSER_UI_PRERENDER_FINAL_STATUS_H_

// PrerenderFinalStatus values are used in the "Prerender.FinalStatus" histogram
// and new values needs to be kept in sync with histogram.xml.
enum PrerenderFinalStatus {
  PRERENDER_FINAL_STATUS_USED = 0,
  PRERENDER_FINAL_STATUS_MEMORY_LIMIT_EXCEEDED = 12,
  PRERENDER_FINAL_STATUS_CANCELLED = 32,
  PRERENDER_FINAL_STATUS_MAX = 52,
};

#endif  // IOS_CHROME_BROWSER_UI_PRERENDER_FINAL_STATUS_H_
