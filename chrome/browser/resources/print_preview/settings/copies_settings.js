// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  /**
   * Component that renders the copies settings UI.
   * @param {!print_preview.ticket_items.Copies} copiesTicketItem Used to read
   *     and write the copies value.
   * @param {!print_preview.ticket_items.Collate} collateTicketItem Used to read
   *     and write the collate value.
   * @constructor
   * @extends {print_preview.SettingsSection}
   */
  function CopiesSettings(copiesTicketItem, collateTicketItem) {
    print_preview.SettingsSection.call(this);

    /**
     * Used to read and write the copies value.
     * @type {!print_preview.ticket_items.Copies}
     * @private
     */
    this.copiesTicketItem_ = copiesTicketItem;

    /**
     * Used to read and write the collate value.
     * @type {!print_preview.ticket_items.Collate}
     * @private
     */
    this.collateTicketItem_ = collateTicketItem;

    /**
     * Timeout used to delay processing of the copies input in ms
     * @type {?number}
     * @private
     */
    this.textfieldTimeout_ = null;

    /**
     * Whether this component is enabled or not.
     * @type {boolean}
     * @private
     */
    this.isEnabled_ = true;

    /**
     * The element for the user input value.
     * @type {HTMLElement}
     * @private
     */
    this.inputField_ = null;
  }

  /**
   * Delay in milliseconds before processing the textfield.
   * @type {number}
   * @private
   */
  CopiesSettings.TEXTFIELD_DELAY_MS_ = 250;

  CopiesSettings.prototype = {
    __proto__: print_preview.SettingsSection.prototype,

    /** @override */
    isAvailable: function() {
      return this.copiesTicketItem_.isCapabilityAvailable();
    },

    /** @override */
    hasCollapsibleContent: function() {
      return false;
    },

    /** @override */
    set isEnabled(isEnabled) {
      this.inputField_.disabled = !isEnabled;
      this.getChildElement('input.collate').disabled = !isEnabled;
      this.isEnabled_ = isEnabled;
      if (isEnabled) {
        this.updateState_();
      }
    },

    /** @override */
    enterDocument: function() {
      this.inputField_ = this.getChildElement('input.user-value');
      print_preview.SettingsSection.prototype.enterDocument.call(this);
      this.tracker.add(
          this.inputField_, 'keydown', this.onTextfieldKeyDown_.bind(this));
      this.tracker.add(
          this.inputField_, 'input', this.onTextfieldInput_.bind(this));
      this.tracker.add(
          this.inputField_, 'blur', this.onTextfieldBlur_.bind(this));
      this.tracker.add(
          this.getChildElement('input.collate'), 'click',
          this.onCollateCheckboxClick_.bind(this));
      this.tracker.add(
          this.copiesTicketItem_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.updateState_.bind(this));
      this.tracker.add(
          this.collateTicketItem_,
          print_preview.ticket_items.TicketItem.EventType.CHANGE,
          this.updateState_.bind(this));
    },

    /**
     * Updates the state of the copies settings UI controls.
     * @private
     */
    updateState_: function() {
      if (this.isAvailable()) {
        if (!this.inputField_.validity.valid) {
          this.inputField_.classList.add('invalid');
          fadeInElement(this.getChildElement('.hint'));
          this.getChildElement('.collate-container').hidden = true;
          this.updateUiStateInternal();
          return;
        }
        if (this.inputField_.value != this.copiesTicketItem_.getValue())
          this.inputField_.value = this.copiesTicketItem_.getValue();
        this.inputField_.classList.remove('invalid');
        fadeOutElement(this.getChildElement('.hint'));
        if (!(this.getChildElement('.collate-container').hidden =
                  !this.collateTicketItem_.isCapabilityAvailable() ||
                  this.copiesTicketItem_.getValueAsNumber() <= 1)) {
          this.getChildElement('input.collate').checked =
              this.collateTicketItem_.getValue();
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
      const newValue =
          (this.inputField_.validity.valid && this.inputField_.value != '') ?
          this.inputField_.valueAsNumber.toString() :
          '';
      if (this.copiesTicketItem_.getValue() === newValue) {
        this.updateState_();
        return;
      }
      this.copiesTicketItem_.updateValue(newValue);
    },

    /**
     * Called when a key is pressed on the custom input.
     * @param {Event} event Contains the key that was pressed.
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
     * timeout.
     * @private
     */
    onTextfieldInput_: function() {
      if (this.textfieldTimeout_) {
        clearTimeout(this.textfieldTimeout_);
      }
      this.textfieldTimeout_ = setTimeout(
          this.onTextfieldTimeout_.bind(this),
          CopiesSettings.TEXTFIELD_DELAY_MS_);
    },

    /**
     * Called when the focus leaves the textfield. If the textfield is empty,
     * its value is set to 1.
     * @private
     */
    onTextfieldBlur_: function() {
      if (this.inputField_.validity.valid && this.inputField_.value == '') {
        if (this.copiesTicketItem_.getValue() == '1') {
          // No need to update the ticket, but change the display to match.
          this.inputField_.value = '1';
        } else {
          setTimeout(() => {
            this.copiesTicketItem_.updateValue('1');
          }, 0);
        }
      }
    },

    /**
     * Called when the collate checkbox is clicked. Updates the print ticket.
     * @private
     */
    onCollateCheckboxClick_: function() {
      this.collateTicketItem_.updateValue(
          this.getChildElement('input.collate').checked);
    }
  };

  // Export
  return {CopiesSettings: CopiesSettings};
});
