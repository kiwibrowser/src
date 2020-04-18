// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-autofill-section' is the section containing saved
 * addresses and credit cards for use in autofill.
 */

/**
 * Interface for all callbacks to the autofill API.
 * @interface
 */
class AutofillManager {
  /**
   * Add an observer to the list of addresses.
   * @param {function(!Array<!AutofillManager.AddressEntry>):void} listener
   */
  addAddressListChangedListener(listener) {}

  /**
   * Remove an observer from the list of addresses.
   * @param {function(!Array<!AutofillManager.AddressEntry>):void} listener
   */
  removeAddressListChangedListener(listener) {}

  /**
   * Request the list of addresses.
   * @param {function(!Array<!AutofillManager.AddressEntry>):void} callback
   */
  getAddressList(callback) {}

  /**
   * Saves the given address.
   * @param {!AutofillManager.AddressEntry} address
   */
  saveAddress(address) {}

  /** @param {string} guid The guid of the address to remove.  */
  removeAddress(guid) {}

  /**
   * Add an observer to the list of credit cards.
   * @param {function(!Array<!AutofillManager.CreditCardEntry>):void} listener
   */
  addCreditCardListChangedListener(listener) {}

  /**
   * Remove an observer from the list of credit cards.
   * @param {function(!Array<!AutofillManager.CreditCardEntry>):void} listener
   */
  removeCreditCardListChangedListener(listener) {}

  /**
   * Request the list of credit cards.
   * @param {function(!Array<!AutofillManager.CreditCardEntry>):void} callback
   */
  getCreditCardList(callback) {}

  /** @param {string} guid The GUID of the credit card to remove.  */
  removeCreditCard(guid) {}

  /** @param {string} guid The GUID to credit card to remove from the cache. */
  clearCachedCreditCard(guid) {}

  /**
   * Saves the given credit card.
   * @param {!AutofillManager.CreditCardEntry} creditCard
   */
  saveCreditCard(creditCard) {}
}

/** @typedef {chrome.autofillPrivate.AddressEntry} */
AutofillManager.AddressEntry;

/** @typedef {chrome.autofillPrivate.CreditCardEntry} */
AutofillManager.CreditCardEntry;

/**
 * Implementation that accesses the private API.
 * @implements {AutofillManager}
 */
class AutofillManagerImpl {
  /** @override */
  addAddressListChangedListener(listener) {
    chrome.autofillPrivate.onAddressListChanged.addListener(listener);
  }

  /** @override */
  removeAddressListChangedListener(listener) {
    chrome.autofillPrivate.onAddressListChanged.removeListener(listener);
  }

  /** @override */
  getAddressList(callback) {
    chrome.autofillPrivate.getAddressList(callback);
  }

  /** @override */
  saveAddress(address) {
    chrome.autofillPrivate.saveAddress(address);
  }

  /** @override */
  removeAddress(guid) {
    chrome.autofillPrivate.removeEntry(assert(guid));
  }

  /** @override */
  addCreditCardListChangedListener(listener) {
    chrome.autofillPrivate.onCreditCardListChanged.addListener(listener);
  }

  /** @override */
  removeCreditCardListChangedListener(listener) {
    chrome.autofillPrivate.onCreditCardListChanged.removeListener(listener);
  }

  /** @override */
  getCreditCardList(callback) {
    chrome.autofillPrivate.getCreditCardList(callback);
  }

  /** @override */
  removeCreditCard(guid) {
    chrome.autofillPrivate.removeEntry(assert(guid));
  }

  /** @override */
  clearCachedCreditCard(guid) {
    chrome.autofillPrivate.maskCreditCard(assert(guid));
  }

  /** @override */
  saveCreditCard(creditCard) {
    chrome.autofillPrivate.saveCreditCard(creditCard);
  }
}

cr.addSingletonGetter(AutofillManagerImpl);

(function() {
'use strict';

Polymer({
  is: 'settings-autofill-section',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * An array of saved addresses.
     * @type {!Array<!AutofillManager.AddressEntry>}
     */
    addresses: Array,

    /**
     * The model for any address related action menus or dialogs.
     * @private {?chrome.autofillPrivate.AddressEntry}
     */
    activeAddress: Object,

    /** @private */
    showAddressDialog_: Boolean,

    /**
     * An array of saved credit cards.
     * @type {!Array<!AutofillManager.CreditCardEntry>}
     */
    creditCards: Array,

    /**
     * The model for any credit card related action menus or dialogs.
     * @private {?chrome.autofillPrivate.CreditCardEntry}
     */
    activeCreditCard: Object,

    /** @private */
    showCreditCardDialog_: Boolean,
  },

  listeners: {
    'save-address': 'saveAddress_',
    'save-credit-card': 'saveCreditCard_',
  },

  /**
   * The element to return focus to, when the currently active dialog is
   * closed.
   * @private {?HTMLElement}
   */
  activeDialogAnchor_: null,

  /**
   * @type {AutofillManager}
   * @private
   */
  autofillManager_: null,

  /**
   * @type {?function(!Array<!AutofillManager.AddressEntry>)}
   * @private
   */
  setAddressesListener_: null,

  /**
   * @type {?function(!Array<!AutofillManager.CreditCardEntry>)}
   * @private
   */
  setCreditCardsListener_: null,

  /** @override */
  attached: function() {
    // Create listener functions.
    /** @type {function(!Array<!AutofillManager.AddressEntry>)} */
    const setAddressesListener = list => {
      this.addresses = list;
    };

    /** @type {function(!Array<!AutofillManager.CreditCardEntry>)} */
    const setCreditCardsListener = list => {
      this.creditCards = list;
    };

    // Remember the bound reference in order to detach.
    this.setAddressesListener_ = setAddressesListener;
    this.setCreditCardsListener_ = setCreditCardsListener;

    // Set the managers. These can be overridden by tests.
    this.autofillManager_ = AutofillManagerImpl.getInstance();

    // Request initial data.
    this.autofillManager_.getAddressList(setAddressesListener);
    this.autofillManager_.getCreditCardList(setCreditCardsListener);

    // Listen for changes.
    this.autofillManager_.addAddressListChangedListener(setAddressesListener);
    this.autofillManager_.addCreditCardListChangedListener(
        setCreditCardsListener);
  },

  /** @override */
  detached: function() {
    this.autofillManager_.removeAddressListChangedListener(
        /** @type {function(!Array<!AutofillManager.AddressEntry>)} */ (
            this.setAddressesListener_));
    this.autofillManager_.removeCreditCardListChangedListener(
        /** @type {function(!Array<!AutofillManager.CreditCardEntry>)} */ (
            this.setCreditCardsListener_));
  },

  /**
   * Formats the expiration date so it's displayed as MM/YYYY.
   * @param {!chrome.autofillPrivate.CreditCardEntry} item
   * @return {string}
   * @private
   */
  expiration_: function(item) {
    return item.expirationMonth + '/' + item.expirationYear;
  },

  /**
   * Open the address action menu.
   * @param {!Event} e The polymer event.
   * @private
   */
  onAddressMenuTap_: function(e) {
    const menuEvent = /** @type {!{model: !{item: !Object}}} */ (e);

    /* TODO(scottchen): drop the [dataHost][dataHost] once this bug is fixed:
     https://github.com/Polymer/polymer/issues/2574 */
    const item = menuEvent.model['dataHost']['dataHost'].item;

    // Copy item so dialog won't update model on cancel.
    this.activeAddress = /** @type {!chrome.autofillPrivate.AddressEntry} */ (
        Object.assign({}, item));

    const dotsButton = /** @type {!HTMLElement} */ (Polymer.dom(e).localTarget);
    /** @type {!CrActionMenuElement} */ (this.$.addressSharedMenu)
        .showAt(dotsButton);
    this.activeDialogAnchor_ = dotsButton;
  },

  /**
   * Handles tapping on the "Add address" button.
   * @param {!Event} e The polymer event.
   * @private
   */
  onAddAddressTap_: function(e) {
    e.preventDefault();
    this.activeAddress = {};
    this.showAddressDialog_ = true;
    this.activeDialogAnchor_ = this.$.addAddress;
  },

  /** @private */
  onAddressDialogClosed_: function() {
    this.showAddressDialog_ = false;
    cr.ui.focusWithoutInk(assert(this.activeDialogAnchor_));
    this.activeDialogAnchor_ = null;
  },

  /**
   * Handles tapping on the "Edit" address button.
   * @param {!Event} e The polymer event.
   * @private
   */
  onMenuEditAddressTap_: function(e) {
    e.preventDefault();
    this.showAddressDialog_ = true;
    this.$.addressSharedMenu.close();
  },

  /** @private */
  onRemoteEditAddressTap_: function() {
    window.open(this.i18n('manageAddressesUrl'));
  },

  /**
   * Handles tapping on the "Remove" address button.
   * @private
   */
  onMenuRemoveAddressTap_: function() {
    this.autofillManager_.removeAddress(
        /** @type {string} */ (this.activeAddress.guid));
    this.$.addressSharedMenu.close();
  },

  /**
   * Opens the credit card action menu.
   * @param {!Event} e The polymer event.
   * @private
   */
  onCreditCardMenuTap_: function(e) {
    const menuEvent = /** @type {!{model: !{item: !Object}}} */ (e);

    /* TODO(scottchen): drop the [dataHost][dataHost] once this bug is fixed:
     https://github.com/Polymer/polymer/issues/2574 */
    const item = menuEvent.model['dataHost']['dataHost'].item;

    // Copy item so dialog won't update model on cancel.
    this.activeCreditCard =
        /** @type {!chrome.autofillPrivate.CreditCardEntry} */ (
            Object.assign({}, item));

    const dotsButton = /** @type {!HTMLElement} */ (Polymer.dom(e).localTarget);
    /** @type {!CrActionMenuElement} */ (this.$.creditCardSharedMenu)
        .showAt(dotsButton);
    this.activeDialogAnchor_ = dotsButton;
  },

  /**
   * Handles tapping on the "Add credit card" button.
   * @param {!Event} e
   * @private
   */
  onAddCreditCardTap_: function(e) {
    e.preventDefault();
    const date = new Date();  // Default to current month/year.
    const expirationMonth = date.getMonth() + 1;  // Months are 0 based.
    this.activeCreditCard = {
      expirationMonth: expirationMonth.toString(),
      expirationYear: date.getFullYear().toString(),
    };
    this.showCreditCardDialog_ = true;
    this.activeDialogAnchor_ = this.$.addCreditCard;
  },

  /** @private */
  onCreditCardDialogClosed_: function() {
    this.showCreditCardDialog_ = false;
    cr.ui.focusWithoutInk(assert(this.activeDialogAnchor_));
    this.activeDialogAnchor_ = null;
  },

  /**
   * Handles tapping on the "Edit" credit card button.
   * @param {!Event} e The polymer event.
   * @private
   */
  onMenuEditCreditCardTap_: function(e) {
    e.preventDefault();

    if (this.activeCreditCard.metadata.isLocal)
      this.showCreditCardDialog_ = true;
    else
      this.onRemoteEditCreditCardTap_();

    this.$.creditCardSharedMenu.close();
  },

  /** @private */
  onRemoteEditCreditCardTap_: function() {
    window.open(this.i18n('manageCreditCardsUrl'));
  },

  /**
   * Handles tapping on the "Remove" credit card button.
   * @private
   */
  onMenuRemoveCreditCardTap_: function() {
    this.autofillManager_.removeCreditCard(
        /** @type {string} */ (this.activeCreditCard.guid));
    this.$.creditCardSharedMenu.close();
  },

  /**
   * Handles tapping on the "Clear copy" button for cached credit cards.
   * @private
   */
  onMenuClearCreditCardTap_: function() {
    this.autofillManager_.clearCachedCreditCard(
        /** @type {string} */ (this.activeCreditCard.guid));
    this.$.creditCardSharedMenu.close();
  },

  /**
   * The 3-dot menu should not be shown if the card is entirely remote.
   * @param {!chrome.autofillPrivate.AutofillMetadata} metadata
   * @return {boolean}
   * @private
   */
  showDots_: function(metadata) {
    return !!(metadata.isLocal || metadata.isCached);
  },

  /**
   * Returns true if the list exists and has items.
   * @param {Array<Object>} list
   * @return {boolean}
   * @private
   */
  hasSome_: function(list) {
    return !!(list && list.length);
  },

  /**
   * Returns true if the pref has been explicitly disabled.
   * @param {Object} pref
   * @return {boolean}
   * @private
   */
  isDisabled_: function(pref) {
    return !!pref && (pref.value === false);
  },


  /**
   * Listens for the save-address event, and calls the private API.
   * @param {!Event} event
   * @private
   */
  saveAddress_: function(event) {
    this.autofillManager_.saveAddress(event.detail);
  },

  /**
   * Listens for the save-credit-card event, and calls the private API.
   * @param {!Event} event
   * @private
   */
  saveCreditCard_: function(event) {
    this.autofillManager_.saveCreditCard(event.detail);
  },

  /**
   * @private
   * @param {boolean} toggleValue
   * @return {string}
   */
  getOnOffLabel_: function(toggleValue) {
    return toggleValue ? this.i18n('toggleOn') : this.i18n('toggleOff');
  }
});
})();
