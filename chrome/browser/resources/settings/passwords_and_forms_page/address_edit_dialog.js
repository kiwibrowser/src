// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'password-edit-dialog' is the dialog that allows showing a
 * saved password.
 */
(function() {
'use strict';

Polymer({
  is: 'settings-address-edit-dialog',

  behaviors: [
    I18nBehavior,
  ],

  properties: {
    /** @type {chrome.autofillPrivate.AddressEntry} */
    address: Object,

    /** @private */
    title_: String,

    /** @private {!Array<!chrome.autofillPrivate.CountryEntry>} */
    countries_: Array,

    /**
     * Updates the address wrapper.
     * @private {string|undefined}
     */
    countryCode_: {
      type: String,
      observer: 'onUpdateCountryCode_',
    },

    /** @private {!Array<!Array<!settings.address.AddressComponentUI>>} */
    addressWrapper_: Object,

    /** @private */
    phoneNumber_: String,

    /** @private */
    email_: String,

    /** @private */
    canSave_: Boolean,
  },

  /** @override */
  attached: function() {
    this.countryInfo = settings.address.CountryDetailManagerImpl.getInstance();
    this.countryInfo.getCountryList().then(countryList => {
      this.countries_ = countryList;

      this.title_ =
          this.i18n(this.address.guid ? 'editAddressTitle' : 'addAddressTitle');

      // |phoneNumbers| and |emailAddresses| are a single item array.
      // See crbug.com/497934 for details.
      this.phoneNumber_ =
          this.address.phoneNumbers ? this.address.phoneNumbers[0] : '';
      this.email_ =
          this.address.emailAddresses ? this.address.emailAddresses[0] : '';

      this.async(() => {
        if (this.countryCode_ == this.address.countryCode)
          this.updateAddressWrapper_();
        else
          this.countryCode_ = this.address.countryCode;
      });
    });

    // Open is called on the dialog after the address wrapper has been updated.
  },

  /**
   * Returns a class to denote how long this entry is.
   * @param {settings.address.AddressComponentUI} setting
   * @return {string}
   */
  long_: function(setting) {
    return setting.component.isLongField ? 'long' : '';
  },

  /**
   * Updates the wrapper that represents this address in the country's format.
   * @private
   */
  updateAddressWrapper_: function() {
    // Default to the last country used if no country code is provided.
    const countryCode = this.countryCode_ || this.countries_[0].countryCode;
    this.countryInfo.getAddressFormat(countryCode).then(format => {
      this.addressWrapper_ = format.components.map(
          component => component.row.map(
              c => new settings.address.AddressComponentUI(this.address, c)));

      // Flush dom before resize and savability updates.
      Polymer.dom.flush();

      this.updateCanSave_();

      this.fire('on-update-address-wrapper');  // For easier testing.

      const dialog = /** @type {HTMLDialogElement} */ (this.$.dialog);
      if (!dialog.open)
        dialog.showModal();
    });
  },

  updateCanSave_: function() {
    const inputs = this.$.dialog.querySelectorAll('.address-column, select');

    for (let i = 0; i < inputs.length; ++i) {
      if (inputs[i].value) {
        this.canSave_ = true;
        this.fire('on-update-can-save');  // For easier testing.
        return;
      }
    }

    this.canSave_ = false;
    this.fire('on-update-can-save');  // For easier testing.
  },

  /**
   * @param {!chrome.autofillPrivate.CountryEntry} country
   * @return {string}
   * @private
   */
  getCode_: function(country) {
    return country.countryCode || 'SPACER';
  },

  /**
   * @param {!chrome.autofillPrivate.CountryEntry} country
   * @return {string}
   * @private
   */
  getName_: function(country) {
    return country.name || '------';
  },

  /**
   * @param {!chrome.autofillPrivate.CountryEntry} country
   * @return {boolean}
   * @private
   */
  isDivision_: function(country) {
    return !country.countryCode;
  },

  /** @private */
  onCancelTap_: function() {
    this.$.dialog.cancel();
  },

  /**
   * Handler for tapping the save button.
   * @private
   */
  onSaveButtonTap_: function() {
    // The Enter key can call this function even if the button is disabled.
    if (!this.canSave_)
      return;

    // Set a default country if none is set.
    if (!this.address.countryCode)
      this.address.countryCode = this.countries_[0].countryCode;

    this.address.phoneNumbers = this.phoneNumber_ ? [this.phoneNumber_] : [];
    this.address.emailAddresses = this.email_ ? [this.email_] : [];

    this.fire('save-address', this.address);
    this.$.dialog.close();
  },

  /**
   * Syncs the country code back to the address and rebuilds the address wrapper
   * for the new location.
   * @param {string|undefined} countryCode
   * @private
   */
  onUpdateCountryCode_: function(countryCode) {
    this.address.countryCode = countryCode;
    this.updateAddressWrapper_();
  },

  /** @private */
  onCountryChange_: function() {
    const countrySelect = /** @type {!HTMLSelectElement} */ (this.$$('select'));
    this.countryCode_ = countrySelect.value;
  },
});
})();

cr.define('settings.address', function() {
  /**
   * Creates a wrapper against a single data member for an address.
   */
  class AddressComponentUI {
    /**
     * @param {!chrome.autofillPrivate.AddressEntry} address
     * @param {!chrome.autofillPrivate.AddressComponent} component
     */
    constructor(address, component) {
      Object.defineProperty(this, 'value', {
        get: function() {
          return this.getValue_();
        },
        set: function(newValue) {
          this.setValue_(newValue);
        },
      });
      this.address_ = address;
      this.component = component;
      this.isTextArea =
          component.field == chrome.autofillPrivate.AddressField.ADDRESS_LINES;
    }

    /**
     * Gets the value from the address that's associated with this component.
     * @return {string|undefined}
     * @private
     */
    getValue_() {
      const address = this.address_;
      switch (this.component.field) {
        case chrome.autofillPrivate.AddressField.FULL_NAME:
          // |fullNames| is a single item array. See crbug.com/497934 for
          // details.
          return address.fullNames ? address.fullNames[0] : undefined;
        case chrome.autofillPrivate.AddressField.COMPANY_NAME:
          return address.companyName;
        case chrome.autofillPrivate.AddressField.ADDRESS_LINES:
          return address.addressLines;
        case chrome.autofillPrivate.AddressField.ADDRESS_LEVEL_1:
          return address.addressLevel1;
        case chrome.autofillPrivate.AddressField.ADDRESS_LEVEL_2:
          return address.addressLevel2;
        case chrome.autofillPrivate.AddressField.ADDRESS_LEVEL_3:
          return address.addressLevel3;
        case chrome.autofillPrivate.AddressField.POSTAL_CODE:
          return address.postalCode;
        case chrome.autofillPrivate.AddressField.SORTING_CODE:
          return address.sortingCode;
        case chrome.autofillPrivate.AddressField.COUNTRY_CODE:
          return address.countryCode;
        default:
          assertNotReached();
      }
    }

    /**
     * Sets the value in the address that's associated with this component.
     * @param {string} value
     * @private
     */
    setValue_(value) {
      const address = this.address_;
      switch (this.component.field) {
        case chrome.autofillPrivate.AddressField.FULL_NAME:
          address.fullNames = [value];
          break;
        case chrome.autofillPrivate.AddressField.COMPANY_NAME:
          address.companyName = value;
          break;
        case chrome.autofillPrivate.AddressField.ADDRESS_LINES:
          address.addressLines = value;
          break;
        case chrome.autofillPrivate.AddressField.ADDRESS_LEVEL_1:
          address.addressLevel1 = value;
          break;
        case chrome.autofillPrivate.AddressField.ADDRESS_LEVEL_2:
          address.addressLevel2 = value;
          break;
        case chrome.autofillPrivate.AddressField.ADDRESS_LEVEL_3:
          address.addressLevel3 = value;
          break;
        case chrome.autofillPrivate.AddressField.POSTAL_CODE:
          address.postalCode = value;
          break;
        case chrome.autofillPrivate.AddressField.SORTING_CODE:
          address.sortingCode = value;
          break;
        case chrome.autofillPrivate.AddressField.COUNTRY_CODE:
          address.countryCode = value;
          break;
        default:
          assertNotReached();
      }
    }
  }

  /** @interface */
  class CountryDetailManager {
    /**
     * Gets the list of available countries.
     * The default country will be first, followed by a separator, followed by
     * an alphabetized list of countries available.
     * @return {!Promise<!Array<!chrome.autofillPrivate.CountryEntry>>}
     */
    getCountryList() {}

    /**
     * Gets the address format for a given country code.
     * @param {string} countryCode
     * @return {!Promise<!chrome.autofillPrivate.AddressComponents>}
     */
    getAddressFormat(countryCode) {}
  }

  /**
   * Default implementation. Override for testing.
   * @implements {settings.address.CountryDetailManager}
   */
  class CountryDetailManagerImpl {
    /** @override */
    getCountryList() {
      return new Promise(function(callback) {
        chrome.autofillPrivate.getCountryList(callback);
      });
    }

    /** @override */
    getAddressFormat(countryCode) {
      return new Promise(function(callback) {
        chrome.autofillPrivate.getAddressComponents(countryCode, callback);
      });
    }
  }

  cr.addSingletonGetter(CountryDetailManagerImpl);

  return {
    AddressComponentUI: AddressComponentUI,
    CountryDetailManager: CountryDetailManager,
    CountryDetailManagerImpl: CountryDetailManagerImpl,
  };
});
