// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @implements {settings.PrivacyPageBrowserProxy} */
class TestPrivacyPageBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'getMetricsReporting',
      'getSafeBrowsingExtendedReporting',
      'setMetricsReportingEnabled',
      'setSafeBrowsingExtendedReportingEnabled',
      'showManageSSLCertificates',
    ]);

    /** @type {!MetricsReporting} */
    this.metricsReporting = {
      enabled: true,
      managed: true,
    };

    /** @type {!SberPrefState} */
    this.sberPrefState = {
      enabled: true,
      managed: true,
    };
  }

  /** @override */
  getMetricsReporting() {
    this.methodCalled('getMetricsReporting');
    return Promise.resolve(this.metricsReporting);
  }

  /** @override */
  setMetricsReportingEnabled(enabled) {
    this.methodCalled('setMetricsReportingEnabled', enabled);
  }

  /** @override */
  showManageSSLCertificates() {
    this.methodCalled('showManageSSLCertificates');
  }

  /** @override */
  getSafeBrowsingExtendedReporting() {
    this.methodCalled('getSafeBrowsingExtendedReporting');
    return Promise.resolve(this.sberPrefState);
  }

  /** @override */
  setSafeBrowsingExtendedReportingEnabled(enabled) {
    this.methodCalled('setSafeBrowsingExtendedReportingEnabled', enabled);
  }
}
