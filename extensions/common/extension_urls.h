// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_EXTENSION_URLS_H_
#define EXTENSIONS_COMMON_EXTENSION_URLS_H_

#include <string>

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "url/gurl.h"

namespace url {
class Origin;
}

namespace extensions {

// The name of the event_bindings module.
extern const char kEventBindings[];

// The name of the schemaUtils module.
extern const char kSchemaUtils[];

// Determine whether or not a source came from an extension. |source| can link
// to a page or a script, and can be external (e.g., "http://www.google.com"),
// extension-related (e.g., "chrome-extension://<extension_id>/background.js"),
// or internal (e.g., "event_bindings" or "schemaUtils").
bool IsSourceFromAnExtension(const base::string16& source);

}  // namespace extensions

namespace extension_urls {

// Canonical URLs for the Chrome Webstore. You probably want to use one of
// the calls below rather than using one of these constants directly, since
// the active extensions embedder may provide its own webstore URLs.
extern const char kChromeWebstoreBaseURL[];
extern const char kChromeWebstoreUpdateURL[];

// Returns the URL prefix for the extension/apps gallery. Can be set via the
// --apps-gallery-url switch. The URL returned will not contain a trailing
// slash. Do not use this as a prefix/extent for the store.
GURL GetWebstoreLaunchURL();

// Returns the URL to the extensions category on the Web Store. This is
// derived from GetWebstoreLaunchURL().
std::string GetWebstoreExtensionsCategoryURL();

// Returns the URL prefix for an item in the extension/app gallery. This URL
// will contain a trailing slash and should be concatenated with an item ID
// to get the item detail URL.
std::string GetWebstoreItemDetailURLPrefix();

// Returns the URL used to get webstore data (ratings, manifest, icon URL,
// etc.) about an extension from the webstore as JSON.
GURL GetWebstoreItemJsonDataURL(const std::string& extension_id);

// Returns the URL used to get webstore search results in JSON format. The URL
// returns a JSON dictionary that has the search results (under "results").
// Each entry in the array is a dictionary as the data returned for
// GetWebstoreItemJsonDataURL above. |query| is the user typed query string.
// |host_language_code| is the host language code, e.g. en_US. Both arguments
// will be escaped and added as a query parameter to the returned web store
// json search URL.
GURL GetWebstoreJsonSearchUrl(const std::string& query,
                              const std::string& host_language_code);

// Returns the URL of the web store search results page for |query|.
GURL GetWebstoreSearchPageUrl(const std::string& query);

// Returns the compile-time constant webstore update url specific to
// Chrome. Usually you should prefer using GetWebstoreUpdateUrl.
GURL GetDefaultWebstoreUpdateUrl();

// Return the update URL used by gallery/webstore extensions/apps. This may
// have been overridden by a command line flag for testing purposes.
GURL GetWebstoreUpdateUrl();

// Returns the url to visit to report abuse for the given |extension_id|
// and |referrer_id|.
GURL GetWebstoreReportAbuseUrl(const std::string& extension_id,
                               const std::string& referrer_id);

// Returns whether the URL is the webstore update URL (just considering host
// and path, not scheme, query, etc.)
bool IsWebstoreUpdateUrl(const GURL& update_url);

// Returns true if the URL points to an extension blacklist.
bool IsBlacklistUpdateUrl(const GURL& url);

// Returns true if the origin points to an URL used for safebrowsing.
// TODO(devlin): Update other methods to also take an url::Origin?
bool IsSafeBrowsingUrl(const url::Origin& origin, base::StringPiece path);

}  // namespace extension_urls

#endif  // EXTENSIONS_COMMON_EXTENSION_URLS_H_
