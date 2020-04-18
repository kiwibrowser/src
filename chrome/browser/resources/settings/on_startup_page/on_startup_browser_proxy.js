// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @typedef {{id: string, name: string, canBeDisabled: boolean}} */
let NtpExtension;

cr.define('settings', function() {
  /** @interface */
  class OnStartupBrowserProxy {
    /** @return {!Promise<?NtpExtension>} */
    getNtpExtension() {}
  }

  /**
   * @implements {settings.OnStartupBrowserProxy}
   */
  class OnStartupBrowserProxyImpl {
    /** @override */
    getNtpExtension() {
      return cr.sendWithPromise('getNtpExtension');
    }
  }

  cr.addSingletonGetter(OnStartupBrowserProxyImpl);

  return {
    OnStartupBrowserProxy: OnStartupBrowserProxy,
    OnStartupBrowserProxyImpl: OnStartupBrowserProxyImpl,
  };
});
