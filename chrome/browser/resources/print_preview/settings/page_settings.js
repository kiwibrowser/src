// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Creates a PageSettings object. This object encapsulates all settings and
   * logic related to page selection.
   * @param {!print_preview.ticket_items.PageRange} pageRangeTicketItem Used to
   *     read and write page range settings.
   * @constructor
   * @extends {print_preview.SettingsSection}
   */
  function PageSettings(pageRangeTicketItem) {
    print_preview.SettingsSection.call(this);

    /**
     * Used to read and write page range settings.
     * @type {!print_preview.ticket_items.PageRange}
     * @private
     */
    this.pageRangeTicketItem_ = pageRangeTicketItem;

    /**
     * Timeout used to delay processing of the custom page range input.
     * @type {?number}
     * @private
     */
    this.customInputTimeout_ = null;

    /**
     * Custom page range input.
     * @type {?HTMLInputElement}
     * @private
     */
    this.customInput_ = null;

    /**
     * Custom page range radio button.
     * @type {?HTMLInputElement}
     * @private
     */
    this.customRadio_ = null;

    /**
     * Custom page range label.
     * @type {?HTMLElement}
     * @private
     */
    this.customLabel_ = null;

    /**
     * All page rage radio button.
     * @type {?HTMLInputElement}
     * @private
     */
    this.allRadio_ = null;

    /**
     * Container of a hint to show when the custom page range is invalid.
     * @type {?HTMLElement}
     * @private
     */
    this.customHintEl_ = null;
  }

  /**
   * CSS classes used by the page settings.
   * @enum {string}
   * @private
   */
  PageSettings.Classes_ = {
    ALL_RADIO: 'page-settings-all-radio',
    CUSTOM_HINT: 'page-settings-custom-hint',
    CUSTOM_INPUT: 'page-settings-custom-input',
    CUSTOM_LABEL: 'page-settings-print-pages-div',
    CUSTOM_RADIO: 'page-settings-custom-radio'
  };

  /**
   * Delay in milliseconds before processing custom page range input.
   * @type {number}
   * @private
   */
  PageSettings.CUSTOM_INPUT_DELAY_ = 500;

  PageSettings.prototype = {
    __proto__: print_preview.SettingsSection.prototype,

    /** @override */
    isAvailable: function() {
      return this.pageRangeTicketItem_.isCapabilityAvailable();
    },

    /** @override */
    hasCollapsibleContent: function() {
      return false;
    },

    /** @override */
    set isEnabled(isEnabled) {
      this.customInput_.disabled = !isEnabled;
      this.allRadio_.disabled = !isEnabled;
      this.customRadio_.disabled = !isEnabled;
    },

    /** @override */
    enterDocument: function() {
      print_preview.SettingsSection.prototype.enterDocument.call(this);
      const customInput = assert(this.customInput_);
      this.tracker.add(
          assert(this.allRadio_), 'click', this.onAllRadioClick_.bind(this));
      this.tracker.add(
          assert(this.customRadio_), 'click',
          this.focusCustomInput_.bind(this));
      this.tracker.add(customInput, 'blur', this.onCustomInputBlur_.bind(this));
      this.tracker.add(
          customInput, 'focus', this.onCustomInputFocus_.bind(this));
      this.tracker.add(
          customInput, 'keydown', this.onCustomInputKeyDown_.bind(this));
      this.tracker.add(
          customInput, 'input', this.onCustomInputChange_.bind(this));
      this.tracker.add(
          assert(this.customLabel_), 'focus',
          this.focusCustomInput_.bind(this));
      this.tracker.add(
          this.pageRangeTicketItem_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.onPageRangeTicketItemChange_.bind(this));
    },

    /** @override */
    exitDocument: function() {
      print_preview.SettingsSection.prototype.exitDocument.call(this);
      this.customInput_ = null;
      this.customRadio_ = null;
      this.allRadio_ = null;
      this.customHintEl_ = null;
    },

    /** @override */
    decorateInternal: function() {
      this.customInput_ = this.getElement().getElementsByClassName(
          PageSettings.Classes_.CUSTOM_INPUT)[0];
      this.allRadio_ = this.getElement().getElementsByClassName(
          PageSettings.Classes_.ALL_RADIO)[0];
      this.customRadio_ = this.getElement().getElementsByClassName(
          PageSettings.Classes_.CUSTOM_RADIO)[0];
      this.customHintEl_ = this.getElement().getElementsByClassName(
          PageSettings.Classes_.CUSTOM_HINT)[0];
      this.customLabel_ = this.getElement().getElementsByClassName(
          PageSettings.Classes_.CUSTOM_LABEL)[0];
    },

    /**
     * @param {!PageRangeStatus} validity (of page range)
     * @private
     */
    setInvalidStateVisible_: function(validity) {
      if (validity === PageRangeStatus.NO_ERROR) {
        this.customInput_.classList.remove('invalid');
        fadeOutElement(this.customHintEl_);
        return;
      }
      let message;
      if (validity === PageRangeStatus.LIMIT_ERROR) {
        if (this.pageRangeTicketItem_.getDocumentNumPages()) {
          message = loadTimeData.getStringF(
              'pageRangeLimitInstructionWithValue',
              this.pageRangeTicketItem_.getDocumentNumPages());
        } else {
          message = loadTimeData.getString('pageRangeLimitInstruction');
        }
      } else {
        message = loadTimeData.getStringF(
            'pageRangeSyntaxInstruction',
            loadTimeData.getString('examplePageRangeText'));
      }
      this.customHintEl_.textContent = message;
      this.customInput_.classList.add('invalid');
      fadeInElement(this.customHintEl_);
    },

    /**
     * Called when the all radio button is clicked. Updates the print ticket.
     * @private
     */
    onAllRadioClick_: function() {
      this.pageRangeTicketItem_.updateValue(null);
    },

    /**
     * Focuses the custom input. Called when the custom radio button or custom
     * input label is clicked.
     * @private
     */
    focusCustomInput_: function() {
      this.customInput_.focus();
    },

    /**
     * Called when the custom input is blurred. Enables the all radio button if
     * the custom input is empty.
     * @private
     */
    onCustomInputBlur_: function(event) {
      if (this.customInput_.value == '' &&
          event.relatedTarget !=
              this.getElement().querySelector(
                  '.page-settings-print-pages-div') &&
          event.relatedTarget != this.customRadio_) {
        this.allRadio_.checked = true;
        if (!this.pageRangeTicketItem_.isValueEqual(this.customInput_.value)) {
          // Update ticket item to match, set timeout to avoid losing focus
          // when the preview regenerates.
          setTimeout(() => {
            this.pageRangeTicketItem_.updateValue(this.customInput_.value);
          });
        }
      }
    },

    /**
     * Called when the custom input is focused.
     * @private
     */
    onCustomInputFocus_: function() {
      this.customRadio_.checked = true;
      this.pageRangeTicketItem_.updateValue(this.customInput_.value);
    },

    /**
     * Called when a key is pressed on the custom input.
     * @param {Event} event Contains the key that was pressed.
     * @private
     */
    onCustomInputKeyDown_: function(event) {
      if (event.keyCode == 13 /*enter*/) {
        if (this.customInputTimeout_) {
          clearTimeout(this.customInputTimeout_);
          this.customInputTimeout_ = null;
        }
        this.pageRangeTicketItem_.updateValue(this.customInput_.value);
      }
    },

    /**
     * Called after a delay following a key press in the custom input.
     * @private
     */
    onCustomInputTimeout_: function() {
      this.customInputTimeout_ = null;
      if (this.customRadio_.checked) {
        this.pageRangeTicketItem_.updateValue(this.customInput_.value);
      }
    },

    /**
     * Called for events that change the text - undo, redo, paste and
     * keystrokes outside of enter, copy, etc. (Re)starts the
     * re-evaluation timer.
     * @private
     */
    onCustomInputChange_: function() {
      if (this.customInputTimeout_)
        clearTimeout(this.customInputTimeout_);
      this.customInputTimeout_ = setTimeout(
          this.onCustomInputTimeout_.bind(this),
          PageSettings.CUSTOM_INPUT_DELAY_);
    },

    /**
     * Called when the print ticket changes. Updates the state of the component.
     * @private
     */
    onPageRangeTicketItemChange_: function() {
      if (this.isAvailable()) {
        const pageRangeStr = this.pageRangeTicketItem_.getValue();
        if (pageRangeStr || this.customRadio_.checked) {
          if (!document.hasFocus() ||
              document.activeElement != this.customInput_) {
            this.customInput_.value = pageRangeStr;
          }
          this.customRadio_.checked = true;
          this.setInvalidStateVisible_(
              this.pageRangeTicketItem_.checkValidity());
        } else {
          this.allRadio_.checked = true;
          this.setInvalidStateVisible_(PageRangeStatus.NO_ERROR);
        }
      }
      this.updateUiStateInternal();
    }
  };

  // Export
  return {PageSettings: PageSettings};
});
