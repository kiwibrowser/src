/**
 * @license
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */
'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function getFile(url, deferred, config) {
    /* global XMLHttpRequest:false */
    var x = new XMLHttpRequest();
    if (config && config.withCredentials) {
        x.withCredentials = true;
    }
    x.onload = function () {
        var status = x.status || 0;
        if (status >= 200 && status < 300) {
            deferred.resolve(x.response);
        } else {
            deferred.reject('xhr status: ' + status);
        }
    };
    x.onerror = function (e) {
        deferred.reject(e);
    };
    x.open('GET', url, true);
    if (config && config.responseType) {
        x.responseType = config.responseType;
    }
    x.send();
}
/**
 * Construct a resolver that requests resources over XHR.
 */

var XHRResolver = function () {
    function XHRResolver(config) {
        _classCallCheck(this, XHRResolver);

        this.config = config;
    }

    _createClass(XHRResolver, [{
        key: 'accept',
        value: function accept(uri, deferred) {
            getFile(uri, deferred, this.config);
            return true;
        }
    }]);

    return XHRResolver;
}();

exports.XHRResolver = XHRResolver;
;