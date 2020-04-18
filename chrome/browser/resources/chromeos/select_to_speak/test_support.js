// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Stubs out extension API functions so that SelectToSpeakUnitTest
 * can load.
 */

chrome.automation = {};

/**
 * Stub
 */
chrome.automation.getDesktop = function() {};

/**
 * Set necessary constants.
 */
chrome.automation.RoleType = {
  WINDOW: 'window',
  ROOT_WEB_AREA: 'rootWebArea',
  STATIC_TEXT: 'staticText',
  INLINE_TEXT_BOX: 'inlineTextBox',
  PARAGRAPH: 'paragraph',
  TEXT_FIELD: 'textField',
};

chrome.automation.StateType = {
  INVISIBLE: 'invisible'
};

chrome.metricsPrivate = {
  recordUserAction: function() {},
  recordValue: function() {},
  MetricTypeType: {HISTOGRAM_LINEAR: 1}
};

chrome.commandLinePrivate = {
  hasSwitch: function() {}
};

chrome.accessibilityPrivate = {};

chrome.accessibilityPrivate.SelectToSpeakState = {
  INACTIVE: 'inactive',
  SELECTING: 'selecting',
  SPEAKING: 'speaking'
};
