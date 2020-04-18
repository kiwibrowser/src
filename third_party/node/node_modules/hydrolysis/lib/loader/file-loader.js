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

var resolver_1 = require('./resolver');
/**
 * A FileLoader lets you resolve URLs with a set of potential resolvers.
 */

var FileLoader = function () {
    function FileLoader() {
        _classCallCheck(this, FileLoader);

        this.resolvers = [];
        // map url -> Deferred
        this.requests = {};
    }
    /**
     * Add an instance of a Resolver class to the list of url resolvers
     *
     * Ordering of resolvers is most to least recently added
     * The first resolver to "accept" the url wins.
     * @param {Resolver} resolver The resolver to add.
     */


    _createClass(FileLoader, [{
        key: 'addResolver',
        value: function addResolver(resolver) {
            this.resolvers.push(resolver);
        }
    }, {
        key: 'request',

        /**
         * Return a promise for an absolute url
         *
         * Url requests are deduplicated by the loader, returning the same Promise for
         * identical urls
         *
         * @param {string} url        The absolute url to request.
         * @return {Promise.<string>} A promise that resolves to the contents of the URL.
         */
        value: function request(uri) {
            var promise;
            if (!(uri in this.requests)) {
                var handled = false;
                var deferred = new resolver_1.Deferred();
                this.requests[uri] = deferred;
                // loop backwards through resolvers until one "accepts" the request
                for (var i = this.resolvers.length - 1; i >= 0; i--) {
                    var r = this.resolvers[i];
                    if (r.accept(uri, deferred)) {
                        handled = true;
                        break;
                    }
                }
                if (!handled) {
                    deferred.reject(new Error('no resolver found for ' + uri));
                }
                promise = deferred.promise;
            } else {
                promise = this.requests[uri].promise;
            }
            return promise;
        }
    }]);

    return FileLoader;
}();

exports.FileLoader = FileLoader;
;