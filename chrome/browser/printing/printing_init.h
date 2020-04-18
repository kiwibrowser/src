// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRINTING_PRINTING_INIT_H__
#define CHROME_BROWSER_PRINTING_PRINTING_INIT_H__

namespace content {
class WebContents;
}

namespace printing {

// Initialize printing related classes for web contents.
void InitializePrinting(content::WebContents* web_contents);

}  // namespace printing

#endif  // CHROME_BROWSER_PRINTING_PRINTING_INIT_H__
