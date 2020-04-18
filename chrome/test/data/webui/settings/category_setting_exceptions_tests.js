// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for category-setting-exceptions. */
suite('CategorySettingExceptions', function() {
  /**
   * A site settings exceptions created before each test.
   * @type {SiteSettingsExceptionsElement}
   */
  let testElement;

  // Initialize a category-setting-exceptions before each test.
  setup(function() {
    browserProxy = new TestSiteSettingsPrefsBrowserProxy();
    settings.SiteSettingsPrefsBrowserProxyImpl.instance_ = browserProxy;
    PolymerTest.clearBody();
    testElement = document.createElement('category-setting-exceptions');
    document.body.appendChild(testElement);
  });

  test('create category-setting-exceptions', function() {
    // The category-setting-exceptions is mainly a container for site-lists.
    // There's not much that merits testing.
    assertTrue(!!testElement);
  });
});
