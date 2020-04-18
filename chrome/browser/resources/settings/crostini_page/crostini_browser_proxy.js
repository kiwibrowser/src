// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used by the "Linux Apps" (Crostini) section
 * to install and uninstall Crostini.
 */
cr.define('settings', function() {
  /** @interface */
  class CrostiniBrowserProxy {
    requestCrostiniInstallerView() {}
    requestRemoveCrostini() {}
  }

  /** @implements {settings.CrostiniBrowserProxy} */
  class CrostiniBrowserProxyImpl {
    /** @override */
    requestCrostiniInstallerView() {
      chrome.send('requestCrostiniInstallerView');
    }

    /** @override */
    requestRemoveCrostini() {
      chrome.send('requestRemoveCrostini');
    }
  }

  // The singleton instance_ can be replaced with a test version of this wrapper
  // during testing.
  cr.addSingletonGetter(CrostiniBrowserProxyImpl);

  return {
    CrostiniBrowserProxy: CrostiniBrowserProxy,
    CrostiniBrowserProxyImpl: CrostiniBrowserProxyImpl,
  };
});
