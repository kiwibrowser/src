"use strict";
/**
 * @license
 * Copyright (c) 2017 The Polymer Project Authors. All rights reserved.
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
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
const esutil = require("../javascript/esutil");
const esutil_1 = require("../javascript/esutil");
const jsdoc = require("../javascript/jsdoc");
const model_1 = require("../model/model");
const js_utils_1 = require("./js-utils");
const polymer_core_feature_1 = require("./polymer-core-feature");
/**
 * Scans for Polymer 1.x core "features".
 *
 * In the Polymer 1.x core library, the `Polymer.Base` prototype is dynamically
 * augmented with properties via calls to `Polymer.Base._addFeature`. These
 * calls are spread across multiple files and split between the micro, mini,
 * and standard "feature layers". Polymer 2.x does not use this pattern.
 *
 * Example: https://github.com/Polymer/polymer/blob/1.x/src/mini/debouncer.html
 */
class PolymerCoreFeatureScanner {
    scan(document, visit) {
        return __awaiter(this, void 0, void 0, function* () {
            const visitor = new PolymerCoreFeatureVisitor(document);
            yield visit(visitor);
            return { features: visitor.features };
        });
    }
}
exports.PolymerCoreFeatureScanner = PolymerCoreFeatureScanner;
class PolymerCoreFeatureVisitor {
    constructor(document) {
        this.document = document;
        this.features = [];
    }
    /**
     * Scan for `Polymer.Base = {...}`.
     */
    enterAssignmentExpression(assignment, parent) {
        if (assignment.left.type !== 'MemberExpression' ||
            !esutil.matchesCallExpression(assignment.left, ['Polymer', 'Base'])) {
            return;
        }
        const parsedJsdoc = jsdoc.parseJsdoc(esutil.getAttachedComment(parent) || '');
        const feature = new polymer_core_feature_1.ScannedPolymerCoreFeature(this.document.sourceRangeForNode(assignment), assignment, parsedJsdoc.description.trim(), parsedJsdoc);
        this.features.push(feature);
        const rhs = assignment.right;
        if (rhs.type !== 'ObjectExpression') {
            feature.warnings.push(new model_1.Warning({
                message: `Expected assignment to \`Polymer.Base\` to be an object.` +
                    `Got \`${rhs.type}\` instead.`,
                severity: model_1.Severity.WARNING,
                code: 'invalid-polymer-base-assignment',
                sourceRange: this.document.sourceRangeForNode(assignment),
                parsedDocument: this.document
            }));
            return;
        }
        this._scanObjectProperties(rhs, feature);
    }
    /**
     * Scan for `addFeature({...})`.
     */
    enterCallExpression(call, parent) {
        if (call.callee.type !== 'MemberExpression' ||
            !esutil.matchesCallExpression(call.callee, ['Polymer', 'Base', '_addFeature'])) {
            return;
        }
        const parsedJsdoc = jsdoc.parseJsdoc(esutil.getAttachedComment(parent) || '');
        const feature = new polymer_core_feature_1.ScannedPolymerCoreFeature(this.document.sourceRangeForNode(call), call, parsedJsdoc.description.trim(), parsedJsdoc);
        this.features.push(feature);
        if (call.arguments.length !== 1) {
            feature.warnings.push(new model_1.Warning({
                message: `Expected only one argument to \`Polymer.Base._addFeature\`. ` +
                    `Got ${call.arguments.length}.`,
                severity: model_1.Severity.WARNING,
                code: 'invalid-polymer-core-feature-call',
                sourceRange: this.document.sourceRangeForNode(call),
                parsedDocument: this.document
            }));
            return;
        }
        const arg = call.arguments[0];
        if (arg.type !== 'ObjectExpression') {
            feature.warnings.push(new model_1.Warning({
                message: `Expected argument to \`Polymer.Base._addFeature\` to be an ` +
                    `object. Got \`${arg.type}\` instead.`,
                severity: model_1.Severity.WARNING,
                code: 'invalid-polymer-core-feature-call',
                sourceRange: this.document.sourceRangeForNode(call),
                parsedDocument: this.document
            }));
            return;
        }
        this._scanObjectProperties(arg, feature);
    }
    /**
     * Scan all properties of the given object expression and add them to the
     * given feature.
     */
    _scanObjectProperties(obj, feature) {
        for (const prop of obj.properties) {
            const sourceRange = this.document.sourceRangeForNode(prop);
            if (!sourceRange) {
                continue;
            }
            if (esutil.isFunctionType(prop.value)) {
                const method = esutil_1.toScannedMethod(prop, sourceRange, this.document);
                feature.methods.set(method.name, method);
            }
            else {
                const property = js_utils_1.toScannedPolymerProperty(prop, sourceRange, this.document);
                feature.properties.set(property.name, property);
            }
            // TODO(aomarks) Are there any getters/setters on Polymer.Base?
            // TODO(aomarks) Merge with similar code in polymer-element-scanner.
        }
    }
}

//# sourceMappingURL=polymer-core-feature-scanner.js.map
