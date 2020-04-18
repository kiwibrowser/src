// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSER_ABOUT_HANDLER_H_
#define CHROME_BROWSER_BROWSER_ABOUT_HANDLER_H_

class GURL;

namespace content {
class BrowserContext;
}

// A preliminary URLHandler that performs cleanup on the URL before it is
// rewritten.  Changes that happen here will not lead to a virtual URL.
bool FixupBrowserAboutURL(GURL* url, content::BrowserContext* browser_context);

// Returns true if the given URL will be handled by the browser about handler.
// Nowadays, these go through the webui, so the return is always false.
// Either way, |url| will be processed by url_formatter::FixupURL, which
// replaces the about: scheme with chrome:// for all about:foo URLs except
// "about:blank".
// Some |url| host values will be replaced with their respective redirects.
//
// This is used by BrowserURLHandler.
bool WillHandleBrowserAboutURL(GURL* url,
                               content::BrowserContext* browser_context);

// We have a few magic commands that don't cause navigations, but rather pop up
// dialogs. This function handles those cases, and returns true if so. In this
// case, normal tab navigation should be skipped.
bool HandleNonNavigationAboutURL(const GURL& url);

#endif  // CHROME_BROWSER_BROWSER_ABOUT_HANDLER_H_
