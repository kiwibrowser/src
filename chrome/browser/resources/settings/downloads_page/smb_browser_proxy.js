// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the "SMB Share" section to
 * interact with the browser. Used only on Chrome OS.
 */

/**
 *  @enum {number}
 *  These values must be kept in sync with the SmbMountResult enum in
 *  chrome/browser/chromeos/smb_client/smb_service.h.
 */
const SmbMountResult = {
  SUCCESS: 0,
  UNKNOWN_FAILURE: 1,
  AUTHENTICATION_FAILED: 2,
  NOT_FOUND: 3,
  UNSUPPORTED_DEVICE: 4,
  MOUNT_EXISTS: 5,
};

cr.define('settings', function() {
  /** @interface */
  class SmbBrowserProxy {
    /**
     * Attempts to mount an Smb filesystem with the provided url.
     * @param {string} smbUrl
     */
    smbMount(smbUrl, username, password) {}
  }

  /** @implements {settings.SmbBrowserProxy} */
  class SmbBrowserProxyImpl {
    /** @override */
    smbMount(smbUrl, username, password) {
      chrome.send('smbMount', [smbUrl, username, password]);
    }
  }

  cr.addSingletonGetter(SmbBrowserProxyImpl);

  return {
    SmbBrowserProxy: SmbBrowserProxy,
    SmbBrowserProxyImpl: SmbBrowserProxyImpl,
  };

});