/*
 * Copyright 2017 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* global PaymentRequest:false */
/* global print:false */

var request;

/**
 * Do not query CanMakePayment before showing the Payment Request.
 */
function noQueryShow() {  // eslint-disable-line no-unused-vars
  try {
    request = new PaymentRequest(
        [
          {supportedMethods: 'https://bobpay.com'},
          {
            supportedMethods: 'basic-card',
            data: {supportedNetworks: ['visa']},
          },
        ],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}});
    request.show()
        .then(function(resp) {
          resp.complete('success')
              .then(function() {
                print(JSON.stringify(resp, undefined, 2));
              })
              .catch(function(error) {
                print(error);
              });
        })
        .catch(function(error) {
          print(error);
        });
  } catch (error) {
    print(error.message);
  }
}

/**
 * Queries CanMakePayment and the shows the PaymentRequest after.
 */
function queryShow() {  // eslint-disable-line no-unused-vars
  try {
    request = new PaymentRequest(
        [
          {supportedMethods: 'https://bobpay.com'},
          {
            supportedMethods: 'basic-card',
            data: {supportedNetworks: ['visa']},
          },
        ],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}});
    request.canMakePayment()
        .then(function(result) {
          print(result);

          request.show()
              .then(function(resp) {
                resp.complete('success')
                    .then(function() {
                      print(JSON.stringify(resp, undefined, 2));
                    })
                    .catch(function(error) {
                      print(error);
                    });
              })
              .catch(function(error) {
                print(error);
              });
        })
        .catch(function(error) {
          print(error);
        });
  } catch (error) {
    print(error.message);
  }
}

/**
 * Queries CanMakePayment but does not show the PaymentRequest after.
 */
function queryNoShow() {  // eslint-disable-line no-unused-vars
  try {
    request = new PaymentRequest(
        [
          {supportedMethods: 'https://bobpay.com'},
          {
            supportedMethods: 'basic-card',
            data: {supportedNetworks: ['visa']},
          },
        ],
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
 * Aborts the PaymentRequest UI.
 */
function abort() {  // eslint-disable-line no-unused-vars
  try {
    request.abort()
        .then(function() {
          print('Aborted');
        })
        .catch(function() {
          print('Cannot abort');
        });
  } catch (error) {
    print(error.message);
  }
}
