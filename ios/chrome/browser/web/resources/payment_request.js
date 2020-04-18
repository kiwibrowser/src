// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview JavaScript implementation of the Payment Request API. When
 * loaded, installs the API onto the window object. Conforms
 * to https://w3c.github.io/payment-request/. Note: This is a work in progress.
 */

/**
 * This class implements the DOM level 2 EventTarget interface. The
 * Implementation is copied form src/ui/webui/resources/js/cr/event_target.js.
 * This code should be removed once there is a plan to move event_target.js out
 * of WebUI and reuse in iOS.
 * @constructor
 * @implements {EventTarget}
 */
__gCrWeb.EventTarget =
    function() {
  this.listeners_ = null;
}

// Store EventTarget namespace object in a global __gCrWeb object referenced
// by a string, so it does not get renamed by closure compiler during the
// minification.
__gCrWeb['EventTarget'] = __gCrWeb.EventTarget;

/* Beginning of anonymous object. */
(function() {
  __gCrWeb.EventTarget.prototype = {
    /** @override */
    addEventListener: function(type, handler) {
      if (!this.listeners_)
        this.listeners_ = Object.create(null);
      if (!(type in this.listeners_)) {
        this.listeners_[type] = [handler];
      } else {
        var handlers = this.listeners_[type];
        if (handlers.indexOf(handler) < 0)
          handlers.push(handler);
      }
    },

    /** @override */
    removeEventListener: function(type, handler) {
      if (!this.listeners_)
        return;
      if (type in this.listeners_) {
        var handlers = this.listeners_[type];
        var index = handlers.indexOf(handler);
        if (index >= 0) {
          // Clean up if this was the last listener.
          if (handlers.length == 1)
            delete this.listeners_[type];
          else
            handlers.splice(index, 1);
        }
      }
    },

    /** @override */
    dispatchEvent: function(event) {
      var type = event.type;
      var prevented = 0;

      if (!this.listeners_)
        return true;

      // Override the DOM Event's 'target' property.
      Object.defineProperty(event, 'target', {value: this});

      if (type in this.listeners_) {
        // Clone to prevent removal during dispatch
        var handlers = this.listeners_[type].concat();
        for (var i = 0, handler; handler = handlers[i]; i++) {
          if (handler.handleEvent)
            prevented |= handler.handleEvent.call(handler, event) === false;
          else
            prevented |= handler.call(this, event) === false;
        }
      }

      return !prevented && !event.defaultPrevented;
    }
  };
}());  // End of anonymous object

/**
 * PromiseResolver is a helper class that allows creating a Promise that will be
 * fulfilled (resolved or rejected) some time later. The implementation is
 * copied from src/ui/webui/resources/js/promise_resolver.js. This code should
 * be removed once there is a plan to move promise_resolver.js out of WebUI and
 * reuse in iOS.
 * @constructor
 * @template T
 */
__gCrWeb.PromiseResolver =
    function() {
  /** @private {function(T=): void} */
  this.resolve_;

  /** @private {function(*=): void} */
  this.reject_;

  /** @private {!Promise<T>} */
  this.promise_ = new Promise(function(resolve, reject) {
    this.resolve_ = resolve;
    this.reject_ = reject;
  }.bind(this));
}

// Store PromiseResolver namespace object in a global __gCrWeb object referenced
// by a string, so it does not get renamed by closure compiler during the
// minification.
__gCrWeb['PromiseResolver'] = __gCrWeb.PromiseResolver;

/* Beginning of anonymous object. */
(function() {
  __gCrWeb.PromiseResolver.prototype = {
    /** @return {!Promise<T>} */
    get promise() { return this.promise_; },
    /** @return {function(T=): void} */
    get resolve() { return this.resolve_; },
    /** @return {function(*=): void} */
    get reject() { return this.reject_; },
  };
}());  // End of anonymous object

/**
 * A simple object representation of |PaymentRequest| meant for
 * communication to the app side.
 * @typedef {{
 *   id: string,
 *   methodData: !Array<!window.PaymentMethodData>,
 *   details: !window.PaymentDetails,
 *   options: (window.PaymentOptions|undefined)
 * }}
 */
var SerializedPaymentRequest;

/**
 * A simple object representation of |PaymentResponse| meant for
 * communication to the app side.
 * @typedef {{
 *   paymentRequestId: string,
 *   methodName: string,
 *   details: Object,
 *   shippingAddress: (!window.PaymentAddress|undefined),
 *   shippingOption: (string|undefined),
 *   payerName: (string|undefined),
 *   payerEmail: (string|undefined),
 *   payerPhone: (string|undefined)
 * }}
 */
var SerializedPaymentResponse;

/* Beginning of anonymous object. */
(function() {
  // Namespace for all PaymentRequest implementations. __gCrWeb must have
  // already
  // been defined.
  __gCrWeb.paymentRequestManager = {};

  /** @const {number} */
  __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH = 1024;

  /** @const {number} */
  __gCrWeb['paymentRequestManager'].MAX_JSON_STRING_LENGTH = 1048576;

  /** @const {number} */
  __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE = 1024;

  /** @const {number} */
  __gCrWeb['paymentRequestManager'].SHOW_PROMISE_TIMEOUT_MS =
      10000;  // 10 seconds.

  /**
   * Returns a Promise that either resolves with the result of the passed in
   * Promise or rejects after the given duration.
   * @param {number} duration Duration after which the returned promise rejects.
   * @param {!Promise} promise Promise that must settle in the given duration.
   * @return {!Promise}
   */
  __gCrWeb['paymentRequestManager'].settleOrTimeout = function(
      duration, promise) {
    // Create a Promise that rejects in |duration| milliseconds.
    var timeout = new Promise(function(resolve, reject) {
      window.setTimeout(function() {
        reject('Timed out after ' + duration + 'milliseconds.')
      }, duration);
    });

    // Return a race between the timeout Promise and the passed in Promise.
    return Promise.race([promise, timeout]);
  };

  /**
   * Validates a PaymentCurrencyAmount which is used to supply monetary amounts.
   * https://w3c.github.io/payment-request/#dfn-valid-decimal-monetary-value
   * @param {!window.PaymentCurrencyAmount} amount
   * @param {string} amountName The name of |amount| to use in the error.
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validatePaymentCurrencyAmount = function(
      amount, amountName) {
    // Convert the value to String if it isn't already one.
    amount.value = String(amount.value);
    if (amount.value.length >
        __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH) {
      throw new TypeError(
          amountName + ' value cannot be longer than ' +
          __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH + ' characters');
    }
    if (!/^-?[0-9]+(\.[0-9]+)?$/.test(amount.value)) {
      throw new TypeError(
          amountName + ' value is not a valid decimal monetary value');
    }

    if (typeof amount.currency !== 'string')
      throw new TypeError(amountName + ' currency must be a string');
    if (amount.currency.length >
        __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH) {
      throw new TypeError(
          amountName + ' currency cannot be longer than ' +
          __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH + ' characters');
    }
    if (!/^[a-zA-Z]{3}$/.test(amount.currency)) {
      throw new RangeError(
          amountName + ' currency is not a valid ISO 4217 currency code');
    }
  };

  /**
   * Validates a window.PaymentItem which indicates a monetary amount along with
   * a human-readable description of the amount.
   * @param {!window.PaymentItem} paymentItem
   * @param {string} paymentItemName The name of |paymentItem| to use in the
   *     error.
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validatePaymentItem = function(
      paymentItem, paymentItemName) {
    if (typeof paymentItem.label !== 'string')
      throw new TypeError(paymentItemName + ' label must be a string');
    if (paymentItem.label.length >
        __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH) {
      throw new TypeError(
          paymentItemName + ' label cannot be longer than ' +
          __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH + ' characters');
    }

    if (!paymentItem.amount)
      throw new TypeError(paymentItemName + ' amount is missing');
    __gCrWeb['paymentRequestManager'].validatePaymentCurrencyAmount(
        paymentItem.amount, paymentItemName + ' amount');
  };

  /**
   * Validates a Payment method identifier according to:
   * https://w3c.github.io/payment-method-id/
   * @param {string} identifier Payment method identifier to validate.
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validatePaymentMethodIdentifier = function(
      identifier) {
    if (typeof identifier !== 'string')
      throw new TypeError('A payment method identifier must be a string');
    if (identifier.length >
        __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH) {
      throw new TypeError(
          'A payment method identifier cannot be longer than ' +
          __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH + ' characters');
    }
    try {
      var url = new URL(identifier);
      var invalidIdentifier =
          (url.protocol != 'https:' || url.username || url.password);
    } catch (error) {
      invalidIdentifier =
          !/^[a-z]+[0-9a-z]*(-[a-z]+[0-9a-z]*)*$/.test(identifier);
    } finally {
      if (invalidIdentifier) {
        throw new RangeError(
            'A payment method identifier must either be valid URL with a ' +
            'https scheme and empty username and password or a lower-case ' +
            'alphanumeric string with optional hyphens');
      }
    }
  };

  /**
   * Validates the supportedMethods property and its associated data property in
   * a window.PaymentMethodData or window.PaymentDetailsModifier.
   * @param {(!Array<string>|string)} supportedMethods
   * @param {(window.BasicCardData|Object)} data
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validateSupportedMethods = function(
      supportedMethods, data) {
    var hasBasicCardMethod = false;

    if (supportedMethods instanceof Array) {
      if (supportedMethods.length == 0) {
        throw new TypeError(
            'Each payment method needs to include at least one payment ' +
            'method identifier');
      }
      if (supportedMethods.length >
          __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE) {
        throw new TypeError(
            'At most' + __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE +
            ' payment method identifiers are supported');
      }
      for (var i = 0; i < supportedMethods.length; i++) {
        if (typeof supportedMethods[i] !== 'string') {
          throw new TypeError('A payment method identifier must be a string');
        }
        __gCrWeb['paymentRequestManager'].validatePaymentMethodIdentifier(
            supportedMethods[i]);

        hasBasicCardMethod =
            hasBasicCardMethod || supportedMethods[i] == 'basic-card';
      }
    } else if (typeof supportedMethods !== 'string') {
      throw new TypeError('A payment method identifier must be a string');
    } else {
      __gCrWeb['paymentRequestManager'].validatePaymentMethodIdentifier(
          supportedMethods);
    }

    if (typeof data === 'undefined')
      return;

    if (!(data instanceof Object)) {
      throw new TypeError('Payment method data must be of type \'Object\'');
    }
    if (JSON.stringify(data).length >
        __gCrWeb['paymentRequestManager'].MAX_JSON_STRING_LENGTH) {
      throw new TypeError(
          'JSON serialization of payment method data should be no longer ' +
          'than ' + __gCrWeb['paymentRequestManager'].MAX_JSON_STRING_LENGTH +
          ' characters');
    }

    // Validate basic-card data.
    if (hasBasicCardMethod || supportedMethods == 'basic-card') {
      // Validate basic-card supportedNetworks.
      if (data.supportedNetworks) {
        if (!(data.supportedNetworks instanceof Array)) {
          throw new TypeError('basic-card supportedNetworks must be an array');
        }
        if (data.supportedNetworks.length >
            __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE) {
          throw new TypeError(
              'basic-card supportedNetworks cannot be longer than ' +
              __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE + ' elements');
        }
        for (var i = 0; i < data.supportedNetworks.length; i++) {
          if (typeof data.supportedNetworks[i] !== 'string') {
            throw new TypeError(
                'A basic-card supported network must be a string');
          }
          if (data.supportedNetworks[i].length >
              __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH) {
            throw new TypeError(
                'A basic-card supported network cannot be longer than ' +
                __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH +
                ' characters');
          }
        }
      }
      // Validate basic-card supportedTypes.
      if (data.supportedTypes) {
        if (!(data.supportedTypes instanceof Array)) {
          throw new TypeError('basic-card supportedTypes must be an array');
        }
        if (data.supportedTypes.length >
            __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE) {
          throw new TypeError(
              'basic-card supportedTypes cannot be longer than ' +
              __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE + ' elements');
        }
        for (var i = 0; i < data.supportedTypes.length; i++) {
          if (typeof data.supportedTypes[i] !== 'string') {
            throw new TypeError('A basic-card supported type must be a string');
          }
          if (data.supportedTypes[i].length >
              __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH) {
            throw new TypeError(
                'A basic-card supported type cannot be longer than ' +
                __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH +
                ' characters');
          }
        }
      }
    }
  };

  /**
   * Validates a list of window.PaymentDetailsModifier instances.
   * @param {!Array<!window.PaymentDetailsModifier>} modifiers
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validatePaymentDetailsModifiers = function(
      modifiers) {
    if (!(modifiers instanceof Array))
      throw new TypeError('Modifiers must be an array');
    if (modifiers.length > __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE) {
      throw new TypeError(
          'At most ' + __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE +
          ' modifiers allowed');
    }
    for (var i = 0; i < modifiers.length; i++) {
      if (!modifiers[i].supportedMethods) {
        throw new TypeError(
            'Must specify at least one payment method identifier');
      }
      // Validate PaymentDetailsModifier.supportedMethods and
      // PaymentDetailsModifier.data.
      __gCrWeb['paymentRequestManager'].validateSupportedMethods(
          modifiers[i].supportedMethods, modifiers[i].data);

      if (typeof modifiers[i].total !== 'undefined') {
        __gCrWeb['paymentRequestManager'].validatePaymentItem(
            modifiers[i].total, 'Modifier total');
        if (String(modifiers[i].total.amount.value)[0] == '-')
          throw new TypeError('Modifier total value should be non-negative');
      }

      if (typeof modifiers[i].additionalDisplayItems === 'undefined')
        continue;

      if (!(modifiers[i].additionalDisplayItems instanceof Array)) {
        throw new TypeError('additionalDisplayItems must be an array');
      }
      if (modifiers[i].additionalDisplayItems.length >
          __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE) {
        throw new TypeError(
            'At most ' + __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE +
            ' additionalDisplayItems allowed');
      }
      for (var j = 0; j < modifiers[i].additionalDisplayItems.length; j++) {
        __gCrWeb['paymentRequestManager'].validatePaymentItem(
            modifiers[i].additionalDisplayItems[j],
            'Additional display items in modifier');
      }
    }
  };

  /**
   * Validates the shipping options passed to the PaymentRequest constructor and
   * returns the validate values.
   * @param {(Array<!window.PaymentShippingOption>|undefined)} shippingOptions
   * @param {window.PaymentOptions=} opt_options payment request options.
   * @return {!Array<!window.PaymentShippingOption>} Validated shipping options.
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validateShippingOptions = function(
      shippingOptions, opt_options) {
    if (!shippingOptions || !(shippingOptions instanceof Array)) {
      return [];
    }
    if (shippingOptions.length >
        __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE) {
      throw new TypeError(
          'At most ' + __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE +
          ' shipping options allowed');
    }

    var uniqueIDS = {};
    for (var i = 0; i < shippingOptions.length; i++) {
      // Validate window.PaymentShippingOption.
      if (typeof shippingOptions[i].id !== 'string') {
        throw new TypeError('Shipping option ID must be a string');
      }
      if (shippingOptions[i].id.length >
          __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH) {
        throw new TypeError(
            'Shipping option ID cannot be longer than ' +
            __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH +
            ' characters');
      }
      if (opt_options && opt_options.requestShipping &&
          uniqueIDS[shippingOptions[i].id]) {
        throw new TypeError('Shipping option IDs mus tbe unique');
      }
      uniqueIDS[shippingOptions[i].id] = true;

      if (typeof shippingOptions[i].label !== 'string') {
        throw new TypeError('Shipping option label must be a string');
      }

      if (!shippingOptions[i].amount)
        throw new TypeError('Shipping option amount is missing');
      __gCrWeb['paymentRequestManager'].validatePaymentCurrencyAmount(
          shippingOptions[i].amount, 'Shipping option amount');
    }
    return shippingOptions;
  };

  /**
   * Validates an instance of PaymentDetailsBase and returns the validated
   * shipping options.
   * @param {!window.PaymentDetails} details
   * @param {window.PaymentOptions=} opt_options payment request options.
   * @return {!Array<!window.PaymentShippingOption>} The validated shipping
   *     options.
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validatePaymentDetailsBase = function(
      details, opt_options) {
    // Validate the details.displayItems.
    if (typeof details.displayItems !== 'undefined') {
      if (!(details.displayItems instanceof Array))
        throw new TypeError('display items must be an array');
      if (details.displayItems.length >
          __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE) {
        throw new TypeError(
            'At most ' + __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE +
            'display items allowed');
      }
      for (var i = 0; i < details.displayItems.length; i++) {
        __gCrWeb['paymentRequestManager'].validatePaymentItem(
            details.displayItems[i], 'display items');
      }
    }

    // Validate the details.modifiers.
    if (typeof details.modifiers !== 'undefined') {
      __gCrWeb['paymentRequestManager'].validatePaymentDetailsModifiers(
          details.modifiers);
    }

    // Validate the details.shippingOptions.
    return __gCrWeb['paymentRequestManager'].validateShippingOptions(
        details.shippingOptions, opt_options);
  };

  /**
   * Validates an instance of PaymentDetailsInit and returns the validated
   * shipping options.
   * @param {!window.PaymentDetails} details
   * @param {window.PaymentOptions=} opt_options payment request options.
   * @return {!Array<!window.PaymentShippingOption>} The validated shipping
   *     options.
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validatePaymentDetailsInit = function(
      details, opt_options) {
    // Validate details.total.
    if (!details.total)
      throw new TypeError('Total is missing.');
    __gCrWeb['paymentRequestManager'].validatePaymentItem(
        details.total, 'Total');
    if (String(details.total.amount.value)[0] == '-')
      throw new TypeError('Total value should be non-negative');

    return __gCrWeb['paymentRequestManager'].validatePaymentDetailsBase(
        details, opt_options);
  };

  /**
   * Validates an instance of PaymentDetailsUpdate and returns the validated
   * shipping options.
   * @param {!window.PaymentDetails} details
   * @return {!Array<!window.PaymentShippingOption>} The validated shipping
   *     options.
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validatePaymentDetailsUpdate = function(
      details) {
    // Validate details.total.
    if (details.total) {
      __gCrWeb['paymentRequestManager'].validatePaymentItem(
          details.total, 'Total');
      if (String(details.total.amount.value)[0] == '-')
        throw new TypeError('Total value should be non-negative');
    }

    // Validate details.error.
    if (details.errorMessage) {
      if (typeof details.error !== 'string') {
        throw new TypeError('Error message must be a string');
      }
      if (details.error.length >
          __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH) {
        throw new TypeError(
            'Error message cannot be longer than ' +
            __gCrWeb['paymentRequestManager'].MAX_STRING_LENGTH +
            ' characters');
      }
    }

    return __gCrWeb['paymentRequestManager'].validatePaymentDetailsBase(
        details);
  };

  /**
   * Validates the array of PaymentMethodData instances passed to the
   * PaymentRequest constructor.
   * @param {!Array<!window.PaymentMethodData>} methodData
   * @throws {TypeError|RangeError}
   */
  __gCrWeb['paymentRequestManager'].validatePaymentMethodData = function(
      methodData) {
    if (!(methodData instanceof Array))
      throw new TypeError('methodData must be an array');
    if (methodData.length == 0) {
      throw new TypeError('Needs to include at least one payment method');
    }
    if (methodData.length > __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE) {
      throw new TypeError(
          'At most ' + __gCrWeb['paymentRequestManager'].MAX_LIST_SIZE +
          ' payment methods are supported');
    }
    for (var i = 0; i < methodData.length; i++) {
      if (!methodData[i].supportedMethods) {
        throw new TypeError(
            'Each payment method needs to include at least one payment ' +
            'method identifier');
      }
      // Validate PaymentMethodData.supportedMethods and PaymentMethodData.data.
      __gCrWeb['paymentRequestManager'].validateSupportedMethods(
          methodData[i].supportedMethods, methodData[i].data);
    }
  };

  /**
   * Generates a random string identfier resembling a GUID.
   * @return {string}
   */
  __gCrWeb['paymentRequestManager'].guid = function() {
    /**
     * Generates a stringified random 4 digit hexadecimal number.
     * @return {string}
     */
    var s4 = function() {
      return Math.floor((1 + Math.random()) * 0x10000)
          .toString(16)
          .substring(1);
    };
    return s4() + s4() + '-' + s4() + '-' + s4() + '-' + s4() + '-' + s4() +
        s4() + s4();
  };

  // Store paymentRequestManager namespace object in a global __gCrWeb object
  // referenced by a string, so it does not get renamed by closure compiler
  // during
  // the minification.
  __gCrWeb['paymentRequestManager'] = __gCrWeb.paymentRequestManager;

  /**
   * Wether the origin is secure. The default is true unless it is set to false.
   * If false, PaymentRequest constructor throws a SecurityError DOMException.
   * @type {boolean}
   */
  __gCrWeb['paymentRequestManager'].isContextSecure = true;

  /**
   * The PaymentRequest object, if any. This object is provided by the page and
   * only updated by the app side.
   * @type {PaymentRequest}
   */
  __gCrWeb['paymentRequestManager'].pendingRequest = null;

  /**
   * The PromiseResolver object used to resolve or reject the promise returned
   * by PaymentRequest.prototype.show, if any.
   * @type {__gCrWeb.PromiseResolver}
   */
  __gCrWeb['paymentRequestManager'].requestPromiseResolver = null;

  /**
   * The PromiseResolver object used to resolve or reject the promise returned
   * by PaymentRequest.prototype.abort, if any.
   * @type {__gCrWeb.PromiseResolver}
   */
  __gCrWeb['paymentRequestManager'].abortPromiseResolver = null;

  /**
   * The PromiseResolver object used to resolve the promise returned by
   * PaymentResponse.prototype.complete, if any.
   * @type {PaymentRequest}
   */
  __gCrWeb['paymentRequestManager'].responsePromiseResolver = null;

  /**
   * Parses |paymentResponseData| into a PaymentResponse object.
   * @param {!SerializedPaymentResponse} paymentResponseData
   * @return {PaymentResponse}
   */
  __gCrWeb['paymentRequestManager'].parsePaymentResponseData = function(
      paymentResponseData) {
    var response = new PaymentResponse();
    response.requestId = paymentResponseData['requestId'];
    response.methodName = paymentResponseData['methodName'];
    response.details = paymentResponseData['details'];
    if (paymentResponseData['shippingAddress'])
      response.shippingAddress = paymentResponseData['shippingAddress'];
    if (paymentResponseData['shippingOption'])
      response.shippingOption = paymentResponseData['shippingOption'];
    if (paymentResponseData['payerName'])
      response.payerName = paymentResponseData['payerName'];
    if (paymentResponseData['payerEmail'])
      response.payerEmail = paymentResponseData['payerEmail'];
    if (paymentResponseData['payerPhone'])
      response.payerPhone = paymentResponseData['payerPhone'];

    return response;
  };

  /**
   * The event that enables the web page to update the details of the payment
   * request in response to a user interaction.
   * @type {Event}
   */
  __gCrWeb['paymentRequestManager'].updateEvent = null;


  /**
   * Handles invocation of updateWith() on the updateEvent object.
   * @param {!Promise<!window.PaymentDetails>|!window.PaymentDetails}
   *     detailsOrPromise
   * @throws {DOMException}
   */
  __gCrWeb['paymentRequestManager'].updateEventUpdateWith = function(
      detailsOrPromise) {
    if (!this || this != __gCrWeb['paymentRequestManager'].updateEvent)
      return;

    // if |detailsOrPromise| is not an instance of a Promise, wrap it in a
    // Promise that fulfills with |detailsOrPromise|.
    if (!detailsOrPromise || !(detailsOrPromise.then instanceof Function) ||
        !(detailsOrPromise.catch instanceof Function)) {
      detailsOrPromise = Promise.resolve(detailsOrPromise);
    }

    __gCrWeb['paymentRequestManager']
        .updateWith(detailsOrPromise)
        .then(function() {
          __gCrWeb['paymentRequestManager'].updateEvent = null;
        });
  };

  /**
   * Updates the payment details. Throws an error if |detailsPromise| is not a
   * valid instance of window.PaymentDetails.
   * @param {!Promise<!window.PaymentDetails>} detailsPromise
   * @return {!Promise} A completion Promise that is always resolved whether
   *     detailsPromise is resolved or rejected.
   * @throws {DOMException}
   * @suppress {checkTypes} Required for DOMException's constructor.
   */
  __gCrWeb['paymentRequestManager'].updateWith = function(detailsPromise) {
    if (!__gCrWeb['paymentRequestManager'].pendingRequest ||
        __gCrWeb['paymentRequestManager'].pendingRequest.state !=
            PaymentRequestState.INTERACTIVE) {
      return;
    }

    if (__gCrWeb['paymentRequestManager'].pendingRequest.updating) {
      throw new DOMException(
          'Failed to update PaymentRequest details' +
              ': \'Cannot update details twice',
          'InvalidStateError');
    }

    __gCrWeb['paymentRequestManager'].pendingRequest.updating = true;
    var message = {
      'command': 'paymentRequest.setPendingRequestUpdating',
      'updating': true,
    };
    __gCrWeb.message.invokeOnHost(message);

    return detailsPromise
        .then(function(paymentDetails) {
          if (!paymentDetails)
            throw new TypeError(
                'An instance of PaymentDetails must be provided.');

          // Validate details and update the shipping options.
          var validatedShippingOptions =
              __gCrWeb['paymentRequestManager'].validatePaymentDetailsUpdate(
                  paymentDetails);
          paymentDetails.shippingOptions = validatedShippingOptions;

          var message = {
            'command': 'paymentRequest.updatePaymentDetails',
            'payment_details': paymentDetails,
          };
          __gCrWeb.message.invokeOnHost(message);

          __gCrWeb['paymentRequestManager'].pendingRequest.updating = false;
        })
        .catch(function() {
          var message = {
            'command': 'paymentRequest.requestAbort',
          };
          __gCrWeb.message.invokeOnHost(message);
        });
  };

  /**
   * Resolves the pending PaymentRequest.
   * @param {!SerializedPaymentResponse} paymentResponseData The response to
   *     provide to the resolve function of the Promise.
   */
  __gCrWeb['paymentRequestManager'].resolveRequestPromise = function(
      paymentResponseData) {
    if (!__gCrWeb['paymentRequestManager'].requestPromiseResolver) {
      throw new Error('Internal PaymentRequest error: No Promise to resolve.');
    }

    if (!paymentResponseData) {
      __gCrWeb['paymentRequestManager'].rejectRequestPromise(
          'Internal PaymentRequest error: PaymentResponse missing.');
    }

    var paymentResponse = null;
    try {
      paymentResponse =
          __gCrWeb['paymentRequestManager'].parsePaymentResponseData(
              paymentResponseData);
    } catch (e) {
      __gCrWeb['paymentRequestManager'].rejectRequestPromise(
          'Internal PaymentRequest error: failed to parse PaymentResponse.');
    }

    __gCrWeb['paymentRequestManager'].responsePromiseResolver =
        new __gCrWeb.PromiseResolver();
    __gCrWeb['paymentRequestManager'].requestPromiseResolver.resolve(
        paymentResponse);
    __gCrWeb['paymentRequestManager'].requestPromiseResolver = null;
    __gCrWeb['paymentRequestManager'].pendingRequest = null;
    __gCrWeb['paymentRequestManager'].updateEvent = null;
  };

  /**
   * Rejects the pending PaymentRequest.
   * @param {string} message An error message explaining why the Promise is
   *     being rejected.
   */
  __gCrWeb['paymentRequestManager'].rejectRequestPromise = function(message) {
    if (!__gCrWeb['paymentRequestManager'].requestPromiseResolver) {
      throw new Error(
          'Internal PaymentRequest error: No Promise to reject. ',
          'Message was: ', message);
    }

    __gCrWeb['paymentRequestManager'].requestPromiseResolver.reject(message);
    __gCrWeb['paymentRequestManager'].requestPromiseResolver = null;
    __gCrWeb['paymentRequestManager'].pendingRequest = null;
    __gCrWeb['paymentRequestManager'].updateEvent = null;
  };

  /**
   * Resolves the promise returned by calling PaymentRequest.prototype.abort.
   * This method also gets called when the request is aborted through rejecting
   * the promise passed to PaymentRequestUpdateEvent.prototype.updateWith.
   * Therefore, it does nothing if there is no abort promise to resolve.
   */
  __gCrWeb['paymentRequestManager'].resolveAbortPromise = function() {
    if (!__gCrWeb['paymentRequestManager'].abortPromiseResolver)
      return;

    __gCrWeb['paymentRequestManager'].abortPromiseResolver.resolve();
    __gCrWeb['paymentRequestManager'].abortPromiseResolver = null;
    __gCrWeb['paymentRequestManager'].pendingRequest = null;
    __gCrWeb['paymentRequestManager'].updateEvent = null;
  };

  /**
   * Resolves the promise returned by calling canMakePayment on the current
   * PaymentRequest.
   * @param {boolean} value The response to provide to the resolve function of
   *     the Promise.
   */
  __gCrWeb['paymentRequestManager'].resolveCanMakePaymentPromise = function(
      value) {
    if (!__gCrWeb['paymentRequestManager'].canMakePaymentPromiseResolver) {
      throw new Error('Internal PaymentRequest error: No Promise to resolve.');
    }

    __gCrWeb['paymentRequestManager'].canMakePaymentPromiseResolver.resolve(
        value);
    __gCrWeb['paymentRequestManager'].canMakePaymentPromiseResolver = null;
  };

  /**
   * Rejects the promise returned by calling canMakePayment on the current
   * PaymentRequest.
   * @param {string} message An error message explaining why the Promise is
   *     being rejected.
   */
  __gCrWeb['paymentRequestManager'].rejectCanMakePaymentPromise = function(
      message) {
    if (!__gCrWeb['paymentRequestManager'].canMakePaymentPromiseResolver) {
      throw new Error(
          'Internal PaymentRequest error: No Promise to reject. ',
          'Message was: ', message);
    }

    __gCrWeb['paymentRequestManager'].canMakePaymentPromiseResolver.reject(
        message);
    __gCrWeb['paymentRequestManager'].canMakePaymentPromiseResolver = null;
  };

  /**
   * Serializes |paymentRequest| to a SerializedPaymentRequest object.
   * @param {PaymentRequest} paymentRequest
   * @return {SerializedPaymentRequest}
   */
  __gCrWeb['paymentRequestManager'].serializePaymentRequest = function(
      paymentRequest) {
    var serialized = {
      'id': paymentRequest.id,
      'methodData': paymentRequest.methodData,
      'details': paymentRequest.details,
    };
    if (paymentRequest.options)
      serialized['options'] = paymentRequest.options;
    return serialized;
  };

  /**
   * Resolves the pending PaymentResponse.
   */
  __gCrWeb['paymentRequestManager'].resolveResponsePromise = function() {
    if (!__gCrWeb['paymentRequestManager'].responsePromiseResolver) {
      throw new Error('Internal PaymentRequest error: No Promise to resolve.');
    }

    __gCrWeb['paymentRequestManager'].responsePromiseResolver.resolve();
    __gCrWeb['paymentRequestManager'].responsePromiseResolver = null;
  };

  /**
   * Updates the shipping_option property of the PaymentRequest object to
   * |shippingOptionID| and dispatches a shippingoptionchange event.
   * @param {string} shippingOptionID
   */
  __gCrWeb['paymentRequestManager'].updateShippingOptionAndDispatchEvent =
      function(shippingOptionID) {
    var pendingRequest = __gCrWeb['paymentRequestManager'].pendingRequest;
    if (!pendingRequest ||
        pendingRequest.state != PaymentRequestState.INTERACTIVE ||
        pendingRequest.updating) {
      return;
    }

    pendingRequest.shippingOption = shippingOptionID;

    __gCrWeb['paymentRequestManager'].updateEvent = new Event(
        'shippingoptionchange', {'bubbles': true, 'cancelable': false});

    Object.defineProperty(
        __gCrWeb['paymentRequestManager'].updateEvent, 'updateWith',
        {value: __gCrWeb['paymentRequestManager'].updateEventUpdateWith});

    // setTimeout() is used in order to return immediately. Otherwise the
    // dispatchEvent call waits for all event handlers to return, which could
    // cause a ReentryGuard failure.
    window.setTimeout(function() {
      pendingRequest.dispatchEvent(
          __gCrWeb['paymentRequestManager'].updateEvent);
    }, 0);
  };

  /**
   * Updates the shipping_address property of the PaymentRequest object to
   * |shippingAddress| and dispatches a shippingaddresschange event.
   * @param {!window.PaymentAddress} shippingAddress
   */
  __gCrWeb['paymentRequestManager'].updateShippingAddressAndDispatchEvent =
      function(shippingAddress) {
    var pendingRequest = __gCrWeb['paymentRequestManager'].pendingRequest;
    if (!pendingRequest ||
        pendingRequest.state != PaymentRequestState.INTERACTIVE ||
        pendingRequest.updating) {
      return;
    }

    pendingRequest.shippingAddress = shippingAddress;

    __gCrWeb['paymentRequestManager'].updateEvent = new Event(
        'shippingaddresschange', {'bubbles': true, 'cancelable': false});

    Object.defineProperty(
        __gCrWeb['paymentRequestManager'].updateEvent, 'updateWith',
        {value: __gCrWeb['paymentRequestManager'].updateEventUpdateWith});

    // setTimeout() is used in order to return immediately. Otherwise the
    // dispatchEvent call waits for all event handlers to return, which could
    // cause a ReentryGuard failure.
    window.setTimeout(function() {
      pendingRequest.dispatchEvent(
          __gCrWeb['paymentRequestManager'].updateEvent);
    }, 0);
  };
}());  // End of anonymous object

/**
 * Possible values for the state of the payment request according to:
 * https://w3c.github.io/payment-request/#dfn-x%5B%5Bstate%5D%5D
 * @enum {string}
 */
var PaymentRequestState = {
  CREATED: 'created',
  INTERACTIVE: 'interactive',
  CLOSED: 'closed'
};

/**
 * A request to make a payment.
 * @param {!Array<!window.PaymentMethodData>} methodData Payment method
 *     identifiers for the payment methods that the web site accepts and any
 *     associated payment method specific data.
 * @param {!window.PaymentDetails} details Information about the transaction
 *     that the user is being asked to complete such as the line items in an
 *     order.
 * @param {window.PaymentOptions=} opt_options Information about what
 *     options the web page wishes to use from the payment request system.
 * @constructor
 * @implements {EventTarget}
 * @extends {__gCrWeb.EventTarget}
 * @throws {DOMException|TypeError|RangeError}
 * @suppress {checkTypes} Required for DOMException's constructor.
 */
function PaymentRequest(methodData, details, opt_options) {
  __gCrWeb.EventTarget.call(this);

  if (window.top != window.self) {
    throw new DOMException(
        'Failed to construct \'PaymentRequest\': Must be in top-level context',
        'SecurityError');
  }

  // https://w3c.github.io/webappsec-secure-contexts/#is-origin-trustworthy
  var isOriginPotentiallyTrustworthy = window.location.origin !== 'null' &&
      (window.location.protocol === 'https:' ||
       window.location.hostname === '127.0.0.1' ||
       window.location.hostname === '::1' ||
       window.location.hostname === 'localhost' ||
       window.location.protocol === 'file:');

  if (!__gCrWeb['paymentRequestManager'].isContextSecure ||
      !isOriginPotentiallyTrustworthy) {
    throw new DOMException(
        'Failed to construct \'PaymentRequest\': Must be in a secure context',
        'SecurityError');
  }

  // Initialize these properties to make them own (vs. inherited) properties.
  this.shippingAddress = null;
  this.shippingOption = null;
  this.shippingType = null;

  // Tracks the event handler registered via
  // PaymentRequest.prototype.onshippingaddresschange.
  this.shippingAddressChangeHandler = null;

  Object.defineProperty(this, 'onshippingaddresschange', {
    set(handler) {
      if (this.shippingAddressChangeHandler) {
        this.removeEventListener(
            'shippingaddresschange', this.shippingAddressChangeHandler);
      }
      this.shippingAddressChangeHandler = handler;
      this.addEventListener('shippingaddresschange', handler);
    },
    configurable: true
  });

  // Tracks the event handler registered via
  // PaymentRequest.prototype.onshippingoptionchange.
  this.shippingOptionChangeHandler = null;

  Object.defineProperty(this, 'onshippingoptionchange', {
    set(handler) {
      if (this.shippingOptionChangeHandler) {
        this.removeEventListener(
            'shippingoptionchange', this.shippingOptionChangeHandler);
      }
      this.shippingOptionChangeHandler = handler;
      this.addEventListener('shippingoptionchange', handler);
    },
    configurable: true
  });

  /**
   * The state of this request, used to govern its lifecycle.
   * @type {PaymentRequestState}
   * @private
   */
  this.state = PaymentRequestState.CREATED;

  /**
   * True if there is a pending updateWith call to update the payment request
   * and false otherwise.
   * @type {boolean}
   * @private
   */
  this.updating = false;

  // Validate methodData.
  __gCrWeb['paymentRequestManager'].validatePaymentMethodData(methodData);

  /**
   * @type {!Array<!window.PaymentMethodData>}
   * @private
   */
  this.methodData = methodData;

  // Validate details and update the shipping options.
  var validatedShippingOptions =
      __gCrWeb['paymentRequestManager'].validatePaymentDetailsInit(
          details, opt_options);
  details.shippingOptions = validatedShippingOptions;

  this.id = details.id ? details.id : __gCrWeb['paymentRequestManager'].guid();

  /**
   * @type {!window.PaymentDetails}
   * @private
   */
  this.details = details;

  // Pick the last selected shipping option as the selected shipping option.
  if (opt_options && opt_options.requestShipping) {
    for (var i = 0; i < validatedShippingOptions.length; i++) {
      if (validatedShippingOptions[i].selected)
        this.shippingOption = validatedShippingOptions[i].id;
    }
  }

  // Validate the opt_options.shippingType.
  if (opt_options && opt_options.shippingType &&
      opt_options.shippingType != PaymentShippingType.SHIPPING &&
      opt_options.shippingType != PaymentShippingType.DELIVERY &&
      opt_options.shippingType != PaymentShippingType.PICKUP) {
    throw new TypeError(
        'The provided shipping type is not a valid enum value of type ' +
        'PaymentShippingType');
  }

  // Use the provided shipping type. Otherwise default to "shipping".
  if (opt_options && opt_options.requestShipping) {
    this.shippingType = opt_options.shippingType ? opt_options.shippingType :
                                                   PaymentShippingType.SHIPPING;
  }

  /**
   * @type {?window.PaymentOptions}
   * @private
   */
  this.options = opt_options || null;

  var message = {
    'command': 'paymentRequest.createPaymentRequest',
    'payment_request':
        __gCrWeb['paymentRequestManager'].serializePaymentRequest(this),
  };
  __gCrWeb.message.invokeOnHost(message);
};

PaymentRequest.prototype.__proto__ = __gCrWeb.EventTarget.prototype;

/**
 * A provided or generated ID for the this Payment Request instance.
 * @type {string}
 */
PaymentRequest.prototype.id = '';

/**
 * Shipping address selected by the user.
 * @type {?window.PaymentAddress}
 */
PaymentRequest.prototype.shippingAddress = null;

/**
 * Shipping option selected by the user.
 * @type {?string}
 */
PaymentRequest.prototype.shippingOption = null;

/**
 * Set to the value of shippingType property of |opt_options| if it is a valid
 * PaymentShippingType. Otherwise set to PaymentShippingType.SHIPPING.
 * @type {?PaymentShippingType}
 */
PaymentRequest.prototype.shippingType = null;

/**
 * Presents the PaymentRequest UI to the user. If |opt_detailsPromise| is a
 * valid Promise, disables the UI and waits until the Promise settles. If the
 * Promise is resolved with valid payment details, enables the UI with the new
 * details. Otherwise, the request is aborted.
 * @param {Promise<!window.PaymentDetails>=} opt_detailsPromise
 * @return {!Promise<PaymentResponse>} A promise to notify the caller
 *     whether the user accepted or rejected the request.
 * @suppress {checkTypes} Required for DOMException's constructor.
 */
PaymentRequest.prototype.show = function(opt_detailsPromise) {
  if (!(this instanceof PaymentRequest)) {
    return Promise.reject(new DOMException(
        'show() must be called on an instance of PaymentRequest.',
        'InvalidStateError'));
  }

  if (this.state != PaymentRequestState.CREATED) {
    return Promise.reject(
        new DOMException('Already called show() once', 'InvalidStateError'));
  }

  if (__gCrWeb['paymentRequestManager'].pendingRequest ||
      __gCrWeb['paymentRequestManager'].requestPromiseResolver) {
    return Promise.reject(new DOMException(
        'Only one PaymentRequest may be shown at a time', 'AbortError'));
  }

  __gCrWeb['paymentRequestManager'].pendingRequest = this;
  __gCrWeb['paymentRequestManager'].requestPromiseResolver =
      new __gCrWeb.PromiseResolver();
  this.state = PaymentRequestState.INTERACTIVE;

  var waitForShowPromise = false;

  // if |opt_detailsPromise| is an instance of Promise, the UI should be
  // disabled until the Promise is settled.
  if (opt_detailsPromise && opt_detailsPromise.then instanceof Function &&
      opt_detailsPromise.catch instanceof Function) {
    waitForShowPromise = true;
  }

  var message = {
    'command': 'paymentRequest.requestShow',
    'payment_request':
        __gCrWeb['paymentRequestManager'].serializePaymentRequest(this),
    'waitForShowPromise': waitForShowPromise,
  };
  __gCrWeb.message.invokeOnHost(message);

  // if |opt_detailsPromise| is an instance of Promise, update the payment
  // details with it. The UI will be re-enabled with the new details or the flow
  // will be aborted depending on how this Promise settles.
  if (waitForShowPromise) {
    __gCrWeb['paymentRequestManager'].updateWith(
        __gCrWeb['paymentRequestManager'].settleOrTimeout(
            __gCrWeb['paymentRequestManager'].SHOW_PROMISE_TIMEOUT_MS,
            opt_detailsPromise));
  }

  return __gCrWeb['paymentRequestManager'].requestPromiseResolver.promise;
};

/**
 * May be called if the web page wishes to tell the user agent to abort the
 * payment request and to tear down any user interface that might be shown.
 * @return {!Promise<undefined>} A promise to notify the caller whether the user
 *     agent was able to abort the payment request.
 * @suppress {checkTypes} Required for DOMException's constructor.
 */
PaymentRequest.prototype.abort = function() {
  if (!(this instanceof PaymentRequest)) {
    return Promise.reject(new DOMException(
        'abort() must be called on an instance of PaymentRequest.',
        'InvalidStateError'));
  }

  if (!__gCrWeb['paymentRequestManager'].pendingRequest ||
      !__gCrWeb['paymentRequestManager'].requestPromiseResolver) {
    return Promise.reject(new DOMException(
        'Never called show(), so nothing to abort', 'InvalidStateError'));
  }

  if (__gCrWeb['paymentRequestManager'].abortPromiseResolver) {
    return Promise.reject(new DOMException(
        'Cannot abort() again until the previous abort() has resolved or ' +
        'rejected'));
  }

  __gCrWeb['paymentRequestManager'].abortPromiseResolver =
      new __gCrWeb.PromiseResolver();
  this.state = PaymentRequestState.CLOSED;

  var message = {
    'command': 'paymentRequest.requestAbort',
  };
  __gCrWeb.message.invokeOnHost(message);

  return __gCrWeb['paymentRequestManager'].abortPromiseResolver.promise;
};

/**
 * May be called before calling show() to determine if the PaymentRequest object
 * can be used to make a payment.
 * @return {!Promise<boolean>} A promise to notify the caller whether the
 *     PaymentRequest object can be used to make a payment.
 * @suppress {checkTypes} Required for DOMException's constructor.
 */
PaymentRequest.prototype.canMakePayment = function() {
  if (!(this instanceof PaymentRequest)) {
    return Promise.reject(new DOMException(
        'canMakePayment() must be called on an instance of PaymentRequest.',
        'InvalidStateError'));
  }

  if (this.state != PaymentRequestState.CREATED) {
    return Promise.reject(
        new DOMException('Cannot query payment request', 'InvalidStateError'));
  }

  var message = {
    'command': 'paymentRequest.requestCanMakePayment',
    'payment_request':
        __gCrWeb['paymentRequestManager'].serializePaymentRequest(this),
  };
  __gCrWeb.message.invokeOnHost(message);

  __gCrWeb['paymentRequestManager'].canMakePaymentPromiseResolver =
      new __gCrWeb.PromiseResolver();
  return __gCrWeb['paymentRequestManager'].canMakePaymentPromiseResolver
      .promise;
};

/**
 * @typedef {{
 *   supportedNetworks: !Array<string>,
 *   supportedTypes: !Array<string>
 * }}
 */
window.BasicCardData;

/**
 * @typedef {{
 *   supportedMethods: (!Array<string>|string),
 *   data: (!window.BasicCardData|Object)
 * }}
 */
window.PaymentMethodData;

/**
 * @typedef {{
 *   currency: string,
 *   value: string,
 * }}
 */
window.PaymentCurrencyAmount;

/**
 * @typedef {{
 *   id: (string|undefined),
 *   total: (window.PaymentItem|undefined),
 *   displayItems: (!Array<!window.PaymentItem>|undefined),
 *   shippingOptions: (!Array<!window.PaymentShippingOption>|undefined),
 *   modifiers: (!Array<!window.PaymentDetailsModifier>|undefined),
 *   error: (string|undefined)
 * }}
 */
window.PaymentDetails;

/**
 * @typedef {{
 *   supportedMethods: (!Array<string>|string),
 *   total: (window.PaymentItem|undefined),
 *   additionalDisplayItems: (!Array<!window.PaymentItem>|undefined),
 *   data: (!window.BasicCardData|Object)
 * }}
 */
window.PaymentDetailsModifier;

/**
 * Contains the possible values for affecting the payment request user interface
 * for gathering the shipping address if window.PaymentOptions.requestShipping
 * is set to true.
 * @enum {string}
 */
var PaymentShippingType = {
  SHIPPING: 'shipping',
  DELIVERY: 'delivery',
  PICKUP: 'pickup'
};

/**
 * @typedef {{
 *   requestPayerName: (boolean|undefined),
 *   requestPayerEmail: (boolean|undefined),
 *   requestPayerPhone: (boolean|undefined),
 *   requestShipping: (boolean|undefined),
 *   shippingType: (!PaymentShippingType|undefined)
 * }}
 */
window.PaymentOptions;

/**
 * @typedef {{
 *   label: string,
 *   amount: window.PaymentCurrencyAmount
 * }}
 */
window.PaymentItem;

/**
 * @typedef {{
 *   country: string,
 *   addressLine: !Array<string>,
 *   region: string,
 *   city: string,
 *   dependentLocality: string,
 *   postalCode: string,
 *   sortingCode: string,
 *   languageCode: string,
 *   organization: string,
 *   recipient: string,
 *   phone: string
 * }}
 */
window.PaymentAddress;

/**
 * @typedef {{
 *   id: string,
 *   label: string,
 *   amount: window.PaymentCurrencyAmount,
 *   selected: boolean
 * }}
 */
window.PaymentShippingOption;

/**
 * A response to a request to make a payment. Represents the choices made by the
 * user and provided to the web page through the resolve function of the Promise
 * returned by PaymentRequest.show().
 * https://w3c.github.io/browser-payment-api/#paymentresponse-interface
 * @constructor
 * @private
 */
function PaymentResponse(){
  // Initialize these properties to make them own (vs. inherited) properties.
  this.requestId = '';
  this.methodName = '';
  this.details = {};
  this.shippingAddress = null;
  this.shippingOption = null;
  this.payerName = null;
  this.payerEmail = null;
  this.payerPhone = null;
};

/**
 * The same identifier present in the original PaymentRequest.
 * @type {string}
 */
PaymentResponse.prototype.requestId = '';

/**
 * The payment method identifier for the payment method that the user selected
 * to fulfil the transaction.
 * @type {string}
 */
PaymentResponse.prototype.methodName = '';

/**
 * An object that provides a payment method specific message used by the
 * merchant to process the transaction and determine successful fund transfer.
 * the payment request.
 * @type {Object}
 */
PaymentResponse.prototype.details = {};

/**
 * If the requestShipping flag was set to true in the window.PaymentOptions
 * passed to the PaymentRequest constructor, this will be the full and
 * final shipping address chosen by the user.
 * @type {?window.PaymentAddress}
 */
PaymentResponse.prototype.shippingAddress = null;

/**
 * If the requestShipping flag was set to true in the window.PaymentOptions
 * passed to the PaymentRequest constructor, this will be the id
 * attribute of the selected shipping option.
 * @type {?string}
 */
PaymentResponse.prototype.shippingOption = null;

/**
 * If the requestPayerName flag was set to true in the window.PaymentOptions
 * passed to the PaymentRequest constructor, this will be the name
 * provided by the user.
 * @type {?string}
 */
PaymentResponse.prototype.payerName = null;

/**
 * If the requestPayerEmail flag was set to true in the window.PaymentOptions
 * passed to the PaymentRequest constructor, this will be the email
 * address chosen by the user.
 * @type {?string}
 */
PaymentResponse.prototype.payerEmail = null;

/**
 * If the requestPayerPhone flag was set to true in the window.PaymentOptions
 * passed to the PaymentRequest constructor, this will be the phone
 * number chosen by the user.
 * @type {?string}
 */
PaymentResponse.prototype.payerPhone = null;

/**
 * Contains the possible values for the string argument accepted by
 * PaymentResponse.prototype.complete.
 * @enum {string}
 */
var PaymentComplete = {
  FAIL: 'fail',
  SUCCESS: 'success',
  UNKNOWN: 'unknown'
};

/**
 * Communicates the result of processing the payment.
 * @param {PaymentComplete=} opt_result Indicates whether payment was
 *     successfully processed.
 * @return {!Promise} A promise to notify the caller when the user interface has
 *     been closed.
 */
PaymentResponse.prototype.complete = function(opt_result) {
  if (opt_result != PaymentComplete.UNKNOWN &&
      opt_result != PaymentComplete.SUCCESS &&
      opt_result != PaymentComplete.FAIL) {
    opt_result = PaymentComplete.UNKNOWN;
  }

  if (!__gCrWeb['paymentRequestManager'].responsePromiseResolver) {
    throw new Error('Internal PaymentRequest error: No Promise to return.');
  }

  var message = {
    'command': 'paymentRequest.responseComplete',
    'result': opt_result,
  };
  __gCrWeb.message.invokeOnHost(message);

  return __gCrWeb['paymentRequestManager'].responsePromiseResolver.promise;
};
