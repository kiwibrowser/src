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
const index_1 = require("../index");
/**
 * A basic demo of a linter CLI using the Analyzer API.
 */
function main() {
    return __awaiter(this, void 0, void 0, function* () {
        const basedir = process.cwd();
        const analyzer = new index_1.Analyzer({
            urlLoader: new index_1.FSUrlLoader(basedir),
            urlResolver: new index_1.PackageUrlResolver()
        });
        const warnings = yield getWarnings(analyzer, process.argv[2]);
        const warningPrinter = new index_1.WarningPrinter(process.stderr);
        yield warningPrinter.printWarnings(warnings);
        const worstSeverity = Math.min.apply(Math, warnings.map((w) => w.severity));
        if (worstSeverity === index_1.Severity.ERROR) {
            process.exit(1);
        }
    });
}
;
function getWarnings(analyzer, localPath) {
    return __awaiter(this, void 0, void 0, function* () {
        const result = (yield analyzer.analyze([localPath])).getDocument(localPath);
        if (result instanceof index_1.Document) {
            return result.getWarnings({ imported: false });
        }
        else if (result !== undefined) {
            return [result];
        }
        else {
            return [];
        }
    });
}
main()
    .catch((err) => {
    console.error(err.stack || err.message || err);
    process.exit(1);
});

//# sourceMappingURL=polymer-lint.js.map
