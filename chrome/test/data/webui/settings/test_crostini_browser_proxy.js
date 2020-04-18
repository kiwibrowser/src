// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @implements {settings.CrostiniBrowserProxy} */
class TestCrostiniBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'requestCrostiniInstallerView',
      'requestRemoveCrostini',
    ]);
    this.prefs = {crostini: {enabled: {value: false}}};
  }

  /** @override */
  requestCrostiniInstallerView() {
    this.methodCalled('requestCrostiniInstallerView');
    this.setCrostiniEnabledValue(true);
  }

  /** override */
  requestRemoveCrostini() {
    this.methodCalled('requestRemoveCrostini');
    this.setCrostiniEnabledValue(false);
  }

  setCrostiniEnabledValue(newValue) {
    this.prefs = {crostini: {enabled: {value: newValue}}};
  }
}
