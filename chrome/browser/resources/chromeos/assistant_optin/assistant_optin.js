// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// <include src="../login/oobe_types.js">
// <include src="../login/oobe_buttons.js">
// <include src="../login/oobe_change_picture.js">
// <include src="../login/oobe_dialog.js">
// <include src="assistant_value_prop.js">
// <include src="setting_zippy.js">

cr.define('assistantOptin', function() {
  return {

    /**
     * Starts the assistant opt-in flow.
     */
    show: function() {
      $('value-prop-md').locale = loadTimeData.getString('locale');
      $('value-prop-md').onShow();
      chrome.send('initialized');
    },

    /**
     * Reloads localized strings.
     * @param {!Object} data New dictionary with i18n values.
     */
    reloadContent: function(data) {
      // Reload global local strings, process DOM tree again.
      loadTimeData.overrideValues(data);
      i18nTemplate.process(document, loadTimeData);
      $('value-prop-md').reloadContent(data);
    },

    /**
     * Add a setting zippy object in the value prop screen.
     * @param {!Object} data String and url for the setting zippy.
     */
    addSettingZippy: function(data) {
      $('value-prop-md').addSettingZippy(data);
    },
  };
});

document.addEventListener('DOMContentLoaded', function() {
  assistantOptin.show();
});
