// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('reset_page', function() {
  /** @implements {settings.ResetBrowserProxy} */
  class TestResetBrowserProxy extends TestBrowserProxy {
    constructor() {
      super([
        'performResetProfileSettings',
        'onHideResetProfileDialog',
        'onHideResetProfileBanner',
        'onShowResetProfileDialog',
        'showReportedSettings',
        'getTriggeredResetToolName',
        'onPowerwashDialogShow',
      ]);
    }

    /** @override */
    performResetProfileSettings(sendSettings, requestOrigin) {
      this.methodCalled('performResetProfileSettings', requestOrigin);
      return Promise.resolve();
    }

    /** @override */
    onHideResetProfileDialog() {
      this.methodCalled('onHideResetProfileDialog');
    }

    /** @override */
    onHideResetProfileBanner() {
      this.methodCalled('onHideResetProfileBanner');
    }

    /** @override */
    onShowResetProfileDialog() {
      this.methodCalled('onShowResetProfileDialog');
    }

    /** @override */
    showReportedSettings() {
      this.methodCalled('showReportedSettings');
    }

    /** @override */
    getTriggeredResetToolName() {
      this.methodCalled('getTriggeredResetToolName');
      return Promise.resolve('WonderfulAV');
    }

    /** @override */
    onPowerwashDialogShow() {
      this.methodCalled('onPowerwashDialogShow');
    }
  }

  return {
    TestResetBrowserProxy: TestResetBrowserProxy,
  };
});
