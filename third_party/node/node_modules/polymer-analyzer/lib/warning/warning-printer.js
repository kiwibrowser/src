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
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
const chalk = require("chalk");
const defaultPrinterOptions = {
    verbosity: 'full',
    color: true
};
class WarningPrinter {
    constructor(_outStream, options) {
        this._outStream = _outStream;
        this._options = Object.assign({}, defaultPrinterOptions, options);
        this._chalk = new chalk.constructor({ enabled: !!this._options.color });
    }
    /**
     * Convenience method around `printWarning`.
     */
    printWarnings(warnings) {
        return __awaiter(this, void 0, void 0, function* () {
            for (const warning of warnings) {
                if (this._options.verbosity === 'full') {
                    this._outStream.write('\n\n');
                }
                this._outStream.write(warning.toString(this._options));
            }
        });
    }
}
exports.WarningPrinter = WarningPrinter;

//# sourceMappingURL=warning-printer.js.map
