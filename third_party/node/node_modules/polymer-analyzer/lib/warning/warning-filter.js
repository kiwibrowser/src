"use strict";
/**
 * @license
 * Copyright (c) 2016 The Polymer Project Authors. All rights reserved.
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
const model_1 = require("../model/model");
class WarningFilter {
    constructor(_options) {
        this.warningCodesToIgnore = new Set();
        this.minimumSeverity = model_1.Severity.INFO;
        if (_options.warningCodesToIgnore) {
            this.warningCodesToIgnore = _options.warningCodesToIgnore;
        }
        if (_options.minimumSeverity != null) {
            this.minimumSeverity = _options.minimumSeverity;
        }
    }
    shouldIgnore(warning) {
        if (this.warningCodesToIgnore.has(warning.code)) {
            return true;
        }
        if (warning.severity > this.minimumSeverity) {
            return true;
        }
        return false;
    }
}
exports.WarningFilter = WarningFilter;

//# sourceMappingURL=warning-filter.js.map
