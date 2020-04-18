// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Represents a single option in the Other Options settings section.
   * @param {!print_preview.ticket_items.TicketItem} ticketItem The ticket item
   *     for this element, used to read and write.
   * @param {boolean} collapsible Whether this option is collapsible or not.
   * @param {string} cssId The CSS id for the container element for this option,
   *     used to access the container and checkbox HTML elements.
   * @constructor
   */
  function CheckboxTicketItemElement(ticketItem, collapsible, cssId) {
    /**
     * Ticket item for this element, used to read/write.
     * @private {!print_preview.ticket_items.TicketItem}
     */
    this.ticketItem_ = ticketItem;

    /**
     * Whether this element is collapsible.
     * @private {boolean}
     */
    this.collapsible_ = collapsible;

    /**
     * The CSS id string for the container element.
     * @private {string}
     */
    this.cssId_ = cssId;

    /**
     * The HTML container element. Populated when decorate() is called.
     * @private {HTMLElement}
     */
    this.container_ = null;

    /**
     * The HTML checkbox input element. The checkbox child element of
     * container_. Populated when decorate() is called.
     * @private {HTMLElement}
     */
    this.checkbox_ = null;
  }

  CheckboxTicketItemElement.prototype = {

    /** @return {boolean} Whether the element is collapsible */
    get collapsible() {
      return this.collapsible_;
    },

    /**
     * @return {!print_preview.ticket_items.TicketItem} The ticket item for this
     *      element.
     */
    get ticketItem() {
      return this.ticketItem_;
    },

    /** @return {HTMLElement} The checkbox HTML element. */
    get checkbox() {
      return this.checkbox_;
    },

    /** Initializes container and checkbox */
    decorate: function() {
      this.container_ = $(this.cssId_);
      this.checkbox_ = /** @type {HTMLElement} */ (
          this.container_.querySelector('.checkbox'));
    },

    /** Resets container and checkbox. */
    exitDocument: function() {
      this.container_ = null;
      this.checkbox_ = null;
    },

    /** Called when the checkbox is clicked. Updates the ticket item value. */
    onCheckboxClick: function() {
      this.ticketItem_.updateValue(this.checkbox_.checked);
    },

    /**
     * Called when the ticket item changes. Updates the UI state.
     * @param {!print_preview.OtherOptionsSettings}
     *     otherOptionsSettings The settings section that this element is part
     *     of.
     */
    onTicketItemChange: function(otherOptionsSettings) {
      this.checkbox_.checked = this.ticketItem_.getValue();
      otherOptionsSettings.updateUiStateInternal();
    },

    /**
     * @param {boolean} collapseContent Whether the settings section has content
     *     collapsed.
     * @return {boolean} Whether this element should be visible.
     */
    isVisible: function(collapseContent) {
      return this.ticketItem_.isCapabilityAvailable() &&
          (!this.collapsible_ || !collapseContent);
    },

    /**
     * Sets the visibility of the element.
     * @param {boolean} collapseContent Whether the settings section has content
     *     collapsed.
     */
    setVisibility: function(collapseContent) {
      setIsVisible(assert(this.container_), this.isVisible(collapseContent));
    },

  };

  /**
   * UI component that renders checkboxes for various print options.
   * @param {!print_preview.ticket_items.Duplex} duplex Duplex ticket item.
   * @param {!print_preview.ticket_items.CssBackground} cssBackground CSS
   *     background ticket item.
   * @param {!print_preview.ticket_items.SelectionOnly} selectionOnly Selection
   *     only ticket item.
   * @param {!print_preview.ticket_items.HeaderFooter} headerFooter Header
   *     footer ticket item.
   * @param {!print_preview.ticket_items.Rasterize} rasterize Rasterize ticket
   *     item.
   * @constructor
   * @extends {print_preview.SettingsSection}
   */
  function OtherOptionsSettings(
      duplex, cssBackground, selectionOnly, headerFooter, rasterize) {
    print_preview.SettingsSection.call(this);
    /**
     * @private {boolean} rasterizeEnabled Whether the print as image feature is
     *     enabled.
     */
    this.rasterizeEnabled_ = !cr.isWindows && !cr.isMac;

    /**
     * @private {!Array<!CheckboxTicketItemElement>} checkbox ticket item
     *      elements representing the different options in the section.
     *      Selection only must always be the last element in the array.
     */
    this.elements_ = [
      new CheckboxTicketItemElement(
          headerFooter, true, 'header-footer-container'),
      new CheckboxTicketItemElement(duplex, false, 'duplex-container'),
      new CheckboxTicketItemElement(
          cssBackground, true, 'css-background-container'),
      new CheckboxTicketItemElement(
          selectionOnly, true, 'selection-only-container')
    ];
    if (this.rasterizeEnabled_) {
      this.elements_.splice(
          this.elements_.length - 1, 0,
          new CheckboxTicketItemElement(
              rasterize, true, 'rasterize-container'));
    }
  }

  OtherOptionsSettings.prototype = {
    __proto__: print_preview.SettingsSection.prototype,

    /** @override */
    isAvailable: function() {
      return this.elements_.some(function(element) {
        return element.ticketItem.isCapabilityAvailable();
      });
    },

    /** @override */
    hasCollapsibleContent: function() {
      return this.elements_.some(function(element) {
        return element.collapsible;
      });
    },

    /** @override */
    set isEnabled(isEnabled) {
      /* Skip |ticket_items.SelectionOnly|, which must always be the last
       * element, as this checkbox is enabled based on whether the user has
       * selected something in the page, which is different logic from the
       * other elements. */
      for (let i = 0; i < this.elements_.length - 1; i++)
        this.elements_[i].checkbox.disabled = !isEnabled;
    },

    /** @override */
    enterDocument: function() {
      print_preview.SettingsSection.prototype.enterDocument.call(this);
      this.elements_.forEach(function(element) {
        this.tracker.add(
            assert(element.checkbox), 'click',
            element.onCheckboxClick.bind(element));
        this.tracker.add(
            element.ticketItem,
            print_preview.ticket_items.TicketItem.EventType.CHANGE,
            element.onTicketItemChange.bind(element, this));
      }, this);
    },

    /** @override */
    exitDocument: function() {
      print_preview.SettingsSection.prototype.exitDocument.call(this);
      for (let i = 0; i < this.elements_.length; i++)
        this.elements_[i].exitDocument();
    },

    /** @override */
    decorateInternal: function() {
      for (let i = 0; i < this.elements_.length; i++)
        this.elements_[i].decorate();
      $('rasterize-container').hidden = !this.rasterizeEnabled_;
    },

    /**
     * @public
     * @override
     */
    updateUiStateInternal: function() {
      if (this.isAvailable()) {
        for (let i = 0; i < this.elements_.length; i++)
          this.elements_[i].setVisibility(this.collapseContent);
      }
      print_preview.SettingsSection.prototype.updateUiStateInternal.call(this);
    },

    /** @override */
    isSectionVisibleInternal: function() {
      return this.elements_.some(function(element) {
        return element.isVisible(this.collapseContent);
      }, this);
    },

  };

  // Export
  return {OtherOptionsSettings: OtherOptionsSettings};
});
