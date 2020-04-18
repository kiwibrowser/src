"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
Object.defineProperty(exports, "__esModule", { value: true });
const analyzer_utils_1 = require("./analyzer-utils");
/**
 * For a given document, return a set of transitive dependencies, including
 * all eagerly-loaded dependencies and lazy html imports encountered.
 */
function getHtmlDependencies(document) {
    const deps = new Set();
    const eagerDeps = new Set();
    const lazyImports = new Set();
    _getHtmlDependencies(document, true, deps, eagerDeps, lazyImports);
    return { deps, eagerDeps, lazyImports };
}
function _getHtmlDependencies(document, viaEager, visited, visitedEager, lazyImports) {
    const htmlImports = document.getFeatures({ kind: 'html-import', imported: false, externalPackages: true });
    for (const htmlImport of htmlImports) {
        const importUrl = htmlImport.document.url;
        if (htmlImport.lazy) {
            lazyImports.add(importUrl);
        }
        if (visitedEager.has(importUrl)) {
            continue;
        }
        const isEager = viaEager && !htmlImport.lazy;
        if (isEager) {
            visitedEager.add(importUrl);
            // In this case we've visited a node eagerly for the first time,
            // so recurse
        }
        else if (visited.has(importUrl)) {
            // In this case we're seeing a node lazily again, so don't recurse
            continue;
        }
        visited.add(importUrl);
        _getHtmlDependencies(htmlImport.document, isEager, visited, visitedEager, lazyImports);
    }
}
/**
 * Analyzes all entrypoints and determines each of their transitive
 * dependencies.
 * @param entrypoints Urls of entrypoints to analyze.
 * @param analyzer
 * @return a dependency index of every entrypoint, including entrypoints that
 *     were discovered as lazy entrypoints in the graph.
 */
function buildDepsIndex(entrypoints, analyzer) {
    return __awaiter(this, void 0, void 0, function* () {
        const depsIndex = { entrypointToDeps: new Map() };
        const analysis = yield analyzer.analyze(entrypoints);
        const allEntrypoints = new Set(entrypoints);
        // Note: the following iteration takes place over a Set which may be added
        // to from within the loop.
        for (const entrypoint of allEntrypoints) {
            try {
                const document = analyzer_utils_1.getAnalysisDocument(analysis, entrypoint);
                const deps = getHtmlDependencies(document);
                depsIndex.entrypointToDeps.set(entrypoint, new Set([entrypoint, ...deps.eagerDeps]));
                // Add lazy imports to the set of all entrypoints, which supports
                // recursive
                for (const dep of deps.lazyImports) {
                    allEntrypoints.add(dep);
                }
            }
            catch (e) {
                console.warn(e.message);
            }
        }
        return depsIndex;
    });
}
exports.buildDepsIndex = buildDepsIndex;
//# sourceMappingURL=deps-index.js.map