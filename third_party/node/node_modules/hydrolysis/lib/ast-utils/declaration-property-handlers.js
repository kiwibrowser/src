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

var astValue = require('./ast-value');
var analyze_properties_1 = require('./analyze-properties');
/**
 * Returns an object containing functions that will annotate `declaration` with
 * the polymer-specificmeaning of the value nodes for the named properties.
 *
 * @param  {ElementDescriptor} declaration The descriptor to annotate.
 * @return {object.<string,function>}      An object containing property
 *                                         handlers.
 */
function declarationPropertyHandlers(declaration) {
    return {
        is: function is(node) {
            if (node.type == 'Literal') {
                declaration.is = node.value.toString();
            }
        },
        properties: function properties(node) {
            var props = analyze_properties_1.analyzeProperties(node);
            for (var i = 0; i < props.length; i++) {
                declaration.properties.push(props[i]);
            }
        },
        behaviors: function behaviors(node) {
            if (node.type != 'ArrayExpression') {
                return;
            }
            var arrNode = node;
            var _iteratorNormalCompletion = true;
            var _didIteratorError = false;
            var _iteratorError = undefined;

            try {
                for (var _iterator = arrNode.elements[Symbol.iterator](), _step; !(_iteratorNormalCompletion = (_step = _iterator.next()).done); _iteratorNormalCompletion = true) {
                    var element = _step.value;

                    var v = astValue.expressionToValue(element);
                    if (v === undefined) {
                        v = astValue.CANT_CONVERT;
                    }
                    declaration.behaviors.push(v);
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
        },
        observers: function observers(node) {
            if (node.type != 'ArrayExpression') {
                return;
            }
            var arrNode = node;
            var _iteratorNormalCompletion2 = true;
            var _didIteratorError2 = false;
            var _iteratorError2 = undefined;

            try {
                for (var _iterator2 = arrNode.elements[Symbol.iterator](), _step2; !(_iteratorNormalCompletion2 = (_step2 = _iterator2.next()).done); _iteratorNormalCompletion2 = true) {
                    var element = _step2.value;

                    var v = astValue.expressionToValue(element);
                    if (v === undefined) v = astValue.CANT_CONVERT;
                    declaration.observers.push({
                        javascriptNode: element,
                        expression: v
                    });
                }
            } catch (err) {
                _didIteratorError2 = true;
                _iteratorError2 = err;
            } finally {
                try {
                    if (!_iteratorNormalCompletion2 && _iterator2.return) {
                        _iterator2.return();
                    }
                } finally {
                    if (_didIteratorError2) {
                        throw _iteratorError2;
                    }
                }
            }
        }
    };
}
exports.declarationPropertyHandlers = declarationPropertyHandlers;