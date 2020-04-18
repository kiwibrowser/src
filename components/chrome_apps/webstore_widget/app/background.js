// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @const {number} */
var APP_WINDOW_WIDTH = 735;

/** @const {number} */
var APP_WINDOW_HEIGHT = 530;

chrome.webstoreWidgetPrivate.onShowWidget.addListener(function(options) {
  if (!options || options.type != 'PRINTER_PROVIDER' || !options.usbId) {
    console.error('Invalid widget options.');
    return;
  }

  chrome.app.window.create('app/main.html', {
    id: JSON.stringify(options),
    frame: 'none',
    innerBounds: {width: APP_WINDOW_WIDTH, height: APP_WINDOW_HEIGHT},
    resizable: false
  }, function(createdWindow) {
    createdWindow.contentWindow.params = {
      filter: {
        'printer_provider_vendor_id': options.usbId.vendorId,
        'printer_provider_product_id': options.usbId.productId
     },
     webstoreUrl: null
    };
  });
});
