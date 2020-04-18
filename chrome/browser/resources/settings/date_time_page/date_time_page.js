// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-date-time-page' is the settings page containing date and time
 * settings.
 */

Polymer({
  is: 'settings-date-time-page',

  behaviors: [I18nBehavior, PrefsBehavior, WebUIListenerBehavior],

  properties: {
    /**
     * Whether date and time are settable. Normally the date and time are forced
     * by network time, so default to false to initially hide the button.
     * @private
     */
    canSetDateTime_: {
      type: Boolean,
      value: false,
    },

    /**
     * This is used to get current time zone display name from
     * <timezone-selector> via bi-directional binding.
     */
    activeTimeZoneDisplayName: {
      type: String,
      value: loadTimeData.getString('timeZoneName'),
    },

    /** @private {!Map<string, string>} */
    focusConfig_: {
      type: Object,
      value: function() {
        const map = new Map();
        if (settings.routes.DATETIME_TIMEZONE_SUBPAGE)
          map.set(
              settings.routes.DATETIME_TIMEZONE_SUBPAGE.path,
              '#timeZoneSettingsTrigger .subpage-arrow');
        return map;
      },
    },
  },

  /** @override */
  attached: function() {
    this.addWebUIListener(
        'can-set-date-time-changed', this.onCanSetDateTimeChanged_.bind(this));

    chrome.send('dateTimePageReady');
  },

  /**
   * @param {boolean} canSetDateTime Whether date and time are settable.
   * @private
   */
  onCanSetDateTimeChanged_: function(canSetDateTime) {
    this.canSetDateTime_ = canSetDateTime;
  },

  /** @private */
  onSetDateTimeTap_: function() {
    chrome.send('showSetDateTimeUI');
  },

  /**
   * Returns display name of the given time zone detection method.
   * @param {settings.TimeZoneAutoDetectMethod} method
   *     prefs.generated.resolve_timezone_by_geolocation_method_short.value
   * @return {string}
   * @private
   */
  getTimeZoneAutoDetectMethodDisplayName_: function(method) {
    const id = ([
      'setTimeZoneAutomaticallyDisabled',
      'setTimeZoneAutomaticallyIpOnlyDefault',
      'setTimeZoneAutomaticallyWithWiFiAccessPointsData',
      'setTimeZoneAutomaticallyWithAllLocationInfo'
    ])[method];
    if (id)
      return this.i18n(id);

    return '';
  },

  onTimeZoneSettings_: function() {
    settings.navigateTo(settings.routes.DATETIME_TIMEZONE_SUBPAGE);
  },
});
