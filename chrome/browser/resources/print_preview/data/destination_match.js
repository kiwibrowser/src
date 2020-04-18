// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';
  /**
   * Converts DestinationOrigin to PrinterType.
   * @param {!print_preview.DestinationOrigin} origin The printer's
   *     destination origin.
   * return {?print_preview.PrinterType} The corresponding PrinterType.
   *     Returns null if no match is found.
   */
  const originToType = function(origin) {
    if (origin === print_preview.DestinationOrigin.LOCAL ||
        origin === print_preview.DestinationOrigin.CROS) {
      return print_preview.PrinterType.LOCAL_PRINTER;
    }
    if (origin === print_preview.DestinationOrigin.PRIVET)
      return print_preview.PrinterType.PRIVET_PRINTER;
    if (origin === print_preview.DestinationOrigin.EXTENSION)
      return print_preview.PrinterType.EXTENSION_PRINTER;
    return null;
  };

  class DestinationMatch {
    /**
     * A set of key parameters describing a destination used to determine
     * if two destinations are the same.
     * @param {!Array<!print_preview.DestinationOrigin>} origins Match
     *     destinations from these origins.
     * @param {RegExp} idRegExp Match destination's id.
     * @param {RegExp} displayNameRegExp Match destination's displayName.
     * @param {boolean} skipVirtualDestinations Whether to ignore virtual
     *     destinations, for example, Save as PDF.
     */
    constructor(origins, idRegExp, displayNameRegExp, skipVirtualDestinations) {
      /** @private {!Array<!print_preview.DestinationOrigin>} */
      this.origins_ = origins;

      /** @private {RegExp} */
      this.idRegExp_ = idRegExp;

      /** @private {RegExp} */
      this.displayNameRegExp_ = displayNameRegExp;

      /** @private {boolean} */
      this.skipVirtualDestinations_ = skipVirtualDestinations;
    }

    /**
     * @param {string} origin Origin to match.
     * @return {boolean} Whether the origin is one of the {@code origins_}.
     */
    matchOrigin(origin) {
      return arrayContains(this.origins_, origin);
    }

    /**
     * @param {string} id Id of the destination.
     * @param {string} origin Origin of the destination.
     * @return {boolean} Whether destination is the same as initial.
     */
    matchIdAndOrigin(id, origin) {
      return this.matchOrigin(origin) && !!this.idRegExp_ &&
          this.idRegExp_.test(id);
    }

    /**
     * @param {!print_preview.Destination} destination Destination to match.
     * @return {boolean} Whether {@code destination} matches the last user
     *     selected one.
     */
    match(destination) {
      if (!this.matchOrigin(destination.origin)) {
        return false;
      }
      if (this.idRegExp_ && !this.idRegExp_.test(destination.id)) {
        return false;
      }
      if (this.displayNameRegExp_ &&
          !this.displayNameRegExp_.test(destination.displayName)) {
        return false;
      }
      if (this.skipVirtualDestinations_ &&
          this.isVirtualDestination_(destination)) {
        return false;
      }
      return true;
    }

    /**
     * @param {!print_preview.Destination} destination Destination to check.
     * @return {boolean} Whether {@code destination} is virtual, in terms of
     *     destination selection.
     * @private
     */
    isVirtualDestination_(destination) {
      if (destination.origin == print_preview.DestinationOrigin.LOCAL) {
        return arrayContains(
            [print_preview.Destination.GooglePromotedId.SAVE_AS_PDF],
            destination.id);
      }
      return arrayContains(
          [print_preview.Destination.GooglePromotedId.DOCS], destination.id);
    }

    /**
     * @return {!Set<?print_preview.PrinterType>} The printer types that
     *     correspond to this destination match. A null element in the set
     *     indicates the match may represent a Cloud destination.
     */
    getTypes() {
      return new Set(this.origins_.map(origin => originToType(origin)));
    }
  }

  // Export
  return {originToType: originToType, DestinationMatch: DestinationMatch};
});
