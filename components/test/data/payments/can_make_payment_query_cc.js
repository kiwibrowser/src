/*
 * Copyright 2016 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * Checks for existence of a complete VISA credit card.
 */
function buy() {  // eslint-disable-line no-unused-vars
  try {
    var request = new PaymentRequest(
        [{supportedMethods: 'basic-card', data: {supportedNetworks: ['visa']}}],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}});
    request.canMakePayment()
        .then(function(result) {
          print(result);
        })
        .catch(function(error) {
          print(error);
        });
  } catch (error) {
    print(error.message);
  }
}

/**
 * Checks for existence of a complete MasterCard credit card.
 */
function other_buy() {  // eslint-disable-line no-unused-vars, camelcase
  try {
    var request = new PaymentRequest(
        [{
          supportedMethods: 'basic-card',
          data: {supportedNetworks: ['mastercard']},
        }],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}});
    request.canMakePayment()
        .then(function(result) {
          print(result);
        })
        .catch(function(error) {
          print(error);
        });
  } catch (error) {
    print(error.message);
  }
}
