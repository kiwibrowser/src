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
function scan(document, scanners) {
    return __awaiter(this, void 0, void 0, function* () {
        // Scanners register a visitor to run via the `visit` callback passed to
        // `scan()`. We run these visitors in a batch, then pass control back
        // to the `scan()` methods by resolving a single Promise return for
        // all calls to visit() in a batch when the visitors have run.
        // Then we repeat if any visitors have registered new visitors.
        // Resolves Promises returned by visit() calls
        let batchDone;
        // Promise returned by visit()
        let visitorsPromise;
        // Current batch of visitors
        let visitors;
        // A Promise that runs the next batch of visitors in a microtask
        let runner = null;
        let visitError;
        // Initializes the current batch running state
        function setup() {
            visitorsPromise = new Promise((resolve, _) => {
                batchDone = resolve;
            });
            visitors = [];
            runner = null;
        }
        // Runs the current batch of visitors
        function runVisitors() {
            // Record the current state so that any new visitors are enqueued into
            // a fresh batch.
            const currentVisitors = visitors;
            const currentDoneCallback = batchDone;
            setup();
            try {
                document.visit(currentVisitors);
            }
            finally {
                // Let `scan` continue after calls to visit().then()
                currentDoneCallback();
            }
        }
        ;
        // The callback passed to `scan()`
        function visit(visitor) {
            visitors.push(visitor);
            if (!runner) {
                runner = Promise.resolve().then(runVisitors).catch((error) => {
                    visitError = visitError || error;
                });
            }
            return visitorsPromise;
        }
        ;
        // Ok, go!
        setup();
        const scannerPromises = scanners.map((f) => f.scan(document, visit));
        // This waits for all `scan()` calls to finish
        const nestedResults = yield Promise.all(scannerPromises);
        if (visitError) {
            throw visitError;
        }
        const nestedFeatures = [];
        const warnings = [];
        for (const { features, warnings: w } of nestedResults) {
            nestedFeatures.push(features);
            if (w !== undefined) {
                warnings.push(...w);
            }
        }
        return { features: sortFeatures(nestedFeatures), warnings };
    });
}
exports.scan = scan;
function compareFeaturesBySourceLocation(ent1, ent2) {
    const range1 = ent1.sourceRange;
    const range2 = ent2.sourceRange;
    if (range1 === range2) {
        // Should only be true in the `both null` case
        return 0;
    }
    if (range2 == null) {
        // ent1 comes first
        return -1;
    }
    if (range1 == null) {
        // ent1 comes second
        return 1;
    }
    const position1 = range1.start;
    const position2 = range2.start;
    if (position1.line < position2.line) {
        return -1;
    }
    if (position1.line > position2.line) {
        return 1;
    }
    // Lines are equal, compare by column.
    return position1.column - position2.column;
}
function sortFeatures(unorderedFeatures) {
    const allFeatures = [];
    for (const subArray of unorderedFeatures) {
        allFeatures.push(...subArray);
    }
    return allFeatures.sort(compareFeaturesBySourceLocation);
}

//# sourceMappingURL=scan.js.map
