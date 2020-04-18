// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('sync_test_util', function() {
  /** @param {!settings.SyncStatus} */
  function simulateSyncStatus(status) {
    cr.webUIListenerCallback('sync-status-changed', status);
    Polymer.dom.flush();
  }

  /** @param {Array<!settings.StoredAccount>} */
  function simulateStoredAccounts(accounts) {
    cr.webUIListenerCallback('stored-accounts-updated', accounts);
    Polymer.dom.flush();
  }

  return {
    simulateSyncStatus: simulateSyncStatus,
    simulateStoredAccounts: simulateStoredAccounts,
  };
});