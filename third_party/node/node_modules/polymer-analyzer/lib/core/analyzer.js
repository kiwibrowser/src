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
/// <reference path="../../custom_typings/main.d.ts" />
const path = require("path");
const model_1 = require("../model/model");
const analysis_context_1 = require("./analysis-context");
class NoKnownParserError extends Error {
}
exports.NoKnownParserError = NoKnownParserError;
;
/**
 * A static analyzer for web projects.
 *
 * An Analyzer can load and parse documents of various types, and extract
 * arbitrary information from the documents, and transitively load
 * dependencies. An Analyzer instance is configured with parsers, and scanners
 * which do the actual work of understanding different file types.
 */
class Analyzer {
    constructor(options) {
        if (options.__contextPromise) {
            this._urlLoader = options.urlLoader;
            this._urlResolver = options.urlResolver;
            this._analysisComplete = options.__contextPromise;
        }
        else {
            const context = new analysis_context_1.AnalysisContext(options);
            this._urlResolver = context.resolver;
            this._urlLoader = context.loader;
            this._analysisComplete = Promise.resolve(context);
        }
    }
    /**
     * Loads, parses and analyzes the root document of a dependency graph and its
     * transitive dependencies.
     */
    analyze(urls) {
        return __awaiter(this, void 0, void 0, function* () {
            const previousAnalysisComplete = this._analysisComplete;
            this._analysisComplete = (() => __awaiter(this, void 0, void 0, function* () {
                const previousContext = yield previousAnalysisComplete;
                return yield previousContext.analyze(urls);
            }))();
            const context = yield this._analysisComplete;
            const results = new Map(urls.map((url) => [url, context.getDocument(url)]));
            return new model_1.Analysis(results);
        });
    }
    analyzePackage() {
        return __awaiter(this, void 0, void 0, function* () {
            const previousAnalysisComplete = this._analysisComplete;
            let _package = null;
            this._analysisComplete = (() => __awaiter(this, void 0, void 0, function* () {
                const previousContext = yield previousAnalysisComplete;
                if (!previousContext.loader.readDirectory) {
                    throw new Error(`This analyzer doesn't support analyzerPackage, ` +
                        `the urlLoader can't list the files in a directory.`);
                }
                const allFiles = yield previousContext.loader.readDirectory('', true);
                // TODO(rictic): parameterize this, perhaps with polymer.json.
                const filesInPackage = allFiles.filter((file) => !model_1.Analysis.isExternal(file));
                const extensions = new Set(previousContext.parsers.keys());
                const filesWithParsers = filesInPackage.filter((fn) => extensions.has(path.extname(fn).substring(1)));
                const newContext = yield previousContext.analyze(filesWithParsers);
                const documentsOrWarnings = new Map(filesWithParsers.map((url) => [url, newContext.getDocument(url)]));
                _package = new model_1.Analysis(documentsOrWarnings);
                return newContext;
            }))();
            yield this._analysisComplete;
            return _package;
        });
    }
    /**
     * Clears all information about the given files from our caches, such that
     * future calls to analyze() will reload these files if they're needed.
     *
     * The analyzer assumes that if this method isn't called with a file's url,
     * then that file has not changed and does not need to be reloaded.
     *
     * @param urls The urls of files which may have changed.
     */
    filesChanged(urls) {
        return __awaiter(this, void 0, void 0, function* () {
            const previousAnalysisComplete = this._analysisComplete;
            this._analysisComplete = (() => __awaiter(this, void 0, void 0, function* () {
                const previousContext = yield previousAnalysisComplete;
                return yield previousContext.filesChanged(urls);
            }))();
            yield this._analysisComplete;
        });
    }
    /**
     * Clear all cached information from this analyzer instance.
     *
     * Note: if at all possible, instead tell the analyzer about the specific
     * files that changed rather than clearing caches like this. Caching provides
     * large performance gains.
     */
    clearCaches() {
        return __awaiter(this, void 0, void 0, function* () {
            const previousAnalysisComplete = this._analysisComplete;
            this._analysisComplete = (() => __awaiter(this, void 0, void 0, function* () {
                const previousContext = yield previousAnalysisComplete;
                return yield previousContext.clearCaches();
            }))();
            yield this._analysisComplete;
        });
    }
    /**
     * Returns a copy of the analyzer.  If options are given, the AnalysisContext
     * is also forked and individual properties are overridden by the options.
     * is forked with the given options.
     *
     * When the analysis context is forked, its cache is preserved, so you will
     * see a mixture of pre-fork and post-fork contents when you analyze with a
     * forked analyzer.
     *
     * Note: this feature is experimental. It may be removed without being
     *     considered a breaking change, so check for its existence before calling
     *     it.
     */
    _fork(options) {
        const contextPromise = (() => __awaiter(this, void 0, void 0, function* () {
            return options ?
                (yield this._analysisComplete)._fork(undefined, options) :
                (yield this._analysisComplete);
        }))();
        return new Analyzer({
            urlLoader: this._urlLoader,
            urlResolver: this._urlResolver,
            __contextPromise: contextPromise
        });
    }
    /**
     * Returns `true` if the provided resolved URL can be loaded.  Obeys the
     * semantics defined by `UrlLoader` and should only be used to check
     * resolved URLs.
     */
    canLoad(resolvedUrl) {
        return this._urlLoader.canLoad(resolvedUrl);
    }
    /**
     * Loads the content at the provided resolved URL.  Obeys the semantics
     * defined by `UrlLoader` and should only be used to attempt to load resolved
     * URLs.
     */
    load(resolvedUrl) {
        return __awaiter(this, void 0, void 0, function* () {
            return (yield this._analysisComplete).load(resolvedUrl);
        });
    }
    /**
     * Returns `true` if the given `url` can be resolved.
     */
    canResolveUrl(url) {
        return this._urlResolver.canResolve(url);
    }
    /**
     * Resoves `url` to a new location.
     */
    resolveUrl(url) {
        return this._urlResolver.resolve(url);
    }
}
exports.Analyzer = Analyzer;

//# sourceMappingURL=analyzer.js.map
