// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview.ticket_items', function() {
  'use strict';

  class Color extends print_preview.ticket_items.TicketItem {
    /**
     * Color ticket item whose value is a {@code boolean} that indicates whether
     * the document should be printed in color.
     * @param {!print_preview.AppState} appState App state persistence object to
     *     save the state of the color selection.
     * @param {!print_preview.DestinationStore} destinationStore Used to
     *     determine whether color printing should be available.
     */
    constructor(appState, destinationStore) {
      super(
          appState, print_preview.AppStateField.IS_COLOR_ENABLED,
          destinationStore);
    }

    /** @override */
    wouldValueBeValid(value) {
      return true;
    }

    /** @override */
    isCapabilityAvailable() {
      const dest = this.getSelectedDestInternal();
      return dest ? dest.hasColorCapability : false;
    }

    /** @override */
    getDefaultValueInternal() {
      const dest = this.getSelectedDestInternal();
      const defaultOption = dest ? dest.defaultColorOption : null;
      return defaultOption &&
          (Color.COLOR_TYPES_.indexOf(defaultOption.type) >= 0);
    }

    /** @override */
    getCapabilityNotAvailableValueInternal() {
      // TODO(rltoscano): Get rid of this check based on destination ID. These
      // destinations should really update their CDDs to have only one color
      // option that has type 'STANDARD_COLOR'.
      const dest = this.getSelectedDestInternal();
      if (dest) {
        if (dest.id == print_preview.Destination.GooglePromotedId.DOCS ||
            dest.type == print_preview.DestinationType.MOBILE) {
          return true;
        }
      }
      return this.getDefaultValueInternal();
    }
  }

  /**
   * @private {!Array<string>} List of capability types considered color.
   * @const
   */
  Color.COLOR_TYPES_ = ['STANDARD_COLOR', 'CUSTOM_COLOR'];

  /**
   * @private {!Array<string>} List of capability types considered monochrome.
   * @const
   */
  Color.MONOCHROME_TYPES_ = ['STANDARD_MONOCHROME', 'CUSTOM_MONOCHROME'];


  // Export
  return {Color: Color};
});
