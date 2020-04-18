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
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
const fs = require("fs");
const path = require("path");
const analyzer_1 = require("../core/analyzer");
const fs_url_loader_1 = require("../url-loader/fs-url-loader");
const overlay_loader_1 = require("../url-loader/overlay-loader");
const now = require("performance-now");
const bowerDir = path.resolve(__dirname, `../../bower_components`);
const inMemoryOverlay = new overlay_loader_1.InMemoryOverlayUrlLoader(new fs_url_loader_1.FSUrlLoader(bowerDir));
const analyzer = new analyzer_1.Analyzer({ urlLoader: inMemoryOverlay });
const filesToAnalyze = [];
for (const baseDir of fs.readdirSync(bowerDir)) {
    const bowerJsonPath = path.join(bowerDir, baseDir, 'bower.json');
    let bowerJson;
    try {
        bowerJson = JSON.parse(fs.readFileSync(bowerJsonPath, 'utf-8'));
    }
    catch (e) {
        continue;
    }
    const main = bowerJson.main || [];
    const mains = Array.isArray(main) ? main : [main];
    for (const mainFile of mains) {
        if (existsSync(path.join(bowerDir, baseDir, mainFile))) {
            filesToAnalyze.push(path.join(baseDir, mainFile));
        }
    }
}
const fakeFileContents = filesToAnalyze.map((fn) => `<link rel="import" href="${fn}">`).join('\n');
inMemoryOverlay.urlContentsMap.set('ephemeral.html', fakeFileContents);
function existsSync(fn) {
    try {
        fs.statSync(fn);
        return true;
    }
    catch (_) {
        return false;
    }
}
function padLeft(str, num) {
    if (str.length < num) {
        return padLeft(' ' + str, num);
    }
    return str;
}
function MiB(usage) {
    return `${(usage / (1024 * 1024)).toFixed(1)}MiB`;
}
function measure() {
    return __awaiter(this, void 0, void 0, function* () {
        if (!global.gc) {
            throw new Error('This benchmark must be run with node --expose-gc.\n' +
                '      Just do:\n             npm run benchmark');
        }
        global.gc();
        const initialMemUse = process.memoryUsage().rss;
        console.log(`Initial rss: ${MiB(initialMemUse)}`);
        const start = now();
        let document;
        const measurements = [];
        for (let i = 0; i < 10; i++) {
            const before = now();
            yield analyzer.filesChanged(['ephemeral.html']);
            document = yield analyzer.analyze(['ephemeral.html']);
            measurements.push(now() - before);
        }
        global.gc();
        const afterInitialAnalyses = process.memoryUsage().rss;
        printMeasurements(measurements);
        console.log(`\n\n\n${document.getFeatures().size} total features resolved.`);
        console.log(`${((now() - start) / 1000).toFixed(2)} seconds total elapsed time`);
        for (let i = 0; i < 100; i++) {
            yield analyzer.filesChanged(['ephemeral.html']);
            yield analyzer.analyze(['ephemeral.html']);
        }
        global.gc();
        const afterMoreAnalyses = process.memoryUsage().rss;
        console.log(`Additional memory used in analyzing all Polymer-owned code: ${MiB(afterInitialAnalyses -
            initialMemUse)}`);
        const leakedMemory = afterMoreAnalyses - afterInitialAnalyses;
        console.log(`Additional memory used after 100 more incremental analyses: ${MiB(afterMoreAnalyses -
            afterInitialAnalyses)}`);
        // TODO(rictic): looks like we've got a memory leak. Need to track this down.
        // This should be < 10MiB, not < 100 MiB.
        const threshold = 300 * (1024 * 1024);
        if (leakedMemory > threshold) {
            console.error(`\n\n==========================================\n` +
                `ERROR: Leaked ${MiB(leakedMemory)}, ` +
                `which is more than the threshold of ${MiB(threshold)}. ` +
                `Exiting with error code 1.` +
                `\n==========================================\n\n`);
            process.exit(1);
        }
    });
}
;
function printMeasurements(measurements) {
    console.log(`\n\n\n\n
      The most important thing to benchmark is the resolve step, as everything
      else is cacheable. Here are times for resolving every element in the
      PolymerElements org.

      The total time for this benchmark will also include the initial parse and
      scan and so should be much much longer.
  `);
    console.log(`${padLeft('ms to analyze file that imports all polymer team\'s elements', 10)}}`);
    for (const elapsed of measurements) {
        console.log(`${padLeft(elapsed.toFixed(0), 10)}`);
    }
}
measure().catch(((err) => console.log(err.stack) && process.exit(1)));

//# sourceMappingURL=parse-all-benchmark.js.map
