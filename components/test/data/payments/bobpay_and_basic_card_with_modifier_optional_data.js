/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * Launches the PaymentRequest UI with Bob Pay and 'basic-card' as
 * payment methods, and Bob Pay modifier.
 */
function buy() {  // eslint-disable-line no-unused-vars
  try {
    new PaymentRequest(
        [
          {supportedMethods: 'https://bobpay.com'},
          {supportedMethods: 'basic-card'},
        ],
        {
          total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}},
          modifiers: [{
            supportedMethods: 'https://bobpay.com',
            total: {
              label: 'Total',
              amount: {currency: 'USD', value: '4.00'},
            },
            additionalDisplayItems: [{
              label: 'BobPay discount',
              amount: {currency: 'USD', value: '-1.00'},
            }],
            data: {
              discountProgramParticipantId: '86328764873265',
            },
          }],
        })
        .show()
        .then(function(resp) {
          resp.complete('success')
              .then(function() {
                print(JSON.stringify(resp, undefined, 2));
              })
              .catch(function(error) {
                print('complete() rejected<br>' + error.message);
              });
        })
        .catch(function(error) {
          print('show() rejected<br>' + error.message);
        });
  } catch (error) {
    print('exception thrown<br>' + error.message);
  }
}

/**
 * Launches the PaymentRequest UI with 'basic-card' payment method and
 * all cards modifier.
 */
function buyWithAllCardsModifier() {  // eslint-disable-line no-unused-vars
  try {
    new PaymentRequest([{supportedMethods: 'basic-card'}], {
      total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}},
      modifiers: [{
        supportedMethods: 'basic-card',
        total: {
          label: 'Total',
          amount: {currency: 'USD', value: '4.00'},
        },
        additionalDisplayItems: [{
          label: 'basic-card discount',
          amount: {currency: 'USD', value: '-1.00'},
        }],
        data: {discountProgramParticipantId: '86328764873265'},
      }],
    })
        .show()
        .then(function(resp) {
          resp.complete('success')
              .then(function() {
                print(JSON.stringify(resp, undefined, 2));
              })
              .catch(function(error) {
                print('complete() rejected<br>' + error.message);
              });
        })
        .catch(function(error) {
          print('show() rejected<br>' + error.message);
        });
  } catch (error) {
    print('exception thrown<br>' + error.message);
  }
}

/**
 * Launches the PaymentRequest UI with 'basic-card' as payment method and
 * visa credit card modifier.
 */
function buyWithVisaCreditModifier() {  // eslint-disable-line no-unused-vars
  try {
    new PaymentRequest(
        [{supportedMethods: 'basic-card'}],
        {
          total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}},
          modifiers: [{
            supportedMethods: 'basic-card',
            total: {
              label: 'Total',
              amount: {currency: 'USD', value: '4.00'},
            },
            additionalDisplayItems: [{
              label: 'Visa credit discount',
              amount: {currency: 'USD', value: '-1.00'},
            }],
            data: {
              discountProgramParticipantId: '86328764873265',
              supportedTypes: ['credit'],
              supportedNetworks: ['visa'],
            },
          }],
        })
        .show()
        .then(function(resp) {
          resp.complete('success')
              .then(function() {
                print(JSON.stringify(resp, undefined, 2));
              })
              .catch(function(error) {
                print('complete() rejected<br>' + error.message);
              });
        })
        .catch(function(error) {
          print('show() rejected<br>' + error.message);
        });
  } catch (error) {
    print('exception thrown<br>' + error.message);
  }
}

/**
 * Launches the PaymentRequest UI with 'basic-card' as payment method and
 * visa debit card modifier.
 */
function buyWithVisaDebitModifier() {  // eslint-disable-line no-unused-vars
  try {
    new PaymentRequest(
        [{
          supportedMethods: 'basic-card',
        }],
        {
          total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}},
          modifiers: [{
            supportedMethods: 'basic-card',
            total: {
              label: 'Total',
              amount: {currency: 'USD', value: '4.00'},
            },
            additionalDisplayItems: [{
              label: 'Visa debit discount',
              amount: {currency: 'USD', value: '-1.00'},
            }],
            data: {
              discountProgramParticipantId: '86328764873265',
              supportedTypes: ['debit'],
              supportedNetworks: ['visa'],
            },
          }],
        })
        .show()
        .then(function(resp) {
          resp.complete('success')
              .then(function() {
                print(JSON.stringify(resp, undefined, 2));
              })
              .catch(function(error) {
                print('complete() rejected<br>' + error.message);
              });
        })
        .catch(function(error) {
          print('show() rejected<br>' + error.message);
        });
  } catch (error) {
    print('exception thrown<br>' + error.message);
  }
}

/**
 * Launches the PaymentRequest UI with 'basic-card' as payment method and
 * credit card modifier.
 */
function buyWithCreditModifier() {  // eslint-disable-line no-unused-vars
  try {
    new PaymentRequest(
        [{
          supportedMethods: 'basic-card',
        }],
        {
          total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}},
          modifiers: [{
            supportedMethods: 'basic-card',
            total: {
              label: 'Total',
              amount: {currency: 'USD', value: '4.00'},
            },
            additionalDisplayItems: [{
              label: 'Credit card discount',
              amount: {currency: 'USD', value: '-1.00'},
            }],
            data: {
              discountProgramParticipantId: '86328764873265',
              supportedTypes: ['credit'],
            },
          }],
        })
        .show()
        .then(function(resp) {
          resp.complete('success')
              .then(function() {
                print(JSON.stringify(resp, undefined, 2));
              })
              .catch(function(error) {
                print('complete() rejected<br>' + error.message);
              });
        })
        .catch(function(error) {
          print('show() rejected<br>' + error.message);
        });
  } catch (error) {
    print('exception thrown<br>' + error.message);
  }
}

/**
 * Launches the PaymentRequest UI with 'basic-card' as payment method and
 * visa card modifier.
 */
function buyWithVisaModifier() {  // eslint-disable-line no-unused-vars
  try {
    new PaymentRequest(
        [{
          supportedMethods: 'basic-card',
        }],
        {
          total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}},
          modifiers: [{
            supportedMethods: 'basic-card',
            total: {
              label: 'Total',
              amount: {currency: 'USD', value: '4.00'},
            },
            additionalDisplayItems: [{
              label: 'Visa discount',
              amount: {currency: 'USD', value: '-1.00'},
            }],
            data: {
              discountProgramParticipantId: '86328764873265',
              supportedNetworks: ['visa'],
            },
          }],
        })
        .show()
        .then(function(resp) {
          resp.complete('success')
              .then(function() {
                print(JSON.stringify(resp, undefined, 2));
              })
              .catch(function(error) {
                print('complete() rejected<br>' + error.message);
              });
        })
        .catch(function(error) {
          print('show() rejected<br>' + error.message);
        });
  } catch (error) {
    print('exception thrown<br>' + error.message);
  }
}
