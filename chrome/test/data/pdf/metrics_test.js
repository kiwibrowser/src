// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests(function() {
  'use strict';

  class MockMetricsPrivate {
    constructor() {
      this.MetricTypeType = {HISTOGRAM_LOG: 'test_histogram_log'};
      this.actionCounter = {};
    }

    recordValue(metric, value) {
      chrome.test.assertEq('PDF.Actions', metric.metricName);
      chrome.test.assertEq('test_histogram_log', metric.type);
      chrome.test.assertEq(1, metric.min);
      chrome.test.assertEq(
          window.PDFMetrics.UserAction.NUMBER_OF_ACTIONS, metric.max);
      chrome.test.assertEq(
          window.PDFMetrics.UserAction.NUMBER_OF_ACTIONS + 1, metric.buckets);
      this.actionCounter[value] = (this.actionCounter[value] + 1) || 1;
    }
  };

  return [
    function testMetricsDocumentOpened() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetricsImpl();
      metrics.onDocumentOpened();

      chrome.test.assertEq(
          {[window.PDFMetrics.UserAction.DOCUMENT_OPENED]: 1},
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },

    function testMetricsRotation() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetricsImpl();
      metrics.onDocumentOpened();
      for (var i = 0; i < 4; i++)
        metrics.onRotation();

      chrome.test.assertEq(
          {
            [window.PDFMetrics.UserAction.DOCUMENT_OPENED]: 1,
            [window.PDFMetrics.UserAction.ROTATE_FIRST]: 1,
            [window.PDFMetrics.UserAction.ROTATE]: 4
          },
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },

    function testMetricsFitTo() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetricsImpl();
      metrics.onDocumentOpened();
      metrics.onFitTo(FittingType.FIT_TO_HEIGHT);
      metrics.onFitTo(FittingType.FIT_TO_PAGE);
      metrics.onFitTo(FittingType.FIT_TO_WIDTH);
      metrics.onFitTo(FittingType.FIT_TO_PAGE);
      metrics.onFitTo(FittingType.FIT_TO_WIDTH);
      metrics.onFitTo(FittingType.FIT_TO_PAGE);

      chrome.test.assertEq(
          {
            [window.PDFMetrics.UserAction.DOCUMENT_OPENED]: 1,
            [window.PDFMetrics.UserAction.FIT_TO_PAGE_FIRST]: 1,
            [window.PDFMetrics.UserAction.FIT_TO_PAGE]: 3,
            [window.PDFMetrics.UserAction.FIT_TO_WIDTH_FIRST]: 1,
            [window.PDFMetrics.UserAction.FIT_TO_WIDTH]: 2
          },
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },

    function testMetricsBookmarks() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetricsImpl();
      metrics.onDocumentOpened();

      metrics.onOpenBookmarksPanel();
      metrics.onFollowBookmark();
      metrics.onFollowBookmark();

      metrics.onOpenBookmarksPanel();
      metrics.onFollowBookmark();
      metrics.onFollowBookmark();
      metrics.onFollowBookmark();

      chrome.test.assertEq(
          {
            [window.PDFMetrics.UserAction.DOCUMENT_OPENED]: 1,
            [window.PDFMetrics.UserAction.OPEN_BOOKMARKS_PANEL_FIRST]: 1,
            [window.PDFMetrics.UserAction.OPEN_BOOKMARKS_PANEL]: 2,
            [window.PDFMetrics.UserAction.FOLLOW_BOOKMARK_FIRST]: 1,
            [window.PDFMetrics.UserAction.FOLLOW_BOOKMARK]: 5
          },
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },

    function testMetricsPageSelector() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetricsImpl();
      metrics.onDocumentOpened();

      metrics.onPageSelectorNavigation();
      metrics.onPageSelectorNavigation();

      chrome.test.assertEq(
          {
            [PDFMetrics.UserAction.DOCUMENT_OPENED]: 1,
            [PDFMetrics.UserAction.PAGE_SELECTOR_NAVIGATE_FIRST]: 1,
            [PDFMetrics.UserAction.PAGE_SELECTOR_NAVIGATE]: 2
          },
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },
  ];
}());
