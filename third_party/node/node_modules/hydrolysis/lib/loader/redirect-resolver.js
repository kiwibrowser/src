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

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var path = require('path');
var url = require('url');
var fs_resolver_1 = require('./fs-resolver');
/**
 * A single redirect configuration
 * @param {Object} config              The configuration object
 * @param {string} config.protocol     The protocol this redirect matches.
 * @param {string} config.hostname     The host name this redirect matches.
 * @param {string} config.path         The part of the path to match and
 *                                     replace with 'redirectPath'
 * @param {string} config.redirectPath The local filesystem path that should
 *                                     replace "protocol://hosname/path/"
 */

var ProtocolRedirect = function () {
    function ProtocolRedirect(config) {
        _classCallCheck(this, ProtocolRedirect);

        this.protocol = config.protocol;
        this.hostname = config.hostname;
        this.path = config.path;
        this.redirectPath = config.redirectPath;
    }

    _createClass(ProtocolRedirect, [{
        key: 'redirect',
        value: function redirect(uri) {
            var parsed = url.parse(uri);
            if (this.protocol !== parsed.protocol) {
                return null;
            } else if (this.hostname !== parsed.hostname) {
                return null;
            } else if (parsed.pathname.indexOf(this.path) !== 0) {
                return null;
            }
            return path.join(this.redirectPath, parsed.pathname.slice(this.path.length));
        }
    }]);

    return ProtocolRedirect;
}();

;
/**
 * Resolves protocol://hostname/path to the local filesystem.
 * @constructor
 * @memberof hydrolysis
 * @param {Object} config  configuration options.
 * @param {string} config.root Filesystem root to search. Defaults to the
 *     current working directory.
 * @param {Array.<ProtocolRedirect>} redirects A list of protocol redirects
 *     for the resolver. They are checked for matching first-to-last.
 */

var RedirectResolver = function (_fs_resolver_1$FSReso) {
    _inherits(RedirectResolver, _fs_resolver_1$FSReso);

    function RedirectResolver(config) {
        _classCallCheck(this, RedirectResolver);

        var _this = _possibleConstructorReturn(this, (RedirectResolver.__proto__ || Object.getPrototypeOf(RedirectResolver)).call(this, config));

        _this.redirects = config.redirects || [];
        return _this;
    }

    _createClass(RedirectResolver, [{
        key: 'accept',
        value: function accept(uri, deferred) {
            for (var i = 0; i < this.redirects.length; i++) {
                var redirected = this.redirects[i].redirect(uri);
                if (redirected) {
                    return fs_resolver_1.FSResolver.prototype.accept.call(this, redirected, deferred);
                }
            }
            return false;
        }
    }]);

    return RedirectResolver;
}(fs_resolver_1.FSResolver);

RedirectResolver.ProtocolRedirect = ProtocolRedirect;
exports.RedirectResolver = RedirectResolver;