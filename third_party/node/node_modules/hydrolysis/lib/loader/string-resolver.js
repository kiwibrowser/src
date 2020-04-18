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
/**
 * A resolver that resolves to `config.content` any uri matching config.
 */

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var StringResolver = function () {
    function StringResolver(config) {
        _classCallCheck(this, StringResolver);

        this.url = config.url;
        this.content = config.content;
        if (!this.url || !this.content) {
            throw new Error("Must provide a url and content to the string resolver.");
        }
    }
    /**
     * @param {string}    uri      The absolute URI being requested.
     * @param {!Deferred} deferred The deferred promise that should be resolved if
     *     this resolver handles the URI.
     * @return {boolean} Whether the URI is handled by this resolver.
     */


    _createClass(StringResolver, [{
        key: "accept",
        value: function accept(uri, deferred) {
            var url = this.url;
            if (url instanceof RegExp) {
                if (!url.test(uri)) {
                    return false;
                }
            } else {
                if (uri.indexOf(url) == -1) {
                    return false;
                }
            }
            deferred.resolve(this.content);
            return true;
        }
    }]);

    return StringResolver;
}();

exports.StringResolver = StringResolver;
;