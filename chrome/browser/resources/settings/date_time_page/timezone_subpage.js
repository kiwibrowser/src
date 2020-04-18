// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'timezone-subpage' is the collapsible section containing
 * time zone settings.
 */
Polymer({
  is: 'timezone-subpage',

  behaviors: [PrefsBehavior],

  properties: {
    /**
     * This is <timezone-selector> parameter.
     */
    activeTimeZoneDisplayName: {
      type: String,
      notify: true,
    },
  },

  /**
   * Returns value list for timeZoneResolveMethodDropdown menu.
   * @private
   */
  getTimeZoneResolveMethodsList_: function() {
    let result = [];
    // Make sure current value is in the list, even if it is not
    // user-selectable.
    if (this.getPref('generated.resolve_timezone_by_geolocation_method_short')
            .value == settings.TimeZoneAutoDetectMethod.DISABLED) {
      result.push({
        value: settings.TimeZoneAutoDetectMethod.DISABLED,
        name: loadTimeData.getString('setTimeZoneAutomaticallyDisabled')
      });
    }
    result.push({
      value: settings.TimeZoneAutoDetectMethod.IP_ONLY,
      name: loadTimeData.getString('setTimeZoneAutomaticallyIpOnlyDefault')
    });

    if (this.getPref('generated.resolve_timezone_by_geolocation_method_short')
            .value ==
        settings.TimeZoneAutoDetectMethod.SEND_WIFI_ACCESS_POINTS) {
      result.push({
        value: settings.TimeZoneAutoDetectMethod.SEND_WIFI_ACCESS_POINTS,
        name: loadTimeData.getString(
            'setTimeZoneAutomaticallyWithWiFiAccessPointsData')
      });
    }
    result.push({
      value: settings.TimeZoneAutoDetectMethod.SEND_ALL_LOCATION_INFO,
      name:
          loadTimeData.getString('setTimeZoneAutomaticallyWithAllLocationInfo')
    });
    return result;
  },

});
