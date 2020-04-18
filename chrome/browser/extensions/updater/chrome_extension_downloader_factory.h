// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_UPDATER_CHROME_EXTENSION_DOWNLOADER_FACTORY_H_
#define CHROME_BROWSER_EXTENSIONS_UPDATER_CHROME_EXTENSION_DOWNLOADER_FACTORY_H_

#include <memory>

class Profile;

namespace extensions {
class ExtensionDownloader;
class ExtensionDownloaderDelegate;
}

namespace net {
class URLRequestContextGetter;
}

namespace service_manager {
class Connector;
}

// This provides a simple static interface for constructing an
// ExtensionDownloader suitable for use from within Chrome.
class ChromeExtensionDownloaderFactory {
 public:
  // Creates a downloader with the given request context. No profile identity
  // is associated with this downloader.
  static std::unique_ptr<extensions::ExtensionDownloader>
  CreateForRequestContext(net::URLRequestContextGetter* request_context,
                          extensions::ExtensionDownloaderDelegate* delegate,
                          service_manager::Connector* connector);

  // Creates a downloader for a given Profile. This downloader will be able
  // to authenticate as the signed-in user in the event that it's asked to
  // fetch a protected download.
  static std::unique_ptr<extensions::ExtensionDownloader> CreateForProfile(
      Profile* profile,
      extensions::ExtensionDownloaderDelegate* delegate);
};

#endif  // CHROME_BROWSER_EXTENSIONS_UPDATER_CHROME_EXTENSION_DOWNLOADER_FACTORY_H_
