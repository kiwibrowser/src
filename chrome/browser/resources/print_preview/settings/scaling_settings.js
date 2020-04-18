// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Component that renders the scaling settings UI.
   * @param {!print_preview.ticket_items.Scaling} scalingTicketItem
   *     Used to read and write the scaling value.
   * @param {!print_preview.ticket_items.FitToPage}
   *     fitToPageTicketItem Used to read the fit to page value.
   * @constructor
   * @extends {print_preview.SettingsSection}
   */
  function ScalingSettings(scalingTicketItem, fitToPageTicketItem) {
    print_preview.SettingsSection.call(this);

    /**
     * Used to read and write the scaling value.
     * @private {!print_preview.ticket_items.Scaling}
     */
    this.scalingTicketItem_ = scalingTicketItem;

    /**
     * Used to read the fit to page value
     * @private {!print_preview.ticket_items.FitToPage}
     */
    this.fitToPageTicketItem_ = fitToPageTicketItem;

    /**
     * The scaling percentage required to fit the document to page
     * @private {number}
     */
    this.fitToPageScaling_ = 100;

    /**
     * Timeout used to delay processing of the scaling input, in ms.
     * @private {?number}
     */
    this.textfieldTimeout_ = null;

    /**
     * Whether this component is enabled or not.
     * @private {boolean}
     */
    this.isEnabled_ = true;

    /**
     * Last valid scaling value. Used to restore value when fit to page is
     * unchecked. Must always be a valid scaling value.
     * @private {string}
     */
    this.lastValidScaling_ = '100';

    /**
     * The textfield input element
     * @private {?HTMLElement}
     */
    this.inputField_ = null;

    /**
     * The fit to page checkbox.
     * @private {?HTMLElement}
     */
    this.fitToPageCheckbox_ = null;
  }

  /**
   * Delay in milliseconds before processing the textfield.
   * @private {number}
   * @const
   */
  ScalingSettings.TEXTFIELD_DELAY_MS_ = 250;

  ScalingSettings.prototype = {
    __proto__: print_preview.SettingsSection.prototype,

    /** @override */
    isAvailable: function() {
      return this.scalingTicketItem_.isCapabilityAvailable() ||
          this.fitToPageTicketItem_.isCapabilityAvailable();
    },

    /** @override */
    hasCollapsibleContent: function() {
      return this.scalingTicketItem_.isCapabilityAvailable();
    },

    /** @override */
    set isEnabled(isEnabled) {
      this.inputField_.disabled = !isEnabled;
      this.fitToPageCheckbox_.disabled = !isEnabled;
      this.isEnabled_ = isEnabled;
    },

    /**
     * @return {boolean} Whether fit to page is available and selected.
     * @private
     */
    isFitToPageSelected_: function() {
      return this.fitToPageTicketItem_.isCapabilityAvailable() &&
          /** @type {boolean} */ (this.fitToPageTicketItem_.getValue());
    },

    /** @override */
    enterDocument: function() {
      this.inputField_ = assert(this.getChildElement('input.user-value'));
      this.fitToPageCheckbox_ =
          assert(this.getChildElement('input[type=checkbox]'));
      print_preview.SettingsSection.prototype.enterDocument.call(this);
      this.tracker.add(
          this.inputField_, 'keydown', this.onTextfieldKeyDown_.bind(this));
      this.tracker.add(
          this.inputField_, 'input', this.onTextfieldInput_.bind(this));
      this.tracker.add(
          this.inputField_, 'blur', this.onTextfieldBlur_.bind(this));
      this.tracker.add(
          this.fitToPageCheckbox_, 'click',
          this.onFitToPageClicked_.bind(this));
      this.tracker.add(
          this.scalingTicketItem_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.updateState_.bind(this, false));
      this.tracker.add(
          this.fitToPageTicketItem_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.updateState_.bind(this, true));
    },

    /**
     * Display the fit to page scaling in the scaling field if there is a valid
     * fit to page scaling value. If not, make the field blank. The fit to page
     * value is always considered valid, so remove the hint if it is displayed.
     * @private
     */
    displayFitToPageScaling: function() {
      this.fitToPageCheckbox_.checked = true;
      this.inputField_.value = this.fitToPageScaling_ || '';
      this.removeHint_();
    },

    /**
     * Updates the fit to page scaling value of the scalings settings UI.
     * @param {number} fitToPageScaling The updated percentage scaling required
     *     to fit the document to page.
     */
    updateFitToPageScaling: function(fitToPageScaling) {
      this.fitToPageScaling_ = fitToPageScaling;
      if (this.isFitToPageSelected_())
        this.displayFitToPageScaling();
    },

    /**
     * Removes the error message and red background from the input.
     * @private
     */
    removeHint_: function() {
      this.inputField_.classList.remove('invalid');
      fadeOutElement(this.getChildElement('.hint'));
    },

    /** @override */
    updateUiStateInternal: function() {
      setIsVisible(
          this.getChildElement('#fit-to-page-container'),
          this.fitToPageTicketItem_.isCapabilityAvailable());
      setIsVisible(
          this.getChildElement('.settings-box'),
          this.scalingTicketItem_.isCapabilityAvailable() &&
              !this.collapseContent);
      print_preview.SettingsSection.prototype.updateUiStateInternal.call(this);
    },

    /** @override */
    isSectionVisibleInternal: function() {
      return this.fitToPageTicketItem_.isCapabilityAvailable() ||
          (!this.collapseContent &&
           this.scalingTicketItem_.isCapabilityAvailable());
    },

    /**
     * Updates the state of the scaling settings UI controls.
     * @param {boolean} fitToPageChange Whether this update is due to a change
     *     to the fit to page ticket item.
     * @private
     */
    updateState_: function(fitToPageChange) {
      if (this.isAvailable()) {
        if (fitToPageChange && this.isFitToPageSelected_()) {
          // Fit to page was checked. Set scaling to the fit to page scaling.
          this.displayFitToPageScaling();
          this.scalingTicketItem_.updateValue(this.lastValidScaling_);
        } else if (
            fitToPageChange &&
            this.fitToPageTicketItem_.isCapabilityAvailable()) {
          // Fit to page unchecked. Return to last valid scaling.
          this.fitToPageCheckbox_.checked = false;
          if (this.scalingTicketItem_.getValue() == this.lastValidScaling_)
            this.inputField_.value = this.lastValidScaling_;
          else
            this.scalingTicketItem_.updateValue(this.lastValidScaling_);
        } else if (!this.scalingTicketItem_.isValid()) {
          // User entered invalid scaling value, display error message.
          this.inputField_.classList.add('invalid');
          fadeInElement(this.getChildElement('.hint'));
        } else {
          // User entered valid scaling. Update the field and last valid.
          if (!this.isFitToPageSelected_())
            this.inputField_.value = this.scalingTicketItem_.getValue();
          this.lastValidScaling_ =
              /** @type {string} */ (this.scalingTicketItem_.getValue());
          this.removeHint_();
        }
      }
      this.updateUiStateInternal();
    },

    /**
     * Called after a timeout after user input into the textfield.
     * @private
     */
    onTextfieldTimeout_: function() {
      this.textfieldTimeout_ = null;
      if (this.inputField_.value == '')
        return;
      // Convert value to a valid number or ''. The scaling ticket item assumes
      // the only invalid value is ''.
      const value =
          (this.inputField_.validity.valid && this.inputField_.value != '') ?
          this.inputField_.valueAsNumber.toString() :
          '';
      if (value != '' && this.isFitToPageSelected_())
        this.fitToPageTicketItem_.updateValue(false);
      this.scalingTicketItem_.updateValue(value);
    },

    /**
     * Called when a key is pressed on the custom input.
     * @param {!Event} event Contains the key that was pressed.
     * @private
     */
    onTextfieldKeyDown_: function(event) {
      if (event.keyCode != 'Enter')
        return;

      if (this.textfieldTimeout_)
        clearTimeout(this.textfieldTimeout_);
      this.onTextfieldTimeout_();
    },

    /**
     * Called when a input event occurs on the textfield. Starts an input
     * timeout and unchecks fit to page.
     * @private
     */
    onTextfieldInput_: function() {
      this.fitToPageCheckbox_.checked = false;

      if (this.textfieldTimeout_) {
        clearTimeout(this.textfieldTimeout_);
      }
      this.textfieldTimeout_ = setTimeout(
          this.onTextfieldTimeout_.bind(this),
          ScalingSettings.TEXTFIELD_DELAY_MS_);
    },

    /**
     * Called when the focus leaves the textfield. If the textfield is empty,
     * its value is set to the fit to page value if fit to page is checked and
     * 100 otherwise.
     * @private
     */
    onTextfieldBlur_: function() {
      if (this.inputField_.value == '') {
        if (this.isFitToPageSelected_() && this.fitToPageCheckbox_.checked)
          return;
        if (this.isFitToPageSelected_() && this.inputField_.validity.valid)
          this.fitToPageTicketItem_.updateValue(false);
        if (this.scalingTicketItem_.getValue() == '100') {
          // No need to update the ticket, but change the display to match.
          this.updateState_(false);
        } else {
          this.scalingTicketItem_.updateValue('100');
        }
      }
    },

    /**
     * Called when the fit to page checkbox is clicked. Updates the fit to page
     * ticket item and the display.
     * @private
     */
    onFitToPageClicked_: function() {
      if (this.fitToPageTicketItem_.getValue() ==
          this.fitToPageCheckbox_.checked) {
        this.updateState_(true);
      } else {
        this.fitToPageTicketItem_.updateValue(this.fitToPageCheckbox_.checked);
      }
    },
  };

  // Export
  return {ScalingSettings: ScalingSettings};
});
