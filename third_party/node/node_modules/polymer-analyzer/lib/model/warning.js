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
const chalk = require("chalk");
const code_printer_1 = require("../warning/code-printer");
class Warning {
    constructor(init) {
        ({
            message: this.message,
            sourceRange: this.sourceRange,
            severity: this.severity,
            code: this.code,
            parsedDocument: this._parsedDocument,
        } = init);
        if (!this.sourceRange) {
            throw new Error(`Attempted to construct a ${this.code} ` +
                `warning without a source range.`);
        }
        if (!this._parsedDocument) {
            throw new Error(`Attempted to construct a ${this.code} ` +
                `warning without a parsed document.`);
        }
    }
    toString(options = {}) {
        const opts = Object.assign({}, defaultPrinterOptions, options);
        const colorize = opts.color ? this._severityToColorFunction(this.severity) :
            (s) => s;
        const severity = this._severityToString(colorize);
        let result = '';
        if (options.verbosity !== 'one-line') {
            const underlined = code_printer_1.underlineCode(this.sourceRange, this._parsedDocument, colorize);
            if (underlined) {
                result += underlined;
            }
            if (options.verbosity === 'code-only') {
                return result;
            }
            result += '\n\n';
        }
        result +=
            (`${this.sourceRange.file}` +
                `(${this.sourceRange.start.line},${this.sourceRange.start.column}) ` +
                `${severity} [${this.code}] - ${this.message}\n`);
        return result;
    }
    _severityToColorFunction(severity) {
        switch (severity) {
            case Severity.ERROR:
                return chalk.red;
            case Severity.WARNING:
                return chalk.yellow;
            case Severity.INFO:
                return chalk.green;
            default:
                const never = severity;
                throw new Error(`Unknown severity value - ${never}` +
                    ` - encountered while printing warning.`);
        }
    }
    _severityToString(colorize) {
        switch (this.severity) {
            case Severity.ERROR:
                return colorize('error');
            case Severity.WARNING:
                return colorize('warning');
            case Severity.INFO:
                return colorize('info');
            default:
                const never = this.severity;
                throw new Error(`Unknown severity value - ${never} - ` +
                    `encountered while printing warning.`);
        }
    }
    toJSON() {
        return {
            code: this.code,
            message: this.message,
            severity: this.severity,
            sourceRange: this.sourceRange,
        };
    }
}
exports.Warning = Warning;
var Severity;
(function (Severity) {
    Severity[Severity["ERROR"] = 0] = "ERROR";
    Severity[Severity["WARNING"] = 1] = "WARNING";
    Severity[Severity["INFO"] = 2] = "INFO";
})(Severity = exports.Severity || (exports.Severity = {}));
// TODO(rictic): can we get rid of this class entirely?
class WarningCarryingException extends Error {
    constructor(warning) {
        super(warning.message);
        this.warning = warning;
    }
}
exports.WarningCarryingException = WarningCarryingException;
const defaultPrinterOptions = {
    verbosity: 'full',
    color: true
};

//# sourceMappingURL=warning.js.map
