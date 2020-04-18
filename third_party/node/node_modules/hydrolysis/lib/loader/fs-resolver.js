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

var fs = require('fs');
var path = require('path');
var url = require('url');
function getFile(filePath, deferred, secondPath) {
    fs.readFile(filePath, 'utf-8', function (err, content) {
        if (err) {
            if (secondPath) {
                getFile(secondPath, deferred);
            } else {
                console.log("ERROR finding " + filePath);
                deferred.reject(err);
            }
        } else {
            deferred.resolve(content);
        }
    });
}
/**
 * Returns true if `patha` is a sibling or aunt of `pathb`.
 */
function isSiblingOrAunt(patha, pathb) {
    var parent = path.dirname(patha);
    if (pathb.indexOf(patha) === -1 && pathb.indexOf(parent) === 0) {
        return true;
    }
    return false;
}
/**
 * Change `localPath` from a sibling of `basePath` to be a child of
 * `basePath` joined with `redirect`.
 */
function redirectSibling(basePath, localPath, redirect) {
    var parent = path.dirname(basePath);
    var redirected = path.join(basePath, redirect, localPath.slice(parent.length));
    return redirected;
}
/**
 * Resolves requests via the file system.
 */

var FSResolver = function () {
    function FSResolver(config) {
        _classCallCheck(this, FSResolver);

        this.config = config || {};
    }

    _createClass(FSResolver, [{
        key: 'accept',
        value: function accept(uri, deferred) {
            var parsed = url.parse(uri);
            var host = this.config.host;
            var base = this.config.basePath && decodeURIComponent(this.config.basePath);
            var root = this.config.root && path.normalize(this.config.root);
            var redirect = this.config.redirect;
            var local;
            if (!parsed.hostname || parsed.hostname === host) {
                local = parsed.pathname;
            }
            if (local) {
                // un-escape HTML escapes
                local = decodeURIComponent(local);
                if (base) {
                    local = path.relative(base, local);
                }
                if (root) {
                    local = path.join(root, local);
                }
                var backup;
                if (redirect && isSiblingOrAunt(root, local)) {
                    backup = redirectSibling(root, local, redirect);
                }
                getFile(local, deferred, backup);
                return true;
            }
            return false;
        }
    }]);

    return FSResolver;
}();

exports.FSResolver = FSResolver;
;