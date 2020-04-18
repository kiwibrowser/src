/**
 * @license
 * Copyright (c) 2015 The Polymer Project Authors. All rights reserved.
 * This code may only be used under the BSD style license found at http://polymer.github.io/LICENSE.txt
 * The complete set of authors may be found at http://polymer.github.io/AUTHORS.txt
 * The complete set of contributors may be found at http://polymer.github.io/CONTRIBUTORS.txt
 * Code distributed by Google as part of the polymer project is also
 * subject to an additional IP rights grant found at http://polymer.github.io/PATENTS.txt
 */
'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

var __awaiter = undefined && undefined.__awaiter || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) {
            try {
                step(generator.next(value));
            } catch (e) {
                reject(e);
            }
        }
        function rejected(value) {
            try {
                step(generator.throw(value));
            } catch (e) {
                reject(e);
            }
        }
        function step(result) {
            result.done ? resolve(result.value) : new P(function (resolve) {
                resolve(result.value);
            }).then(fulfilled, rejected);
        }
        step((generator = generator.apply(thisArg, _arguments)).next());
    });
};
var dom5 = require('dom5');
var url = require('url');
var docs = require('./ast-utils/docs');
var file_loader_1 = require('./loader/file-loader');
var import_parse_1 = require('./ast-utils/import-parse');
var js_parse_1 = require('./ast-utils/js-parse');
var noop_resolver_1 = require('./loader/noop-resolver');
var string_resolver_1 = require('./loader/string-resolver');
var fs_resolver_1 = require('./loader/fs-resolver');
var xhr_resolver_1 = require('./loader/xhr-resolver');
var error_swallowing_fs_resolver_1 = require('./loader/error-swallowing-fs-resolver');
function reduceMetadata(m1, m2) {
    return {
        elements: m1.elements.concat(m2.elements),
        features: m1.features.concat(m2.features),
        behaviors: m1.behaviors.concat(m2.behaviors)
    };
}
var EMPTY_METADATA = { elements: [], features: [], behaviors: [] };
/**
 * A database of Polymer metadata defined in HTML
 */

var Analyzer = function () {
    /**
     * @param  {boolean} attachAST  If true, attach a parse5 compliant AST
     * @param  {FileLoader=} loader An optional `FileLoader` used to load external
     *                              resources
     */
    function Analyzer(attachAST, loader) {
        _classCallCheck(this, Analyzer);

        /**
         * A list of all elements the `Analyzer` has metadata for.
         */
        this.elements = [];
        /**
         * A view into `elements`, keyed by tag name.
         */
        this.elementsByTagName = {};
        /**
         * A list of API features added to `Polymer.Base` encountered by the
         * analyzer.
         */
        this.features = [];
        /**
         * The behaviors collected by the analysis pass.
         */
        this.behaviors = [];
        /**
         * The behaviors collected by the analysis pass by name.
         */
        this.behaviorsByName = {};
        /**
         * A map, keyed by absolute path, of Document metadata.
         */
        this.html = {};
        /**
         * A map, keyed by path, of HTML document ASTs.
         */
        this.parsedDocuments = {};
        /**
         * A map, keyed by path, of JS script ASTs.
         *
         * If the path is an HTML file with multiple scripts,
         * the entry will be an array of scripts.
         */
        this.parsedScripts = {};
        /**
         * A map, keyed by path, of document content.
         */
        this._content = {};
        this.loader = loader;
    }

    _createClass(Analyzer, [{
        key: 'load',
        value: function load(href) {
            var _this = this;

            return this.loader.request(href).then(function (content) {
                return new Promise(function (resolve, reject) {
                    setTimeout(function () {
                        _this._content[href] = content;
                        resolve(_this._parseHTML(content, href));
                    }, 0);
                }).catch(function (err) {
                    console.error("Error processing document at " + href);
                    throw err;
                });
            });
        }
    }, {
        key: '_parseHTML',

        /**
         * Returns an `AnalyzedDocument` representing the provided document
         * @private
         * @param  {string} htmlImport Raw text of an HTML document
         * @param  {string} href       The document's URL.
         * @return {AnalyzedDocument}       An  `AnalyzedDocument`
         */
        value: function _parseHTML(htmlImport, href) {
            var _this2 = this;

            if (href in this.html) {
                return this.html[href];
            }
            var depsLoaded = [];
            var depHrefs = [];
            var metadataLoaded = Promise.resolve(EMPTY_METADATA);
            var parsed;
            try {
                parsed = import_parse_1.importParse(htmlImport, href);
            } catch (err) {
                console.error('Error parsing!');
                throw err;
            }
            var htmlLoaded = Promise.resolve(parsed);
            if (parsed.script) {
                metadataLoaded = this._processScripts(parsed.script, href);
            }
            var commentText = parsed.comment.map(function (comment) {
                return dom5.getTextContent(comment);
            });
            var pseudoElements = docs.parsePseudoElements(commentText);
            var _iteratorNormalCompletion = true;
            var _didIteratorError = false;
            var _iteratorError = undefined;

            try {
                for (var _iterator = pseudoElements[Symbol.iterator](), _step; !(_iteratorNormalCompletion = (_step = _iterator.next()).done); _iteratorNormalCompletion = true) {
                    var element = _step.value;

                    element.contentHref = href;
                    this.elements.push(element);
                    if (element.is) {
                        this.elementsByTagName[element.is] = element;
                    }
                }
            } catch (err) {
                _didIteratorError = true;
                _iteratorError = err;
            } finally {
                try {
                    if (!_iteratorNormalCompletion && _iterator.return) {
                        _iterator.return();
                    }
                } finally {
                    if (_didIteratorError) {
                        throw _iteratorError;
                    }
                }
            }

            metadataLoaded = metadataLoaded.then(function (metadata) {
                var metadataEntry = {
                    elements: pseudoElements,
                    features: [],
                    behaviors: []
                };
                return [metadata, metadataEntry].reduce(reduceMetadata);
            });
            depsLoaded.push(metadataLoaded);
            if (this.loader) {
                var baseUri = href;
                if (parsed.base.length > 1) {
                    console.error("Only one base tag per document!");
                    throw "Multiple base tags in " + href;
                } else if (parsed.base.length == 1) {
                    var baseHref = dom5.getAttribute(parsed.base[0], "href");
                    if (baseHref) {
                        baseHref = baseHref + "/";
                        baseUri = url.resolve(baseUri, baseHref);
                    }
                }
                var _iteratorNormalCompletion2 = true;
                var _didIteratorError2 = false;
                var _iteratorError2 = undefined;

                try {
                    for (var _iterator2 = parsed.import[Symbol.iterator](), _step2; !(_iteratorNormalCompletion2 = (_step2 = _iterator2.next()).done); _iteratorNormalCompletion2 = true) {
                        var link = _step2.value;

                        var linkurl = dom5.getAttribute(link, 'href');
                        if (linkurl) {
                            var resolvedUrl = url.resolve(baseUri, linkurl);
                            depHrefs.push(resolvedUrl);
                            depsLoaded.push(this._dependenciesLoadedFor(resolvedUrl, href));
                        }
                    }
                } catch (err) {
                    _didIteratorError2 = true;
                    _iteratorError2 = err;
                } finally {
                    try {
                        if (!_iteratorNormalCompletion2 && _iterator2.return) {
                            _iterator2.return();
                        }
                    } finally {
                        if (_didIteratorError2) {
                            throw _iteratorError2;
                        }
                    }
                }

                var _iteratorNormalCompletion3 = true;
                var _didIteratorError3 = false;
                var _iteratorError3 = undefined;

                try {
                    for (var _iterator3 = parsed.style[Symbol.iterator](), _step3; !(_iteratorNormalCompletion3 = (_step3 = _iterator3.next()).done); _iteratorNormalCompletion3 = true) {
                        var styleElement = _step3.value;

                        if (polymerExternalStyle(styleElement)) {
                            var styleHref = dom5.getAttribute(styleElement, 'href');
                            if (href) {
                                styleHref = url.resolve(baseUri, styleHref);
                                depsLoaded.push(this.loader.request(styleHref).then(function (content) {
                                    _this2._content[styleHref] = content;
                                    return {};
                                }));
                            }
                        }
                    }
                } catch (err) {
                    _didIteratorError3 = true;
                    _iteratorError3 = err;
                } finally {
                    try {
                        if (!_iteratorNormalCompletion3 && _iterator3.return) {
                            _iterator3.return();
                        }
                    } finally {
                        if (_didIteratorError3) {
                            throw _iteratorError3;
                        }
                    }
                }
            }
            var depsStrLoaded = Promise.all(depsLoaded).then(function () {
                return depHrefs;
            }).catch(function (err) {
                throw err;
            });
            this.parsedDocuments[href] = parsed.ast;
            this.html[href] = {
                href: href,
                htmlLoaded: htmlLoaded,
                metadataLoaded: metadataLoaded,
                depHrefs: depHrefs,
                depsLoaded: depsStrLoaded
            };
            return this.html[href];
        }
    }, {
        key: '_processScripts',
        value: function _processScripts(scripts, href) {
            var _this3 = this;

            var scriptPromises = [];
            scripts.forEach(function (script) {
                scriptPromises.push(_this3._processScript(script, href));
            });
            return Promise.all(scriptPromises).then(function (metadataList) {
                // TODO(ajo) remove this cast.
                var list = metadataList;
                return list.reduce(reduceMetadata, EMPTY_METADATA);
            });
        }
    }, {
        key: '_processScript',
        value: function _processScript(script, href) {
            var _this4 = this;

            var src = dom5.getAttribute(script, 'src');
            var parsedJs;
            if (!src) {
                try {
                    parsedJs = js_parse_1.jsParse(script.childNodes.length ? script.childNodes[0].value : '');
                } catch (err) {
                    // Figure out the correct line number for the error.
                    var line = 0;
                    var col = 0;
                    if (script.__ownerDocument && script.__ownerDocument == href) {
                        line = script.__locationDetail.line - 1;
                        col = script.__locationDetail.column - 1;
                    }
                    line += err.lineNumber;
                    col += err.column;
                    var message = "Error parsing script in " + href + " at " + line + ":" + col;
                    message += "\n" + err.stack;
                    var fixedErr = new Error(message);
                    fixedErr.location = { line: line, column: col };
                    fixedErr.ownerDocument = script.__ownerDocument;
                    return Promise.reject(fixedErr);
                }
                if (parsedJs.elements) {
                    parsedJs.elements.forEach(function (element) {
                        element.scriptElement = script;
                        element.contentHref = href;
                        _this4.elements.push(element);
                        if (element.is in _this4.elementsByTagName) {
                            console.warn('Ignoring duplicate element definition: ' + element.is);
                        } else if (element.is) {
                            _this4.elementsByTagName[element.is] = element;
                        }
                    });
                }
                if (parsedJs.features) {
                    parsedJs.features.forEach(function (feature) {
                        feature.contentHref = href;
                        feature.scriptElement = script;
                    });
                    this.features = this.features.concat(parsedJs.features);
                }
                if (parsedJs.behaviors) {
                    parsedJs.behaviors.forEach(function (behavior) {
                        behavior.contentHref = href;
                        _this4.behaviorsByName[behavior.is] = behavior;
                        _this4.behaviorsByName[behavior.symbol] = behavior;
                    });
                    this.behaviors = this.behaviors.concat(parsedJs.behaviors);
                }
                if (!Object.hasOwnProperty.call(this.parsedScripts, href)) {
                    this.parsedScripts[href] = [];
                }
                var scriptElement;
                if (script.__ownerDocument && script.__ownerDocument == href) {
                    scriptElement = script;
                }
                this.parsedScripts[href].push({
                    ast: parsedJs.parsedScript,
                    scriptElement: scriptElement
                });
                return Promise.resolve(parsedJs);
            }
            if (this.loader) {
                var resolvedSrc = url.resolve(href, src);
                return this.loader.request(resolvedSrc).then(function (content) {
                    _this4._content[resolvedSrc] = content;
                    var scriptText = dom5.constructors.text(content);
                    dom5.append(script, scriptText);
                    dom5.removeAttribute(script, 'src');
                    script.__hydrolysisInlined = src;
                    return _this4._processScript(script, resolvedSrc);
                }).catch(function (err) {
                    throw err;
                });
            } else {
                return Promise.resolve(EMPTY_METADATA);
            }
        }
    }, {
        key: '_dependenciesLoadedFor',
        value: function _dependenciesLoadedFor(href, root) {
            var _this5 = this;

            var found = {};
            if (root !== undefined) {
                found[root] = true;
            }
            return this._getDependencies(href, found).then(function (deps) {
                var depPromises = deps.map(function (depHref) {
                    return _this5.load(depHref).then(function (htmlMonomer) {
                        return htmlMonomer.metadataLoaded;
                    });
                });
                return Promise.all(depPromises);
            });
        }
    }, {
        key: '_getDependencies',

        /**
         * List all the html dependencies for the document at `href`.
         * @param  {string}                   href      The href to get dependencies for.
         * @param  {Object.<string,boolean>=} found     An object keyed by URL of the
         *     already resolved dependencies.
         * @param  {boolean=}                transitive Whether to load transitive
         *     dependencies. Defaults to true.
         * @return {Array.<string>}  A list of all the html dependencies.
         */
        value: function _getDependencies(href, found, transitive) {
            var _this6 = this;

            if (found === undefined) {
                found = {};
                found[href] = true;
            }
            if (transitive === undefined) {
                transitive = true;
            }
            var deps = [];
            return this.load(href).then(function (htmlMonomer) {
                var transitiveDeps = [];
                htmlMonomer.depHrefs.forEach(function (depHref) {
                    if (found[depHref]) {
                        return;
                    }
                    deps.push(depHref);
                    found[depHref] = true;
                    if (transitive) {
                        transitiveDeps.push(_this6._getDependencies(depHref, found));
                    }
                });
                return Promise.all(transitiveDeps);
            }).then(function (transitiveDeps) {
                var alldeps = transitiveDeps.reduce(function (a, b) {
                    return a.concat(b);
                }, []).concat(deps);
                return alldeps;
            });
        }
    }, {
        key: 'elementsForFolder',

        /**
         * Returns the elements defined in the folder containing `href`.
         * @param {string} href path to search.
         */
        value: function elementsForFolder(href) {
            return this.elements.filter(function (element) {
                return matchesDocumentFolder(element, href);
            });
        }
    }, {
        key: 'behaviorsForFolder',

        /**
         * Returns the behaviors defined in the folder containing `href`.
         * @param {string} href path to search.
         * @return {Array.<BehaviorDescriptor>}
         */
        value: function behaviorsForFolder(href) {
            return this.behaviors.filter(function (behavior) {
                return matchesDocumentFolder(behavior, href);
            });
        }
    }, {
        key: 'metadataTree',

        /**
         * Returns a promise that resolves to a POJO representation of the import
         * tree, in a format that maintains the ordering of the HTML imports spec.
         * @param {string} href the import to get metadata for.
         * @return {Promise}
         */
        value: function metadataTree(href) {
            var _this7 = this;

            return this.load(href).then(function (monomer) {
                var loadedHrefs = {};
                loadedHrefs[href] = true;
                return _this7._metadataTree(monomer, loadedHrefs);
            });
        }
    }, {
        key: '_metadataTree',
        value: function _metadataTree(htmlMonomer, loadedHrefs) {
            return __awaiter(this, void 0, void 0, regeneratorRuntime.mark(function _callee() {
                var metadata, hrefs, depMetadata, _iteratorNormalCompletion4, _didIteratorError4, _iteratorError4, _iterator4, _step4, href, metadataPromise;

                return regeneratorRuntime.wrap(function _callee$(_context) {
                    while (1) {
                        switch (_context.prev = _context.next) {
                            case 0:
                                if (loadedHrefs === undefined) {
                                    loadedHrefs = {};
                                }
                                _context.next = 3;
                                return htmlMonomer.metadataLoaded;

                            case 3:
                                metadata = _context.sent;

                                metadata = {
                                    elements: metadata.elements,
                                    features: metadata.features,
                                    behaviors: [],
                                    href: htmlMonomer.href
                                };
                                _context.next = 7;
                                return htmlMonomer.depsLoaded;

                            case 7:
                                hrefs = _context.sent;
                                depMetadata = [];
                                _iteratorNormalCompletion4 = true;
                                _didIteratorError4 = false;
                                _iteratorError4 = undefined;
                                _context.prev = 12;
                                _iterator4 = hrefs[Symbol.iterator]();

                            case 14:
                                if (_iteratorNormalCompletion4 = (_step4 = _iterator4.next()).done) {
                                    _context.next = 29;
                                    break;
                                }

                                href = _step4.value;
                                metadataPromise = void 0;

                                if (loadedHrefs[href]) {
                                    _context.next = 24;
                                    break;
                                }

                                loadedHrefs[href] = true;
                                metadataPromise = this._metadataTree(this.html[href], loadedHrefs);
                                _context.next = 22;
                                return metadataPromise;

                            case 22:
                                _context.next = 25;
                                break;

                            case 24:
                                metadataPromise = Promise.resolve({});

                            case 25:
                                depMetadata.push(metadataPromise);

                            case 26:
                                _iteratorNormalCompletion4 = true;
                                _context.next = 14;
                                break;

                            case 29:
                                _context.next = 35;
                                break;

                            case 31:
                                _context.prev = 31;
                                _context.t0 = _context['catch'](12);
                                _didIteratorError4 = true;
                                _iteratorError4 = _context.t0;

                            case 35:
                                _context.prev = 35;
                                _context.prev = 36;

                                if (!_iteratorNormalCompletion4 && _iterator4.return) {
                                    _iterator4.return();
                                }

                            case 38:
                                _context.prev = 38;

                                if (!_didIteratorError4) {
                                    _context.next = 41;
                                    break;
                                }

                                throw _iteratorError4;

                            case 41:
                                return _context.finish(38);

                            case 42:
                                return _context.finish(35);

                            case 43:
                                return _context.abrupt('return', Promise.all(depMetadata).then(function (importMetadata) {
                                    // TODO(ajo): remove this when tsc stops having issues.
                                    metadata.imports = importMetadata;
                                    return htmlMonomer.htmlLoaded.then(function (parsedHtml) {
                                        metadata.html = parsedHtml;
                                        if (metadata.elements) {
                                            metadata.elements.forEach(function (element) {
                                                attachDomModule(parsedHtml, element);
                                            });
                                        }
                                        return metadata;
                                    });
                                }));

                            case 44:
                            case 'end':
                                return _context.stop();
                        }
                    }
                }, _callee, this, [[12, 31, 35, 43], [36,, 38, 42]]);
            }));
        }
    }, {
        key: '_inlineStyles',
        value: function _inlineStyles(ast, href) {
            var _this8 = this;

            var cssLinks = dom5.queryAll(ast, polymerExternalStyle);
            cssLinks.forEach(function (link) {
                var linkHref = dom5.getAttribute(link, 'href');
                var uri = url.resolve(href, linkHref);
                var content = _this8._content[uri];
                var style = dom5.constructors.element('style');
                dom5.setTextContent(style, '\n' + content + '\n');
                dom5.replace(link, style);
            });
            return cssLinks.length > 0;
        }
    }, {
        key: '_inlineScripts',
        value: function _inlineScripts(ast, href) {
            var _this9 = this;

            var scripts = dom5.queryAll(ast, externalScript);
            scripts.forEach(function (script) {
                var scriptHref = dom5.getAttribute(script, 'src');
                var uri = url.resolve(href, scriptHref);
                var content = _this9._content[uri];
                var inlined = dom5.constructors.element('script');
                dom5.setTextContent(inlined, '\n' + content + '\n');
                dom5.replace(script, inlined);
            });
            return scripts.length > 0;
        }
    }, {
        key: '_inlineImports',
        value: function _inlineImports(ast, href, loaded) {
            var _this10 = this;

            var imports = dom5.queryAll(ast, isHtmlImportNode);
            imports.forEach(function (htmlImport) {
                var importHref = dom5.getAttribute(htmlImport, 'href');
                var uri = url.resolve(href, importHref);
                if (loaded[uri]) {
                    dom5.remove(htmlImport);
                    return;
                }
                var content = _this10.getLoadedAst(uri, loaded);
                dom5.replace(htmlImport, content);
            });
            return imports.length > 0;
        }
    }, {
        key: 'getLoadedAst',

        /**
         * Returns a promise resolving to a form of the AST with all links replaced
         * with the document they link to. .css and .script files become &lt;style&gt; and
         * &lt;script&gt;, respectively.
         *
         * The elements in the loaded document are unmodified from their original
         * documents.
         *
         * @param {string} href The document to load.
         * @param {Object.<string,boolean>=} loaded An object keyed by already loaded documents.
         * @return {Promise.<DocumentAST>}
         */
        value: function getLoadedAst(href, loaded) {
            if (!loaded) {
                loaded = {};
            }
            loaded[href] = true;
            var parsedDocument = this.parsedDocuments[href];
            var analyzedDocument = this.html[href];
            var astCopy = dom5.parse(dom5.serialize(parsedDocument));
            // Whenever we inline something, reset inlined to true to know that anoather
            // inlining pass is needed;
            this._inlineStyles(astCopy, href);
            this._inlineScripts(astCopy, href);
            this._inlineImports(astCopy, href, loaded);
            return astCopy;
        }
    }, {
        key: 'nodeWalkDocuments',

        /**
         * Calls `dom5.nodeWalkAll` on each document that `Anayzler` has laoded.
         */
        value: function nodeWalkDocuments(predicate) {
            var results = [];
            for (var href in this.parsedDocuments) {
                var newNodes = dom5.nodeWalkAll(this.parsedDocuments[href], predicate);
                results = results.concat(newNodes);
            }
            return results;
        }
    }, {
        key: 'nodeWalkAllDocuments',

        /**
         * Calls `dom5.nodeWalkAll` on each document that `Anayzler` has laoded.
         *
         * TODO: make nodeWalkAll & nodeWalkAllDocuments distict, or delete one.
         */
        value: function nodeWalkAllDocuments(predicate) {
            var results = [];
            for (var href in this.parsedDocuments) {
                var newNodes = dom5.nodeWalkAll(this.parsedDocuments[href], predicate);
                results = results.concat(newNodes);
            }
            return results;
        }
    }, {
        key: 'annotate',

        /** Annotates all loaded metadata with its documentation. */
        value: function annotate() {
            var _this11 = this;

            if (this.features.length > 0) {
                var featureEl = docs.featureElement(this.features);
                this.elements.unshift(featureEl);
                if (featureEl.is) {
                    this.elementsByTagName[featureEl.is] = featureEl;
                }
            }
            var behaviorsByName = this.behaviorsByName;
            var elementHelper = function elementHelper(descriptor) {
                docs.annotateElement(descriptor, behaviorsByName);
            };
            this.elements.forEach(elementHelper);
            this.behaviors.forEach(elementHelper); // Same shape.
            this.behaviors.forEach(function (behavior) {
                if (behavior.is !== behavior.symbol && behavior.symbol) {
                    _this11.behaviorsByName[behavior.symbol] = undefined;
                }
            });
        }
    }, {
        key: 'clean',

        /** Removes redundant properties from the collected descriptors. */
        value: function clean() {
            this.elements.forEach(docs.cleanElement);
        }
    }]);

    return Analyzer;
}();
/**
 * Shorthand for transitively loading and processing all imports beginning at
 * `href`.
 *
 * In order to properly filter paths, `href` _must_ be an absolute URI.
 *
 * @param {string} href The root import to begin loading from.
 * @param {LoadOptions=} options Any additional options for the load.
 * @return {Promise<Analyzer>} A promise that will resolve once `href` and its
 *     dependencies have been loaded and analyzed.
 */


Analyzer.analyze = function analyze(href, options) {
    options = options || {};
    options.filter = options.filter || _defaultFilter(href);
    var loader = new file_loader_1.FileLoader();
    var resolver = options.resolver;
    if (resolver === undefined) {
        if (typeof window === 'undefined') {
            resolver = 'fs';
        } else {
            resolver = 'xhr';
        }
    }
    var primaryResolver = void 0;
    if (resolver === 'fs') {
        primaryResolver = new fs_resolver_1.FSResolver(options);
    } else if (resolver === 'xhr') {
        primaryResolver = new xhr_resolver_1.XHRResolver(options);
    } else if (resolver === 'permissive') {
        primaryResolver = new error_swallowing_fs_resolver_1.ErrorSwallowingFSResolver(options);
    } else {
        throw new Error("Resolver must be one of 'fs', 'xhr', or 'permissive'");
    }
    loader.addResolver(primaryResolver);
    if (options.content) {
        loader.addResolver(new string_resolver_1.StringResolver({ url: href, content: options.content }));
    }
    loader.addResolver(new noop_resolver_1.NoopResolver({ test: options.filter }));
    var analyzer = new Analyzer(false, loader);
    return analyzer.metadataTree(href).then(function (root) {
        if (!options.noAnnotations) {
            analyzer.annotate();
        }
        if (options.clean) {
            analyzer.clean();
        }
        return Promise.resolve(analyzer);
    });
};
exports.Analyzer = Analyzer;
;
/**
 * @private
 * @param {string} href
 * @return {function(string): boolean}
 */
function _defaultFilter(href) {
    // Everything up to the last `/` or `\`.
    var base = href.match(/^(.*?)[^\/\\]*$/)[1];
    return function (uri) {
        return uri.indexOf(base) !== 0;
    };
}
function matchesDocumentFolder(descriptor, href) {
    if (!descriptor.contentHref) {
        return false;
    }
    var descriptorDoc = url.parse(descriptor.contentHref);
    if (!descriptorDoc || !descriptorDoc.pathname) {
        return false;
    }
    var searchDoc = url.parse(href);
    if (!searchDoc || !searchDoc.pathname) {
        return false;
    }
    var searchPath = searchDoc.pathname;
    var lastSlash = searchPath.lastIndexOf("/");
    if (lastSlash > 0) {
        searchPath = searchPath.slice(0, lastSlash);
    }
    return descriptorDoc.pathname.indexOf(searchPath) === 0;
}
// TODO(ajo): Refactor out of vulcanize into dom5.
var polymerExternalStyle = dom5.predicates.AND(dom5.predicates.hasTagName('link'), dom5.predicates.hasAttrValue('rel', 'import'), dom5.predicates.hasAttrValue('type', 'css'));
var externalScript = dom5.predicates.AND(dom5.predicates.hasTagName('script'), dom5.predicates.hasAttr('src'));
var isHtmlImportNode = dom5.predicates.AND(dom5.predicates.hasTagName('link'), dom5.predicates.hasAttrValue('rel', 'import'), dom5.predicates.NOT(dom5.predicates.hasAttrValue('type', 'css')));
function attachDomModule(parsedImport, element) {
    var domModules = parsedImport['dom-module'];
    var _iteratorNormalCompletion5 = true;
    var _didIteratorError5 = false;
    var _iteratorError5 = undefined;

    try {
        for (var _iterator5 = domModules[Symbol.iterator](), _step5; !(_iteratorNormalCompletion5 = (_step5 = _iterator5.next()).done); _iteratorNormalCompletion5 = true) {
            var domModule = _step5.value;

            if (dom5.getAttribute(domModule, 'id') === element.is) {
                element.domModule = domModule;
                return;
            }
        }
    } catch (err) {
        _didIteratorError5 = true;
        _iteratorError5 = err;
    } finally {
        try {
            if (!_iteratorNormalCompletion5 && _iterator5.return) {
                _iterator5.return();
            }
        } finally {
            if (_didIteratorError5) {
                throw _iteratorError5;
            }
        }
    }
}