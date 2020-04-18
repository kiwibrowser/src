/*
 * Copyright 2016 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * Calls PaymentRequest.canMakePayment() and prints out the result.
 * @param {sequence<PaymentMethodData>} methodData The supported methods.
 * @private
 */
function canMakePaymentHelper(methodData) {
  try {
    new PaymentRequest(methodData, {
      total: {
        label: 'Total',
        amount: {
          currency: 'USD',
          value: '5.00',
        },
      },
    })
        .canMakePayment()
        .then(function(result) {
          print(result);
        })
        .catch(function(error) {
          print(error);
        });
  } catch (error) {
    print(error);
  }
}

/**
 * Merchant checks for ability to pay using "basic-card" regardless of issuer
 * network.
 */
function checkBasicCard() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([{
    supportedMethods: 'basic-card',
  }]);
}

/**
 * Merchant checks for ability to pay using debit cards.
 */
function checkBasicDebit() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([{
    supportedMethods: 'basic-card',
    data: {
      supportedTypes: ['debit'],
    },
  }]);
}

/**
 * Merchant checks for ability to pay using "basic-card" with "mastercard" as
 * the supported network.
 */
function checkBasicMasterCard() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([{
    supportedMethods: 'basic-card',
    data: {
      supportedNetworks: ['mastercard'],
    },
  }]);
}

/**
 * Merchant checks for ability to pay using "basic-card" with "visa" as the
 * supported network.
 */
function checkBasicVisa() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([{
    supportedMethods: 'basic-card',
    data: {
      supportedNetworks: ['visa'],
    },
  }]);
}

/**
 * Merchant checks for ability to pay using "mastercard".
 */
function checkMasterCard() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([{
    supportedMethods: 'mastercard',
  }]);
}

/**
 * Merchant checks for ability to pay using "visa".
 */
function checkVisa() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([{
    supportedMethods: 'visa',
  }]);
}

/**
 * Merchant checks for ability to pay using "https://alicepay.com/webpay".
 */
function checkAlicePay() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([{
    supportedMethods: 'https://alicepay.com/webpay',
  }]);
}

/**
 * Merchant checks for ability to pay using "https://bobpay.com/webpay".
 */
function checkBobPay() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([{
    supportedMethods: 'https://bobpay.com/webpay',
  }]);
}

/**
 * Merchant checks for ability to pay using "https://bobpay.com/webpay" or
 * "basic-card".
 */
function checkBobPayAndBasicCard() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([
    {
      supportedMethods: 'https://bobpay.com/webpay',
    },
    {
      supportedMethods: 'basic-card',
    },
  ]);
}

/**
 * Merchant checks for ability to pay using "https://bobpay.com/webpay" or
 * "visa".
 */
function checkBobPayAndVisa() {  // eslint-disable-line no-unused-vars
  canMakePaymentHelper([
    {
      supportedMethods: 'https://bobpay.com/webpay',
    },
    {
      supportedMethods: 'visa',
    },
  ]);
}

/**
 * Calls PaymentRequest.show() and prints out the result.
 * @param {sequence<PaymentMethodData>} methodData The supported methods.
 * @private
 */
function buyHelper(methodData) {
  try {
    new PaymentRequest(methodData, {
      total: {
        label: 'Total',
        amount: {
          currency: 'USD',
          value: '5.00',
        },
      },
    })
        .show()
        .then(function(response) {
          response.complete('success')
              .then(function() {
                print(JSON.stringify(response, undefined, 2));
              })
              .catch(function(error) {
                print(error);
              });
        })
        .catch(function(error) {
          print(error);
        });
  } catch (error) {
    print(error);
  }
}

/**
 * Merchant requests payment via either "mastercard" or "basic-card" with "visa"
 * as the supported network.
 */
function buy() {  // eslint-disable-line no-unused-vars
  buyHelper([
    {
      supportedMethods: 'mastercard',
    },
    {
      supportedMethods: 'basic-card',
      data: {
        supportedNetworks: ['visa'],
      },
    },
  ]);
}

/**
 * Merchant requests payment via "basic-card" with any issuer network.
 */
function buyBasicCard() {  // eslint-disable-line no-unused-vars
  buyHelper([{
    supportedMethods: 'basic-card',
  }]);
}

/**
 * Merchant requests payment via "basic-card" with "debit" as the supported card
 * type.
 */
function buyBasicDebit() {  // eslint-disable-line no-unused-vars
  buyHelper([{
    supportedMethods: 'basic-card',
    data: {
      supportedTypes: ['debit'],
    },
  }]);
}

/**
 * Merchant requests payment via "basic-card" payment method with "mastercard"
 * as the only supported network.
 */
function buyBasicMasterCard() {  // eslint-disable-line no-unused-vars
  buyHelper([{
    supportedMethods: 'basic-card',
    data: {
      supportedNetworks: ['mastercard'],
    },
  }]);
}
