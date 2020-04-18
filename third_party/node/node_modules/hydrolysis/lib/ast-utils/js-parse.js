/**
 * @license
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */
/**
* Finds and annotates the Polymer() and modulate() calls in javascript.
*/
'use strict';

var espree = require('espree');
var estraverse = require('estraverse');
var behavior_finder_1 = require('./behavior-finder');
var element_finder_1 = require('./element-finder');
var feature_finder_1 = require('./feature-finder');
// Patch espree to work around https://github.com/eslint/espree/issues/282
(function () {
    var acorn = require("acorn");
    var origEspree = acorn.plugins.espree;
    acorn.plugins.espree = function (instance) {
        var result = origEspree(instance);
        instance.raise = instance.raiseRecoverable = function (pos, message) {
            var loc = acorn.getLineInfo(this.input, pos);
            var err = Object.create(new SyntaxError(message), {
                index: { value: pos },
                lineNumber: { value: loc.line },
                column: { value: loc.column + 1 }
            });
            throw err;
        };
        return result;
    };
})();
function traverse(visitorRegistries) {
    function applyVisitors(name, node, parent) {
        var _iteratorNormalCompletion = true;
        var _didIteratorError = false;
        var _iteratorError = undefined;

        try {
            for (var _iterator = visitorRegistries[Symbol.iterator](), _step; !(_iteratorNormalCompletion = (_step = _iterator.next()).done); _iteratorNormalCompletion = true) {
                var registry = _step.value;

                if (name in registry) {
                    var returnVal = registry[name](node, parent);
                    if (returnVal) {
                        return returnVal;
                    }
                }
            }
        } catch (err) {
            _didIteratorError = true;
            _iteratorError = err;
        } finally {
            try {
                if (!_iteratorNormalCompletion && _iterator.return) {
                    _iterator.return();
                }
            } finally {
                if (_didIteratorError) {
                    throw _iteratorError;
                }
            }
        }
    }
    return {
        enter: function enter(node, parent) {
            return applyVisitors('enter' + node.type, node, parent);
        },
        leave: function leave(node, parent) {
            return applyVisitors('leave' + node.type, node, parent);
        },
        fallback: 'iteration'
    };
}
function jsParse(jsString) {
    var script = espree.parse(jsString, {
        attachComment: true,
        comment: true,
        loc: true,
        ecmaVersion: 6
    });
    var featureInfo = feature_finder_1.featureFinder();
    var behaviorInfo = behavior_finder_1.behaviorFinder();
    var elementInfo = element_finder_1.elementFinder();
    var visitors = [featureInfo, behaviorInfo, elementInfo].map(function (info) {
        return info.visitors;
    });
    estraverse.traverse(script, traverse(visitors));
    return {
        behaviors: behaviorInfo.behaviors,
        elements: elementInfo.elements,
        features: featureInfo.features,
        parsedScript: script
    };
}
exports.jsParse = jsParse;
;