// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Encapsulates all settings and logic related to the media size selection UI.
   * @param {!print_preview.ticket_items.MediaSize} ticketItem Used to read and
   *     write the media size ticket item.
   * @constructor
   * @extends {print_preview.SettingsSectionSelect}
   */
  function MediaSizeSettings(ticketItem) {
    print_preview.SettingsSectionSelect.call(this, ticketItem);
  }

  MediaSizeSettings.prototype = {
    __proto__: print_preview.SettingsSectionSelect.prototype,

    /** @override */
    getDefaultDisplayName_: function(option) {
      return option.name;
    }
  };

  // Export
  return {MediaSizeSettings: MediaSizeSettings};
});
