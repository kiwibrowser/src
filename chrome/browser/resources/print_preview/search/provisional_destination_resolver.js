// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('print_preview');

/**
 * States that the provisional destination resolver can be in.
 * @enum {string}
 */
print_preview.ResolverState = {
  INITIAL: 'INITIAL',
  ACTIVE: 'ACTIVE',
  GRANTING_PERMISSION: 'GRANTING_PERMISSION',
  ERROR: 'ERROR',
  DONE: 'DONE'
};

cr.define('print_preview', function() {
  'use strict';

  /**
   * Overlay used to resolve a provisional extension destination. The user is
   * prompted to allow print preview to grant a USB device access to an
   * extension associated with the destination. If user agrees destination
   * resolvement is attempted (which includes granting the extension USB access
   * and requesting destination description from the extension). The overlay is
   * hidden when destination resolving is done.
   *
   * @param {!print_preview.DestinationStore} destinationStore The destination
   *    store containing the destination. Used as a proxy to native layer for
   *    resolving the destination.
   * @param {!print_preview.Destination} destination The destination that has
   *     to be resolved.
   * @constructor
   * @extends {print_preview.Overlay}
   */
  function ProvisionalDestinationResolver(destinationStore, destination) {
    print_preview.Overlay.call(this);

    /** @private {!print_preview.DestinationStore} */
    this.destinationStore_ = destinationStore;
    /** @private {!print_preview.Destination} */
    this.destination_ = destination;

    /** @private {print_preview.ResolverState} */
    this.state_ = print_preview.ResolverState.INITIAL;

    /**
     * Promise resolver for promise returned by {@code this.run}.
     * @private {?PromiseResolver<!print_preview.Destination>}
     */
    this.promiseResolver_ = null;
  }

  /**
   * @param {!print_preview.DestinationStore} store
   * @param {!print_preview.Destination} destination
   * @return {?ProvisionalDestinationResolver}
   */
  ProvisionalDestinationResolver.create = function(store, destination) {
    if (destination.provisionalType !=
        print_preview.DestinationProvisionalType.NEEDS_USB_PERMISSION) {
      return null;
    }
    return new ProvisionalDestinationResolver(store, destination);
  };

  ProvisionalDestinationResolver.prototype = {
    __proto__: print_preview.Overlay.prototype,

    /** @override */
    enterDocument: function() {
      print_preview.Overlay.prototype.enterDocument.call(this);

      this.tracker.add(
          this.getChildElement('.usb-permission-ok-button'), 'click',
          this.startResolveDestination_.bind(this));
      this.tracker.add(
          this.getChildElement('.cancel'), 'click', this.cancel.bind(this));

      this.tracker.add(
          this.destinationStore_,
          print_preview.DestinationStore.EventType
              .PROVISIONAL_DESTINATION_RESOLVED,
          this.onDestinationResolved_.bind(this));
    },

    /** @override */
    onSetVisibleInternal: function(visible) {
      if (visible) {
        assert(
            this.state_ == print_preview.ResolverState.INITIAL,
            'Showing overlay while not in initial state.');
        assert(!this.promiseResolver_, 'Promise resolver already set.');
        this.setState_(print_preview.ResolverState.ACTIVE);
        this.promiseResolver_ = new PromiseResolver();
        this.getChildElement('.default').focus();
      } else if (this.state_ != print_preview.ResolverState.DONE) {
        assert(
            this.state_ != print_preview.ResolverState.INITIAL,
            'Hiding in initial state');
        this.setState_(print_preview.ResolverState.DONE);
        this.promiseResolver_.reject();
        this.promiseResolver_ = null;
      }
    },

    /** @override */
    createDom: function() {
      this.setElementInternal(
          this.cloneTemplateInternal('extension-usb-resolver'));

      const extNameEl = this.getChildElement('.usb-permission-extension-name');
      extNameEl.title = this.destination_.extensionName;
      extNameEl.textContent = this.destination_.extensionName;

      const extIconEl = this.getChildElement('.usb-permission-extension-icon');
      extIconEl.style.backgroundImage = '-webkit-image-set(' +
          'url(chrome://extension-icon/' + this.destination_.extensionId +
          '/24/1) 1x,' +
          'url(chrome://extension-icon/' + this.destination_.extensionId +
          '/48/1) 2x)';
    },

    /**
     * Handler for click on OK button. It initiates destination resolving.
     * @private
     */
    startResolveDestination_: function() {
      assert(
          this.state_ == print_preview.ResolverState.ACTIVE,
          'Invalid state in request grant permission');

      this.setState_(print_preview.ResolverState.GRANTING_PERMISSION);
      this.destinationStore_.resolveProvisionalDestination(this.destination_);
    },

    /**
     * Handler for PROVISIONAL_DESTINATION_RESOLVED event. It finalizes the
     * resolver state once the destination associated with the resolver gets
     * resolved.
     * @param {Event} event
     * @private
     */
    onDestinationResolved_: function(event) {
      if (this.state_ == print_preview.ResolverState.DONE)
        return;

      if (event.provisionalId != this.destination_.id)
        return;

      if (event.destination) {
        this.setState_(print_preview.ResolverState.DONE);
        this.promiseResolver_.resolve(event.destination);
        this.promiseResolver_ = null;
        this.setIsVisible(false);
      } else {
        this.setState_(print_preview.ResolverState.ERROR);
      }
    },

    /**
     * Sets new resolver state and updates the UI accordingly.
     * @param {print_preview.ResolverState} state
     * @private
     */
    setState_: function(state) {
      if (this.state_ == state)
        return;

      this.state_ = state;
      this.updateUI_();
    },

    /**
     * Updates the resolver overlay UI to match the resolver state.
     * @private
     */
    updateUI_: function() {
      this.getChildElement('.usb-permission-ok-button').hidden =
          this.state_ == print_preview.ResolverState.ERROR;
      this.getChildElement('.usb-permission-ok-button').disabled =
          this.state_ != print_preview.ResolverState.ACTIVE;

      // If OK button is disabled, make sure Cancel button gets focus.
      if (this.state_ != print_preview.ResolverState.ACTIVE)
        this.getChildElement('.cancel').focus();

      this.getChildElement('.throbber-placeholder')
          .classList.toggle(
              'throbber',
              this.state_ == print_preview.ResolverState.GRANTING_PERMISSION);

      this.getChildElement('.usb-permission-extension-desc').hidden =
          this.state_ == print_preview.ResolverState.ERROR;

      this.getChildElement('.usb-permission-message').textContent =
          this.state_ == print_preview.ResolverState.ERROR ?
          loadTimeData.getStringF(
              'resolveExtensionUSBErrorMessage',
              this.destination_.extensionName) :
          loadTimeData.getString('resolveExtensionUSBPermissionMessage');
    },

    /**
     * Initiates and shows the resolver overlay.
     * @param {!HTMLElement} parent The element that should parent the resolver
     *     UI.
     * @return {!Promise<!print_preview.Destination>} Promise that will be
     * fulfilled when the destination resolving is finished.
     */
    run: function(parent) {
      this.render(parent);
      this.setIsVisible(true);

      assert(this.promiseResolver_, 'Promise resolver not created.');
      return this.promiseResolver_.promise;
    }
  };

  return {ProvisionalDestinationResolver: ProvisionalDestinationResolver};
});
