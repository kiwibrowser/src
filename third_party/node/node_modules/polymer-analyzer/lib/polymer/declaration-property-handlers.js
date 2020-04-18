"use strict";
/**
 * @license
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at
 * http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at
 * http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at
 * http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at
 * http://polymer.github.io/PATENTS.txt
 */
Object.defineProperty(exports, "__esModule", { value: true });
const astValue = require("../javascript/ast-value");
const model_1 = require("../model/model");
const analyze_properties_1 = require("./analyze-properties");
const expression_scanner_1 = require("./expression-scanner");
function getBehaviorAssignmentOrWarning(argNode, document) {
    const behaviorName = astValue.getIdentifierName(argNode);
    if (!behaviorName) {
        return {
            kind: 'warning',
            warning: new model_1.Warning({
                code: 'could-not-determine-behavior-name',
                message: `Could not determine behavior name from expression of type ` +
                    `${argNode.type}`,
                severity: model_1.Severity.WARNING,
                sourceRange: document.sourceRangeForNode(argNode),
                parsedDocument: document
            })
        };
    }
    return {
        kind: 'behaviorAssignment',
        assignment: {
            name: behaviorName,
            sourceRange: document.sourceRangeForNode(argNode)
        }
    };
}
exports.getBehaviorAssignmentOrWarning = getBehaviorAssignmentOrWarning;
/**
 * Returns an object containing functions that will annotate `declaration` with
 * the polymer-specific meaning of the value nodes for the named properties.
 */
function declarationPropertyHandlers(declaration, document) {
    return {
        is(node) {
            if (node.type === 'Literal') {
                declaration.tagName = '' + node.value;
            }
        },
        properties(node) {
            for (const prop of analyze_properties_1.analyzeProperties(node, document)) {
                declaration.addProperty(prop);
            }
        },
        behaviors(node) {
            if (node.type !== 'ArrayExpression') {
                return;
            }
            for (const element of node.elements) {
                const result = getBehaviorAssignmentOrWarning(element, document);
                if (result.kind === 'warning') {
                    declaration.warnings.push(result.warning);
                }
                else {
                    declaration.behaviorAssignments.push(result.assignment);
                }
            }
        },
        observers(node) {
            const observers = extractObservers(node, document);
            if (!observers) {
                return;
            }
            declaration.warnings = declaration.warnings.concat(observers.warnings);
            declaration.observers = declaration.observers.concat(observers.observers);
        },
        listeners(node) {
            if (node.type !== 'ObjectExpression') {
                declaration.warnings.push(new model_1.Warning({
                    code: 'invalid-listeners-declaration',
                    message: '`listeners` property should be an object expression',
                    severity: model_1.Severity.WARNING,
                    sourceRange: document.sourceRangeForNode(node),
                    parsedDocument: document
                }));
                return;
            }
            for (const p of node.properties) {
                const evtName = p.key.type === 'Literal' && p.key.value ||
                    p.key.type === 'Identifier' && p.key.name;
                const handler = p.value.type !== 'Literal' || p.value.value;
                if (typeof evtName !== 'string' || typeof handler !== 'string') {
                    // TODO (maklesoft): Notifiy the user somehow that a listener entry
                    // was not extracted
                    // because the event or handler namecould not be statically analyzed.
                    // E.g. add a low-severity
                    // warning once opting out of rules is supported.
                    continue;
                }
                declaration.listeners.push({ event: evtName, handler: handler });
            }
        }
    };
}
exports.declarationPropertyHandlers = declarationPropertyHandlers;
function extractObservers(observersArray, document) {
    if (observersArray.type !== 'ArrayExpression') {
        return;
    }
    let warnings = [];
    const observers = [];
    for (const element of observersArray.elements) {
        let v = astValue.expressionToValue(element);
        if (v === undefined) {
            v = astValue.CANT_CONVERT;
        }
        const parseResult = expression_scanner_1.parseExpressionInJsStringLiteral(document, element, 'callExpression');
        warnings = warnings.concat(parseResult.warnings);
        observers.push({
            javascriptNode: element,
            expression: v,
            parsedExpression: parseResult.databinding
        });
    }
    return { observers, warnings };
}
exports.extractObservers = extractObservers;

//# sourceMappingURL=declaration-property-handlers.js.map
