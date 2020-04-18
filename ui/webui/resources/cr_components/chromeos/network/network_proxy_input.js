// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying and editing a single
 * network proxy value. When the URL or port changes, a 'proxy-change' event is
 * fired with the combined url and port values passed as a single string,
 * url:port.
 */
Polymer({
  is: 'network-proxy-input',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * Whether or not the proxy value can be edited.
     */
    editable: {
      type: Boolean,
      value: false,
    },

    /**
     * A label for the proxy value.
     */
    label: {
      type: String,
      value: 'Proxy',
    },

    /**
     * The proxy object.
     * @type {!CrOnc.ProxyLocation}
     */
    value: {
      type: Object,
      value: function() {
        return {Host: '', Port: 80};
      },
      notify: true,
    },
  },

  focus: function() {
    this.$$('input').focus();
  },

  /**
   * Event triggered when an input value changes.
   * @private
   */
  onValueChange_: function() {
    var port = parseInt(this.value.Port, 10);
    if (isNaN(port))
      port = 80;
    this.value.Port = port;
    this.fire('proxy-change', {value: this.value});
  }
});
