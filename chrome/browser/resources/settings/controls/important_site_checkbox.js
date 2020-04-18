// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * `important-site-checkbox` is a checkbox that displays an important site.
 * An important site represents a domain with data that the user might want
 * to protect from being deleted. ImportantSites are determined based on
 * various engagement factors, such as whether a site is bookmarked or receives
 * notifications.
 */
Polymer({
  is: 'important-site-checkbox',

  properties: {
    /** @type {ImportantSite} */
    site: Object,

    disabled: {type: Boolean, value: false},
  }
});
