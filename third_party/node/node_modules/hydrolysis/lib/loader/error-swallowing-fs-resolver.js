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

function _possibleConstructorReturn(self, call) { if (!self) { throw new ReferenceError("this hasn't been initialised - super() hasn't been called"); } return call && (typeof call === "object" || typeof call === "function") ? call : self; }

function _inherits(subClass, superClass) { if (typeof superClass !== "function" && superClass !== null) { throw new TypeError("Super expression must either be null or a function, not " + typeof superClass); } subClass.prototype = Object.create(superClass && superClass.prototype, { constructor: { value: subClass, enumerable: false, writable: true, configurable: true } }); if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass; }

var fs_resolver_1 = require('./fs-resolver');

var ErrorSwallowingFSResolver = function (_fs_resolver_1$FSReso) {
    _inherits(ErrorSwallowingFSResolver, _fs_resolver_1$FSReso);

    function ErrorSwallowingFSResolver(config) {
        _classCallCheck(this, ErrorSwallowingFSResolver);

        return _possibleConstructorReturn(this, (ErrorSwallowingFSResolver.__proto__ || Object.getPrototypeOf(ErrorSwallowingFSResolver)).call(this, config));
    }

    _createClass(ErrorSwallowingFSResolver, [{
        key: 'accept',
        value: function accept(uri, deferred) {
            var reject = deferred.reject;
            deferred.reject = function (arg) {
                deferred.resolve("");
            };
            return fs_resolver_1.FSResolver.prototype.accept.call(this, uri, deferred);
        }
    }]);

    return ErrorSwallowingFSResolver;
}(fs_resolver_1.FSResolver);

exports.ErrorSwallowingFSResolver = ErrorSwallowingFSResolver;