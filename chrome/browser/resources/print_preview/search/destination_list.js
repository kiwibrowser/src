// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Component that displays a list of destinations with a heading, action link,
   * and "Show All..." button. An event is dispatched when the action link is
   * activated.
   * @param {!cr.EventTarget} eventTarget Event target to pass to destination
   *     items for dispatching SELECT events.
   * @param {string} title Title of the destination list.
   * @param {?string} actionLinkLabel Optional label of the action link. If
   *     {@code null} is provided, the action link will not be shown.
   * @param {boolean=} opt_showAll Whether to initially show all destinations or
   *     only the first few ones.
   * @constructor
   * @extends {print_preview.Component}
   */
  function DestinationList(eventTarget, title, actionLinkLabel, opt_showAll) {
    print_preview.Component.call(this);

    /**
     * Event target to pass to destination items for dispatching SELECT events.
     * @type {!cr.EventTarget}
     * @private
     */
    this.eventTarget_ = eventTarget;

    /**
     * Title of the destination list.
     * @type {string}
     * @private
     */
    this.title_ = title;

    /**
     * Label of the action link.
     * @type {?string}
     * @private
     */
    this.actionLinkLabel_ = actionLinkLabel;

    /**
     * Backing store for the destination list.
     * @type {!Array<print_preview.Destination>}
     * @private
     */
    this.destinations_ = [];

    /**
     * Set of destination ids.
     * @type {!Object<boolean>}
     * @private
     */
    this.destinationIds_ = {};

    /**
     * Current query used for filtering.
     * @type {RegExp}
     * @private
     */
    this.query_ = null;

    /**
     * Whether the destination list is fully expanded.
     * @type {boolean}
     * @private
     */
    this.isShowAll_ = opt_showAll || false;

    /**
     * Maximum number of destinations before showing the "Show All..." button.
     * @type {number}
     * @private
     */
    this.shortListSize_ = DestinationList.DEFAULT_SHORT_LIST_SIZE_;

    /**
     * List items representing destinations.
     * @type {!Array<!print_preview.DestinationListItem>}
     * @private
     */
    this.listItems_ = [];
  }

  /**
   * Enumeration of event types dispatched by the destination list.
   * @enum {string}
   */
  DestinationList.EventType = {
    // Dispatched when the action linked is activated.
    ACTION_LINK_ACTIVATED: 'print_preview.DestinationList.ACTION_LINK_ACTIVATED'
  };

  /**
   * Default maximum number of destinations before showing the "Show All..."
   * button.
   * @type {number}
   * @const
   * @private
   */
  DestinationList.DEFAULT_SHORT_LIST_SIZE_ = 4;

  /**
   * Height of a destination list item in pixels.
   * @type {number}
   * @const
   * @private
   */
  DestinationList.HEIGHT_OF_ITEM_ = 30;

  DestinationList.prototype = {
    __proto__: print_preview.Component.prototype,

    /** @param {boolean} isShowAll Whether the show-all button is activated. */
    setIsShowAll: function(isShowAll) {
      this.isShowAll_ = isShowAll;
      this.renderDestinations_();
    },

    /**
     * @return {number} Size of list when destination list is in collapsed
     *     mode (a.k.a non-show-all mode).
     */
    getShortListSize: function() {
      return this.shortListSize_;
    },

    /** @return {number} Count of the destinations in the list. */
    getDestinationsCount: function() {
      return this.destinations_.length;
    },

    /**
     * Gets estimated height of the destination list for the given number of
     * items.
     * @param {number} numItems Number of items to render in the destination
     *     list.
     * @return {number} Height (in pixels) of the destination list.
     */
    getEstimatedHeightInPixels: function(numItems) {
      numItems = Math.min(numItems, this.destinations_.length);
      const headerHeight =
          this.getChildElement('.destination-list > header').offsetHeight;
      return headerHeight +
          (numItems > 0 ? numItems * DestinationList.HEIGHT_OF_ITEM_ :
                          // To account for "No destinations found" message.
               DestinationList.HEIGHT_OF_ITEM_);
    },

    /**
     * @return {Element} The element that contains this one. Used for height
     *     calculations.
     */
    getContainerElement: function() {
      return this.getElement().parentNode;
    },

    /** @param {boolean} isVisible Whether the throbber is visible. */
    setIsThrobberVisible: function(isVisible) {
      setIsVisible(this.getChildElement('.throbber-container'), isVisible);
    },

    /**
     * @param {number} size Size of list when destination list is in collapsed
     *     mode (a.k.a non-show-all mode).
     */
    updateShortListSize: function(size) {
      size = Math.max(1, Math.min(size, this.destinations_.length));
      if (size == 1 && this.destinations_.length > 1) {
        // If this is the case, we will only show the "Show All" button and
        // nothing else. Increment the short list size by one so that we can see
        // at least one print destination.
        size++;
      }
      this.setShortListSizeInternal(size);
    },

    /** @override */
    createDom: function() {
      this.setElementInternal(
          this.cloneTemplateInternal('destination-list-template'));
      this.getChildElement('.title').textContent = this.title_;
      if (this.actionLinkLabel_) {
        const actionLinkEl = this.getChildElement('.action-link');
        actionLinkEl.textContent = this.actionLinkLabel_;
        setIsVisible(actionLinkEl, true);
      }
    },

    /** @override */
    enterDocument: function() {
      print_preview.Component.prototype.enterDocument.call(this);
      this.tracker.add(
          this.getChildElement('.action-link'), 'click',
          this.onActionLinkClick_.bind(this));
      this.tracker.add(
          this.getChildElement('.show-all-button'), 'click',
          this.setIsShowAll.bind(this, true));
    },

    /**
     * Updates the destinations to render in the destination list.
     * @param {!Array<print_preview.Destination>} destinations Destinations to
     *     render.
     */
    updateDestinations: function(destinations) {
      this.destinations_ = destinations;
      this.destinationIds_ = destinations.reduce(function(ids, destination) {
        ids[destination.id] = true;
        return ids;
      }, {});
      this.renderDestinations_();
    },

    /** @param {RegExp} query Query to update the filter with. */
    updateSearchQuery: function(query) {
      this.query_ = query;
      this.renderDestinations_();
    },

    /**
     * @param {string} destinationId The ID of the destination.
     * @return {?print_preview.DestinationListItem} The found destination list
     *     item or null if not found.
     */
    getDestinationItem: function(destinationId) {
      return this.listItems_.find(function(listItem) {
        return listItem.destination.id == destinationId;
      }) ||
          null;
    },

    /**
     * @param {string} text Text to set the action link to.
     * @protected
     */
    setActionLinkTextInternal: function(text) {
      this.actionLinkLabel_ = text;
      this.getChildElement('.action-link').textContent = text;
    },

    /**
     * Sets the short list size without constraints.
     * @protected
     */
    setShortListSizeInternal: function(size) {
      this.shortListSize_ = size;
      this.renderDestinations_();
    },

    /**
     * Renders all destinations in the list that match the current query.
     * @private
     */
    renderDestinations_: function() {
      if (!this.query_) {
        this.renderDestinationsList_(this.destinations_);
      } else {
        const filteredDests = this.destinations_.filter(function(destination) {
          return destination.matches(assert(this.query_));
        }, this);
        this.renderDestinationsList_(filteredDests);
      }
    },

    /**
     * Renders all destinations in the given list.
     * @param {!Array<print_preview.Destination>} destinations List of
     *     destinations to render.
     * @private
     */
    renderDestinationsList_: function(destinations) {
      // Update item counters, footers and other misc controls.
      setIsVisible(
          this.getChildElement('.no-destinations-message'),
          destinations.length == 0);
      setIsVisible(this.getChildElement('.destination-list > footer'), false);
      let numItems = destinations.length;
      if (destinations.length > this.shortListSize_ && !this.isShowAll_) {
        numItems = this.shortListSize_ - 1;
        this.getChildElement('.total').textContent =
            loadTimeData.getStringF('destinationCount', destinations.length);
        setIsVisible(this.getChildElement('.destination-list > footer'), true);
      }
      // Remove obsolete list items (those with no corresponding destinations).
      this.listItems_ = this.listItems_.filter(item => {
        const isValid =
            this.destinationIds_.hasOwnProperty(item.destination.id);
        if (!isValid)
          this.removeChild(item);
        return isValid;
      });
      // Prepare id -> list item cache for visible destinations.
      const visibleListItems = {};
      for (let i = 0; i < numItems; i++)
        visibleListItems[destinations[i].id] = null;
      // Update visibility for all existing list items.
      this.listItems_.forEach(function(item) {
        const isVisible = visibleListItems.hasOwnProperty(item.destination.id);
        setIsVisible(item.getElement(), isVisible);
        if (isVisible)
          visibleListItems[item.destination.id] = item;
      });
      // Update the existing items, add the new ones (preserve the focused one).
      const listEl = this.getChildElement('.destination-list > ul');
      // We need to use activeElement instead of :focus selector, which doesn't
      // work in an inactive page. See crbug.com/723579.
      const focusedEl = listEl.contains(document.activeElement) ?
          document.activeElement :
          null;
      for (let i = 0; i < numItems; i++) {
        const destination = assert(destinations[i]);
        const listItem = visibleListItems[destination.id];
        if (listItem) {
          // Destination ID is the same, but it can be registered to a different
          // user account, hence passing it to the item update.
          this.updateListItem_(listEl, listItem, focusedEl, destination);
        } else {
          this.renderListItem_(listEl, destination);
        }
      }
    },

    /**
     * @param {Element} listEl List element.
     * @param {!print_preview.DestinationListItem} listItem List item to update.
     * @param {Element} focusedEl Currently focused element within the listEl.
     * @param {!print_preview.Destination} destination Destination to render.
     * @private
     */
    updateListItem_: function(listEl, listItem, focusedEl, destination) {
      listItem.update(destination, this.query_);

      const itemEl = listItem.getElement();
      // Preserve focused inner element, if there's one.
      const focusedInnerEl =
          focusedEl && itemEl.contains(focusedEl) ? focusedEl : null;
      if (focusedEl)
        itemEl.classList.add('moving');
      // Move it to the end of the list.
      listEl.appendChild(itemEl);
      // Restore focus.
      if (focusedEl) {
        if (focusedEl == itemEl || focusedEl == focusedInnerEl)
          focusedEl.focus();
        itemEl.classList.remove('moving');
      }
    },

    /**
     * @param {Element} listEl List element.
     * @param {!print_preview.Destination} destination Destination to render.
     * @private
     */
    renderListItem_: function(listEl, destination) {
      const listItem = new print_preview.DestinationListItem(
          this.eventTarget_, destination, this.query_);
      this.addChild(listItem);
      listItem.render(assert(listEl));
      this.listItems_.push(listItem);
    },

    /**
     * Called when the action link is clicked. Dispatches an
     * ACTION_LINK_ACTIVATED event.
     * @private
     */
    onActionLinkClick_: function() {
      cr.dispatchSimpleEvent(
          this, DestinationList.EventType.ACTION_LINK_ACTIVATED);
    }
  };

  // Export
  return {DestinationList: DestinationList};
});
