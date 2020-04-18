// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview');

/**
 * Enumeration of field names for serialized app state.
 * @enum {string}
 */
print_preview.AppStateField = {
  VERSION: 'version',
  RECENT_DESTINATIONS: 'recentDestinations',
  DPI: 'dpi',
  MEDIA_SIZE: 'mediaSize',
  MARGINS_TYPE: 'marginsType',
  CUSTOM_MARGINS: 'customMargins',
  IS_COLOR_ENABLED: 'isColorEnabled',
  IS_DUPLEX_ENABLED: 'isDuplexEnabled',
  IS_HEADER_FOOTER_ENABLED: 'isHeaderFooterEnabled',
  IS_LANDSCAPE_ENABLED: 'isLandscapeEnabled',
  IS_COLLATE_ENABLED: 'isCollateEnabled',
  IS_FIT_TO_PAGE_ENABLED: 'isFitToPageEnabled',
  IS_CSS_BACKGROUND_ENABLED: 'isCssBackgroundEnabled',
  SCALING: 'scaling',
  VENDOR_OPTIONS: 'vendorOptions'
};

cr.define('print_preview', function() {
  'use strict';
  class AppState extends cr.EventTarget {
    /**
     * Object used to get and persist the print preview application state.
     * @param {!print_preview.DestinationStore} destinationStore The destination
     *     store, used to track destination selection changes.
     */
    constructor(destinationStore) {
      super();

      /**
       * Internal representation of application state.
       * Must contain only plain objects or classes that override the
       * toJSON() method.
       * @private {!Object}
       */
      this.state_ = {};
      this.state_[print_preview.AppStateField.VERSION] = AppState.VERSION_;
      this.state_[print_preview.AppStateField.RECENT_DESTINATIONS] = [];

      /**
       * Whether the app state has been initialized. The app state will ignore
       * all writes until it has been initialized.
       * @private {boolean}
       */
      this.isInitialized_ = false;

      /**
       * Native Layer object to use for sending app state to C++ handler.
       * @private {!print_preview.NativeLayer}
       */
      this.nativeLayer_ = print_preview.NativeLayer.getInstance();

      /**
       * Destination store object for tracking recent destinations.
       * @private {!print_preview.DestinationStore}
       */
      this.destinationStore_ = destinationStore;

      /**
       * Event tracker used to track event listeners.
       * @private {!EventTracker}
       */
      this.tracker_ = new EventTracker();
    }

    /**
     * @return {?print_preview.RecentDestination} The most recent
     *     destination, which is currently the selected destination.
     */
    get selectedDestination() {
      return (this.state_[print_preview.AppStateField.RECENT_DESTINATIONS]
                  .length > 0) ?
          this.state_[print_preview.AppStateField.RECENT_DESTINATIONS][0] :
          null;
    }

    /**
     * @return {?Array<!print_preview.RecentDestination>} The
     *     AppState.NUM_DESTINATIONS_ most recent destinations.
     */
    get recentDestinations() {
      return this.state_[print_preview.AppStateField.RECENT_DESTINATIONS];
    }

    /**
     * @param {!print_preview.AppStateField} field App state field to check if
     *     set.
     * @return {boolean} Whether a field has been set in the app state.
     */
    hasField(field) {
      return this.state_.hasOwnProperty(field);
    }

    /**
     * @param {!print_preview.AppStateField} field App state field to get.
     * @return {?} Value of the app state field.
     */
    getField(field) {
      if (field == print_preview.AppStateField.CUSTOM_MARGINS) {
        return this.state_[field] ?
            print_preview.Margins.parse(this.state_[field]) :
            null;
      }
      return this.state_[field];
    }

    /**
     * Initializes the app state from a serialized string returned by the native
     * layer.
     * @param {?string} serializedAppStateStr Serialized string representation
     *     of the app state.
     */
    init(serializedAppStateStr) {
      if (serializedAppStateStr) {
        try {
          const state = JSON.parse(serializedAppStateStr);
          if (!!state &&
              state[print_preview.AppStateField.VERSION] == AppState.VERSION_) {
            this.state_ = /** @type {!Object} */ (state);
          }
        } catch (e) {
          console.error('Unable to parse state: ' + e);
          // Proceed with default state.
        }
      } else {
        // Set some state defaults.
        this.state_[print_preview.AppStateField.RECENT_DESTINATIONS] = [];
      }
      if (!this.state_[print_preview.AppStateField.RECENT_DESTINATIONS]) {
        this.state_[print_preview.AppStateField.RECENT_DESTINATIONS] = [];
      } else if (!(this.state_[print_preview.AppStateField
                                   .RECENT_DESTINATIONS] instanceof
                   Array)) {
        const tmp =
            this.state_[print_preview.AppStateField.RECENT_DESTINATIONS];
        this.state_[print_preview.AppStateField.RECENT_DESTINATIONS] = [tmp];
      } else if (
          !this.state_[print_preview.AppStateField.RECENT_DESTINATIONS][0] ||
          !this.state_[print_preview.AppStateField.RECENT_DESTINATIONS][0].id) {
        // read in incorrectly
        this.state_[print_preview.AppStateField.RECENT_DESTINATIONS] = [];
      } else if (
          this.state_[print_preview.AppStateField.RECENT_DESTINATIONS].length >
          AppState.NUM_DESTINATIONS_) {
        this.state_[print_preview.AppStateField.RECENT_DESTINATIONS].length =
            AppState.NUM_DESTINATIONS_;
      }
    }

    /**
     * Sets to initialized state. Now object will accept persist requests and
     * monitor for destination changes.
     */
    setInitialized() {
      this.isInitialized_ = true;
      this.tracker_.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType
              .SELECTED_DESTINATION_CAPABILITIES_READY,
          this.persistSelectedDestination_.bind(this));
      this.tracker_.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType.DESTINATION_SELECT,
          this.persistSelectedDestination_.bind(this));
    }

    /**
     * Persists the given value for the given field.
     * @param {!print_preview.AppStateField} field Field to persist.
     * @param {?} value Value of field to persist.
     */
    persistField(field, value) {
      if (!this.isInitialized_)
        return;
      if (field == print_preview.AppStateField.CUSTOM_MARGINS) {
        this.state_[field] = value ? value.serialize() : null;
      } else {
        this.state_[field] = value;
      }
      this.persist_();
    }

    /**
     * Persists the selected destination from the destination store.
     * @private
     */
    persistSelectedDestination_() {
      assert(this.isInitialized_);

      const destination = this.destinationStore_.selectedDestination;
      if (!destination)
        return;

      // Determine if this destination is already in the recent destinations,
      // and where in the array it is located.
      const newDestination =
          print_preview.makeRecentDestination(assert(destination));
      let indexFound =
          this.state_[print_preview.AppStateField.RECENT_DESTINATIONS]
              .findIndex(function(recent) {
                return (
                    newDestination.id == recent.id &&
                    newDestination.origin == recent.origin);
              });

      // No change
      if (indexFound == 0 &&
          this.selectedDestination.capabilities ==
              newDestination.capabilities) {
        this.persist_();
        return;
      }

      // Shift the array so that the nth most recent destination is located at
      // index n.
      if (indexFound == -1 &&
          this.state_[print_preview.AppStateField.RECENT_DESTINATIONS].length ==
              AppState.NUM_DESTINATIONS_) {
        indexFound = AppState.NUM_DESTINATIONS_ - 1;
      }
      if (indexFound != -1)
        this.state_[print_preview.AppStateField.RECENT_DESTINATIONS].splice(
            indexFound, 1);

      // Add the most recent destination
      this.state_[print_preview.AppStateField.RECENT_DESTINATIONS].splice(
          0, 0, newDestination);

      this.persist_();
    }

    /**
     * Calls into the native layer to persist the application state.
     * @private
     */
    persist_() {
      this.nativeLayer_.saveAppState(JSON.stringify(this.state_));
    }
  }

  /**
   * Number of recent print destinations to store across browser sessions.
   * @const {number}
   * @private
   */
  AppState.NUM_DESTINATIONS_ = 3;

  /**
   * Current version of the app state. This value helps to understand how to
   * parse earlier versions of the app state.
   * @const {number}
   * @private
   */
  AppState.VERSION_ = 2;

  return {
    AppState: AppState,
  };
});
