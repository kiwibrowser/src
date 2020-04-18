// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings', function() {
  /** @interface */
  class DownloadsBrowserProxy {
    initializeDownloads() {}
    selectDownloadLocation() {}
    resetAutoOpenFileTypes() {}
  }

  /**
   * @implements {settings.DownloadsBrowserProxy}
   */
  class DownloadsBrowserProxyImpl {
    /** @override */
    initializeDownloads() {
      chrome.send('initializeDownloads');
    }

    /** @override */
    selectDownloadLocation() {
      chrome.send('selectDownloadLocation');
    }

    /** @override */
    resetAutoOpenFileTypes() {
      chrome.send('resetAutoOpenFileTypes');
    }
  }

  cr.addSingletonGetter(DownloadsBrowserProxyImpl);

  return {
    DownloadsBrowserProxy: DownloadsBrowserProxy,
    DownloadsBrowserProxyImpl: DownloadsBrowserProxyImpl,
  };
});
