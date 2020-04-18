// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Dialog used for pairing a provided |pairing-device|. Set |show-error| to
 * show the error results from a pairing event instead of the pairing UI.
 * NOTE: This module depends on I18nBehavior which depends on loadTimeData.
 */

var PairingEventType = chrome.bluetoothPrivate.PairingEventType;

Polymer({
  is: 'bluetooth-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * Interface for bluetooth calls. Set in bluetooth-page.
     * @type {Bluetooth}
     * @private
     */
    bluetooth: {
      type: Object,
      value: chrome.bluetooth,
    },

    /**
     * Interface for bluetoothPrivate calls.
     * @type {BluetoothPrivate}
     */
    bluetoothPrivate: {
      type: Object,
      value: chrome.bluetoothPrivate,
    },

    noCancel: Boolean,

    dialogTitle: String,

    /**
     * Current Pairing device.
     * @type {!chrome.bluetooth.Device|undefined}
     */
    pairingDevice: Object,

    /**
     * Current Pairing event.
     * @private {?chrome.bluetoothPrivate.PairingEvent}
     */
    pairingEvent_: {
      type: Object,
      value: null,
    },

    /**
     * May be set by the host to show a pairing error result, or may be
     * set by the dialog if a pairing or connect error occured.
     * @private
     */
    errorMessage_: String,

    /**
     * Pincode or passkey value, used to trigger connect enabled changes.
     * @private
     */
    pinOrPass_: String,

    /**
     * @const {!Array<number>}
     * @private
     */
    digits_: {
      type: Array,
      readOnly: true,
      value: [0, 1, 2, 3, 4, 5],
    },
  },

  observers: [
    'dialogUpdated_(errorMessage_, pairingEvent_)',
    'pairingChanged_(pairingDevice, pairingEvent_)',
  ],

  /**
   * Listener for chrome.bluetoothPrivate.onPairing events.
   * @private {?function(!chrome.bluetoothPrivate.PairingEvent)}
   */
  bluetoothPrivateOnPairingListener_: null,

  /**
   * Listener for chrome.bluetooth.onBluetoothDeviceChanged events.
   * @private {?function(!chrome.bluetooth.Device)}
   */
  bluetoothDeviceChangedListener_: null,

  open: function() {
    this.startPairing();
    this.pinOrPass_ = '';
    this.getDialog_().showModal();
    this.itemWasFocused_ = false;
  },

  close: function() {
    this.endPairing();
    var dialog = this.getDialog_();
    if (dialog.open)
      dialog.close();
  },

  /**
   * Updates the dialog after a connect attempt.
   * @param {!chrome.bluetooth.Device} device The device connected to
   * @param {!{message: string}} lastError chrome.runtime.lastError
   * @param {chrome.bluetoothPrivate.ConnectResultType} result The connect
   *     result
   * @return {boolean}
   */
  handleError: function(device, lastError, result) {
    var error;
    if (lastError) {
      error = lastError.message;
    } else {
      switch (result) {
        case chrome.bluetoothPrivate.ConnectResultType.IN_PROGRESS:
        case chrome.bluetoothPrivate.ConnectResultType.ALREADY_CONNECTED:
        case chrome.bluetoothPrivate.ConnectResultType.AUTH_CANCELED:
        case chrome.bluetoothPrivate.ConnectResultType.SUCCESS:
          this.errorMessage_ = '';
          return false;
        default:
          error = result;
      }
    }

    var name = device.name || device.address;
    var id = 'bluetooth_connect_' + error;
    if (this.i18nExists(id)) {
      this.errorMessage_ = this.i18n(id, name);
    } else {
      this.errorMessage_ = error;
      console.error('Unexpected error connecting to: ' + name + ': ' + error);
    }
    return true;
  },

  /** @private */
  dialogUpdated_: function() {
    if (this.showEnterPincode_())
      this.$$('#pincode').focus();
    else if (this.showEnterPasskey_())
      this.$$('#passkey').focus();
  },

  /**
   * @return {!CrDialogElement}
   * @private
   */
  getDialog_: function() {
    return /** @type {!CrDialogElement} */ (this.$.dialog);
  },

  /** @private */
  onCancelTap_: function() {
    this.getDialog_().cancel();
  },

  /** @private */
  onDialogCanceled_: function() {
    if (!this.errorMessage_)
      this.sendResponse_(chrome.bluetoothPrivate.PairingResponse.CANCEL);
    this.endPairing();
  },

  /** Called when the dialog is opened. Starts listening for pairing events. */
  startPairing: function() {
    if (!this.bluetoothPrivateOnPairingListener_) {
      this.bluetoothPrivateOnPairingListener_ =
          this.onBluetoothPrivateOnPairing_.bind(this);
      this.bluetoothPrivate.onPairing.addListener(
          this.bluetoothPrivateOnPairingListener_);
    }
    if (!this.bluetoothDeviceChangedListener_) {
      this.bluetoothDeviceChangedListener_ =
          this.onBluetoothDeviceChanged_.bind(this);
      this.bluetooth.onDeviceChanged.addListener(
          this.bluetoothDeviceChangedListener_);
    }
  },

  /** Called when the dialog is closed. */
  endPairing: function() {
    if (this.bluetoothPrivateOnPairingListener_) {
      this.bluetoothPrivate.onPairing.removeListener(
          this.bluetoothPrivateOnPairingListener_);
      this.bluetoothPrivateOnPairingListener_ = null;
    }
    if (this.bluetoothDeviceChangedListener_) {
      this.bluetooth.onDeviceChanged.removeListener(
          this.bluetoothDeviceChangedListener_);
      this.bluetoothDeviceChangedListener_ = null;
    }
    this.pairingEvent_ = null;
  },

  /**
   * Process bluetoothPrivate.onPairing events.
   * @param {!chrome.bluetoothPrivate.PairingEvent} event
   * @private
   */
  onBluetoothPrivateOnPairing_: function(event) {
    if (!this.pairingDevice ||
        event.device.address != this.pairingDevice.address) {
      return;
    }
    if (event.pairing == PairingEventType.KEYS_ENTERED &&
        event.passkey === undefined && this.pairingEvent_) {
      // 'keysEntered' event might not include the updated passkey so preserve
      // the current one.
      event.passkey = this.pairingEvent_.passkey;
    }
    this.pairingEvent_ = event;
  },

  /**
   * Process bluetooth.onDeviceChanged events. This ensures that the dialog
   * updates when the connection state changes.
   * @param {!chrome.bluetooth.Device} device
   * @private
   */
  onBluetoothDeviceChanged_: function(device) {
    if (!this.pairingDevice || device.address != this.pairingDevice.address)
      return;
    this.pairingDevice = device;
  },

  /** @private */
  pairingChanged_: function() {
    // Auto-close the dialog when pairing completes.
    if (this.pairingDevice.paired && !this.pairingDevice.connecting &&
        this.pairingDevice.connected) {
      this.close();
      return;
    }
    this.errorMessage_ = '';
    this.pinOrPass_ = '';
  },

  /**
   * @return {string}
   * @private
   */
  getMessage_: function() {
    var message;
    if (!this.pairingEvent_)
      message = 'bluetoothStartConnecting';
    else
      message = this.getEventDesc_(this.pairingEvent_.pairing);
    return this.i18n(message, this.pairingDevice.name);
  },

  /**
   * @return {boolean}
   * @private
   */
  showEnterPincode_: function() {
    return !!this.pairingEvent_ &&
        this.pairingEvent_.pairing == PairingEventType.REQUEST_PINCODE;
  },

  /**
   * @return {boolean}
   * @private
   */
  showEnterPasskey_: function() {
    return !!this.pairingEvent_ &&
        this.pairingEvent_.pairing == PairingEventType.REQUEST_PASSKEY;
  },

  /**
   * @return {boolean}
   * @private
   */
  showDisplayPassOrPin_: function() {
    if (!this.pairingEvent_)
      return false;
    var pairing = this.pairingEvent_.pairing;
    return (
        pairing == PairingEventType.DISPLAY_PINCODE ||
        pairing == PairingEventType.DISPLAY_PASSKEY ||
        pairing == PairingEventType.CONFIRM_PASSKEY ||
        pairing == PairingEventType.KEYS_ENTERED);
  },

  /**
   * @return {boolean}
   * @private
   */
  showAcceptReject_: function() {
    return !!this.pairingEvent_ &&
        this.pairingEvent_.pairing == PairingEventType.CONFIRM_PASSKEY;
  },

  /**
   * @return {boolean}
   * @private
   */
  showConnect_: function() {
    if (!this.pairingEvent_)
      return false;
    var pairing = this.pairingEvent_.pairing;
    return pairing == PairingEventType.REQUEST_PINCODE ||
        pairing == PairingEventType.REQUEST_PASSKEY;
  },

  /**
   * @return {boolean}
   * @private
   */
  enableConnect_: function() {
    if (!this.showConnect_())
      return false;
    var inputId =
        (this.pairingEvent_.pairing == PairingEventType.REQUEST_PINCODE) ?
        '#pincode' :
        '#passkey';
    var paperInput = /** @type {!PaperInputElement} */ (this.$$(inputId));
    assert(paperInput);
    /** @type {string} */ var value = paperInput.value;
    return !!value && paperInput.validate();
  },

  /**
   * @return {boolean}
   * @private
   */
  showDismiss_: function() {
    return this.pairingDevice.paired ||
        (!!this.pairingEvent_ &&
         this.pairingEvent_.pairing == PairingEventType.COMPLETE);
  },

  /** @private */
  onAcceptTap_: function() {
    this.sendResponse_(chrome.bluetoothPrivate.PairingResponse.CONFIRM);
  },

  /** @private */
  onConnectTap_: function() {
    this.sendResponse_(chrome.bluetoothPrivate.PairingResponse.CONFIRM);
  },

  /** @private */
  onRejectTap_: function() {
    this.sendResponse_(chrome.bluetoothPrivate.PairingResponse.REJECT);
  },

  /**
   * @param {!chrome.bluetoothPrivate.PairingResponse} response
   * @private
   */
  sendResponse_: function(response) {
    if (!this.pairingDevice)
      return;
    var options =
        /** @type {!chrome.bluetoothPrivate.SetPairingResponseOptions} */ (
            {device: this.pairingDevice, response: response});
    if (response == chrome.bluetoothPrivate.PairingResponse.CONFIRM) {
      var pairing = this.pairingEvent_.pairing;
      if (pairing == PairingEventType.REQUEST_PINCODE)
        options.pincode = this.$$('#pincode').value;
      else if (pairing == PairingEventType.REQUEST_PASSKEY)
        options.passkey = parseInt(this.$$('#passkey').value, 10);
    }
    this.bluetoothPrivate.setPairingResponse(options, () => {
      if (chrome.runtime.lastError) {
        // TODO(stevenjb): Show error.
        console.error(
            'Error setting pairing response: ' + options.device.name +
            ': Response: ' + options.response +
            ': Error: ' + chrome.runtime.lastError.message);
      }
      this.close();
    });

    this.fire('response', options);
  },

  /**
   * @param {!PairingEventType} eventType
   * @return {string}
   * @private
   */
  getEventDesc_: function(eventType) {
    assert(eventType);
    if (eventType == PairingEventType.COMPLETE ||
        eventType == PairingEventType.KEYS_ENTERED ||
        eventType == PairingEventType.REQUEST_AUTHORIZATION) {
      return 'bluetoothStartConnecting';
    }
    return 'bluetooth_' + /** @type {string} */ (eventType);
  },

  /**
   * @param {number} index
   * @return {string}
   * @private
   */
  getPinDigit_: function(index) {
    if (!this.pairingEvent_)
      return '';
    var digit = '0';
    var pairing = this.pairingEvent_.pairing;
    if (pairing == PairingEventType.DISPLAY_PINCODE &&
        this.pairingEvent_.pincode &&
        index < this.pairingEvent_.pincode.length) {
      digit = this.pairingEvent_.pincode[index];
    } else if (
        this.pairingEvent_.passkey &&
        (pairing == PairingEventType.DISPLAY_PASSKEY ||
         pairing == PairingEventType.KEYS_ENTERED ||
         pairing == PairingEventType.CONFIRM_PASSKEY)) {
      var passkeyString =
          String(this.pairingEvent_.passkey).padStart(this.digits_.length, '0');
      digit = passkeyString[index];
    }
    return digit;
  },

  /**
   * @param {number} index
   * @return {string}
   * @private
   */
  getPinClass_: function(index) {
    if (!this.pairingEvent_)
      return '';
    if (this.pairingEvent_.pairing == PairingEventType.CONFIRM_PASSKEY)
      return 'confirm';
    var cssClass = 'display';
    if (this.pairingEvent_.pairing == PairingEventType.DISPLAY_PASSKEY) {
      if (index == 0)
        cssClass += ' next';
      else
        cssClass += ' untyped';
    } else if (
        this.pairingEvent_.pairing == PairingEventType.KEYS_ENTERED &&
        this.pairingEvent_.enteredKey) {
      var enteredKey = this.pairingEvent_.enteredKey;  // 1-7
      var lastKey = this.digits_.length;               // 6
      if ((index == -1 && enteredKey > lastKey) || (index + 1 == enteredKey))
        cssClass += ' next';
      else if (index > enteredKey)
        cssClass += ' untyped';
    }
    return cssClass;
  },
});
