// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Component that renders a destination item in a destination list.
   * @param {!cr.EventTarget} eventTarget Event target to dispatch selection
   *     events to.
   * @param {!print_preview.Destination} destination Destination data object to
   *     render.
   * @param {RegExp} query Active filter query.
   * @constructor
   * @extends {print_preview.Component}
   */
  function DestinationListItem(eventTarget, destination, query) {
    print_preview.Component.call(this);

    /**
     * Event target to dispatch selection events to.
     * @type {!cr.EventTarget}
     * @private
     */
    this.eventTarget_ = eventTarget;

    /**
     * Destination that the list item renders.
     * @type {!print_preview.Destination}
     * @private
     */
    this.destination_ = destination;

    /**
     * Active filter query text.
     * @type {RegExp}
     * @private
     */
    this.query_ = query;
  }

  /**
   * Event types dispatched by the destination list item.
   * @enum {string}
   */
  DestinationListItem.EventType = {
    // Dispatched to check the printer needs to be configured before activation.
    CONFIGURE_REQUEST: 'print_preview.DestinationListItem.CONFIGURE_REQUEST',
    // Dispatched when the list item is activated.
    SELECT: 'print_preview.DestinationListItem.SELECT',
    REGISTER_PROMO_CLICKED:
        'print_preview.DestinationListItem.REGISTER_PROMO_CLICKED'
  };

  DestinationListItem.prototype = {
    __proto__: print_preview.Component.prototype,

    /** @override */
    createDom: function() {
      this.setElementInternal(
          this.cloneTemplateInternal('destination-list-item-template'));
      this.updateUi_();
    },

    /** @override */
    enterDocument: function() {
      print_preview.Component.prototype.enterDocument.call(this);
      this.tracker.add(this.getElement(), 'click', this.onActivate_.bind(this));
      this.tracker.add(
          this.getElement(), 'keydown', this.onKeyDown_.bind(this));
      this.tracker.add(
          this.getChildElement('.register-promo-button'), 'click',
          this.onRegisterPromoClicked_.bind(this));
      this.tracker.add(
          this.getChildElement('.learn-more-link'), 'click',
          this.onGcpErrorLearnMoreClick_.bind(this));
    },

    /** @return {!print_preview.Destination} */
    get destination() {
      return this.destination_;
    },

    /**
     * Updates the list item UI state.
     * @param {!print_preview.Destination} destination Destination data object
     *     to render.
     * @param {RegExp} query Active filter query.
     */
    update: function(destination, query) {
      this.destination_ = destination;
      this.query_ = query;
      this.updateUi_();
    },

    /**
     * Called if the printer configuration request is accepted. Show the waiting
     * message to the user as the configuration might take longer than expected.
     */
    onConfigureRequestAccepted: function() {
      // It must be a Chrome OS CUPS printer which hasn't been set up before.
      assert(
          this.destination_.origin == print_preview.DestinationOrigin.CROS &&
          !this.destination_.capabilities);
      this.updateConfiguringMessage_(true);
    },

    /**
     * Called if the printer configuration request is rejected. The request is
     * rejected if another printer is setting up in process or the current
     * printer doesn't need to be setup.
     * @param {boolean} otherPrinterSetupInProgress
     */
    onConfigureRequestRejected: function(otherPrinterSetupInProgress) {
      // If another printer setup is in progress, the printer should not be
      // activated.
      if (!otherPrinterSetupInProgress)
        this.onDestinationActivated_();
    },

    /**
     * Called when the printer configuration request is resolved successful or
     * failed.
     * @param response {!print_preview.PrinterSetupResponse}
     */
    onConfigureResolved: function(response) {
      assert(response.printerId == this.destination_.id);
      if (response.success) {
        this.updateConfiguringMessage_(false);
        this.destination_.capabilities = response.capabilities;
        this.onDestinationActivated_();
      } else {
        this.updateConfiguringMessage_(false);
        setIsVisible(this.getChildElement('.configuring-failed-text'), true);
      }
    },

    /**
     * Initializes the element with destination's info.
     * @private
     */
    updateUi_: function() {
      const iconImg = this.getChildElement('.destination-list-item-icon');
      iconImg.src = this.destination_.iconUrl;
      iconImg.srcset = this.destination_.srcSet;

      const nameEl = this.getChildElement('.destination-list-item-name');
      let textContent = this.destination_.displayName;
      if (this.query_) {
        nameEl.textContent = '';
        // When search query is specified, make it obvious why this particular
        // printer made it to the list. Display name is always visible, even if
        // it does not match the search query.
        this.addTextWithHighlight_(nameEl, textContent);
        // Show the first matching property.
        this.destination_.extraPropertiesToMatch.some(function(property) {
          if (property.match(this.query_)) {
            const hintSpan = document.createElement('span');
            hintSpan.className = 'search-hint';
            nameEl.appendChild(hintSpan);
            this.addTextWithHighlight_(hintSpan, property);
            // Add the same property to the element title.
            textContent += ' (' + property + ')';
            return true;
          }
        }, this);
      } else {
        // Show just the display name and nothing else to lessen visual clutter.
        nameEl.textContent = textContent;
      }
      nameEl.title = textContent;

      if (this.destination_.isExtension) {
        const extensionNameEl = this.getChildElement('.extension-name');
        const extensionName = this.destination_.extensionName;
        extensionNameEl.title = this.destination_.extensionName;
        if (this.query_) {
          extensionNameEl.textContent = '';
          this.addTextWithHighlight_(extensionNameEl, extensionName);
        } else {
          extensionNameEl.textContent = this.destination_.extensionName;
        }

        const extensionIconEl = this.getChildElement('.extension-icon');
        extensionIconEl.style.backgroundImage = '-webkit-image-set(' +
            'url(chrome://extension-icon/' + this.destination_.extensionId +
            '/24/1) 1x,' +
            'url(chrome://extension-icon/' + this.destination_.extensionId +
            '/48/1) 2x)';
        extensionIconEl.title = loadTimeData.getStringF(
            'extensionDestinationIconTooltip', this.destination_.extensionName);
        extensionIconEl.onclick = this.onExtensionIconClicked_.bind(this);
        extensionIconEl.onkeydown = /** @type {function(Event)} */ (
            this.onExtensionIconKeyDown_.bind(this));
      }

      const extensionIndicatorEl =
          this.getChildElement('.extension-controlled-indicator');
      setIsVisible(extensionIndicatorEl, this.destination_.isExtension);

      // Initialize the element which renders the destination's connection
      // status.
      this.getElement().classList.toggle(
          'stale', this.destination_.isOfflineOrInvalid);
      const connectionStatusEl = this.getChildElement('.connection-status');
      connectionStatusEl.textContent = this.destination_.connectionStatusText;
      setIsVisible(connectionStatusEl, this.destination_.isOfflineOrInvalid);
      setIsVisible(
          this.getChildElement('.learn-more-link'),
          this.destination_.shouldShowInvalidCertificateError);

      // Initialize registration promo element for Privet unregistered printers.
      setIsVisible(
          this.getChildElement('.register-promo'),
          this.destination_.connectionStatus ==
              print_preview.DestinationConnectionStatus.UNREGISTERED);

      if (cr.isChromeOS) {
        // Reset the configuring messages for CUPS printers.
        this.updateConfiguringMessage_(false);
        setIsVisible(this.getChildElement('.configuring-failed-text'), false);
      }
    },

    /**
     * Adds text to parent element wrapping search query matches in highlighted
     * spans.
     * @param {!Element} parent Element to build the text in.
     * @param {string} text The text string to highlight segments in.
     * @private
     */
    addTextWithHighlight_: function(parent, text) {
      const sections = text.split(this.query_);
      for (let i = 0; i < sections.length; ++i) {
        if (i % 2 == 0) {
          parent.appendChild(document.createTextNode(sections[i]));
        } else {
          const span = document.createElement('span');
          span.className = 'destination-list-item-query-highlight';
          span.textContent = sections[i];
          parent.appendChild(span);
        }
      }
    },

    /**
     * Shows/Hides the configuring in progress message and starts/stops its
     * animation accordingly.
     * @param {boolean} show If the message and animation should be shown.
     * @private
     */
    updateConfiguringMessage_: function(show) {
      setIsVisible(this.getChildElement('.configuring-in-progress-text'), show);
      this.getChildElement('.configuring-text-jumping-dots')
          .classList.toggle('jumping-dots', show);
    },

    /**
     * Called when the destination item is activated. Check if the printer needs
     * to be set up first before activation.
     * @private
     */
    onActivate_: function() {
      if (!cr.isChromeOS) {
        this.onDestinationActivated_();
        return;
      }

      // Check if the printer needs configuration before using. The user is only
      // allowed to set up one printer at one time.
      const configureEvent = new CustomEvent(
          DestinationListItem.EventType.CONFIGURE_REQUEST,
          {detail: {destination: this.destination_}});
      this.eventTarget_.dispatchEvent(configureEvent);
    },

    /**
     * Called when the destination has been resolved successfully and needs to
     * be activated. Dispatches a SELECT event on the given event target.
     * @private
     */
    onDestinationActivated_: function() {
      if (this.destination_.connectionStatus !=
          print_preview.DestinationConnectionStatus.UNREGISTERED) {
        const selectEvt = new Event(DestinationListItem.EventType.SELECT);
        selectEvt.destination = this.destination_;
        this.eventTarget_.dispatchEvent(selectEvt);
      }
    },

    /**
     * Called when the key is pressed on the destination item. Dispatches a
     * SELECT event when Enter is pressed.
     * @param {!KeyboardEvent} e Keyboard event to process.
     * @private
     */
    onKeyDown_: function(e) {
      if (!hasKeyModifiers(e)) {
        if (e.keyCode == 13) {
          const activeElementTag = document.activeElement ?
              document.activeElement.tagName.toUpperCase() :
              '';
          if (activeElementTag == 'LI') {
            e.stopPropagation();
            e.preventDefault();
            this.onActivate_();
          }
        }
      }
    },

    /**
     * Called when the registration promo is clicked.
     * @private
     */
    onRegisterPromoClicked_: function() {
      const promoClickedEvent =
          new Event(DestinationListItem.EventType.REGISTER_PROMO_CLICKED);
      promoClickedEvent.destination = this.destination_;
      this.eventTarget_.dispatchEvent(promoClickedEvent);
    },

    /**
     * Called when the learn more link for an unsupported cloud destination is
     * clicked. Opens the help page via native layer.
     * @private
     */
    onGcpErrorLearnMoreClick_: function() {
      print_preview.NativeLayer.getInstance().forceOpenNewTab(
          loadTimeData.getString('gcpCertificateErrorLearnMoreURL'));
    },

    /**
     * Handles click and 'Enter' key down events for the extension icon element.
     * It opens extensions page with the extension associated with the
     * destination highlighted.
     * @param {Event} e The event to handle.
     * @private
     */
    onExtensionIconClicked_: function(e) {
      e.stopPropagation();
      window.open('chrome://extensions?id=' + this.destination_.extensionId);
    },

    /**
     * Handles key down event for the extensin icon element. Keys different than
     * 'Enter' are ignored.
     * @param {!Event} e The event to handle.
     * @private
     */
    onExtensionIconKeyDown_: function(e) {
      if (hasKeyModifiers(e))
        return;
      if (e.keyCode != 13 /* Enter */)
        return;
      this.onExtensionIconClicked_(e);
    }
  };

  // Export
  return {DestinationListItem: DestinationListItem};
});
