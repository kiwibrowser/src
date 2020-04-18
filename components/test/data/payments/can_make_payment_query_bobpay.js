/*
 * Copyright 2016 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* global PaymentRequest:false */
/* global print:false */

var first;
var second;

/**
 * Sets the |first| variable and prints both |first| and |second| only if both
 * were set.
 * @param {object} result The object to print.
 */
function printFirst(result) {
  first = result.toString();
  if (first && second) {
    print(first + ', ' + second);
  }
}

/**
 * Sets the |second| variable and prints both |first| and |second| only if both
 * were set.
 * @param {object} result The object to print.
 */
function printSecond(result) {
  second = result.toString();
  if (first && second) {
    print(first + ', ' + second);
  }
}

/**
 * Checks for existence of Bob Pay twice.
 */
function buy() {  // eslint-disable-line no-unused-vars
  first = null;
  second = null;

  try {
    new PaymentRequest(
        [{supportedMethods: 'https://bobpay.com'}],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}})
        .canMakePayment()
        .then(function(result) {
          printFirst(result);
        })
        .catch(function(error) {
          printFirst(error);
        });
  } catch (error) {
    printFirst(error);
  }

  try {
    new PaymentRequest(
        [{supportedMethods: 'https://bobpay.com'}],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}})
        .canMakePayment()
        .then(function(result) {
          printSecond(result);
        })
        .catch(function(error) {
          printSecond(error);
        });
  } catch (error) {
    printSecond(error);
  }
}

/**
 * Checks for existence of Bob Pay and AlicePay.
 */
function otherBuy() {  // eslint-disable-line no-unused-vars
  first = null;
  second = null;

  try {
    new PaymentRequest(
        [{supportedMethods: 'https://bobpay.com'}],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}})
        .canMakePayment()
        .then(function(result) {
          printFirst(result);
        })
        .catch(function(error) {
          printFirst(error);
        });
  } catch (error) {
    printFirst(error);
  }

  try {
    new PaymentRequest(
        [{supportedMethods: 'https://alicepay.com'}],
        {total: {label: 'Total', amount: {currency: 'USD', value: '5.00'}}})
        .canMakePayment()
        .then(function(result) {
          printSecond(result);
        })
        .catch(function(error) {
          printSecond(error);
        });
  } catch (error) {
    printSecond(error);
  }
}
