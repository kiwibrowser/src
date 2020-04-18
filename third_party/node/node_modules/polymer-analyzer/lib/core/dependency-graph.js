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
const utils_1 = require("./utils");
class DocumentRecord {
    static from(from) {
        return new DocumentRecord(from);
    }
    constructor(urlOrFrom) {
        if (typeof urlOrFrom === 'string') {
            this.url = urlOrFrom;
            this.dependencies = new Set();
            this.dependants = new Set();
            this.dependenciesDeferred = new utils_1.Deferred();
        }
        else {
            const from = urlOrFrom;
            this.url = from.url;
            this.dependencies = from.dependencies;
            this.dependants = from.dependants;
            this.dependenciesDeferred = new utils_1.Deferred();
            if (from.dependenciesDeferred.resolved) {
                this.dependenciesDeferred.resolve(this.dependencies);
            }
            else if (from.dependenciesDeferred.rejected) {
                this.dependenciesDeferred.reject(from.dependenciesDeferred.error);
            }
        }
        this.dependenciesDeferred.promise.catch((_) => {
            // no one listens for document rejections yet,
        });
    }
    get dependenciesKnown() {
        return this.dependenciesDeferred.promise;
    }
    toString() {
        let s = `${this.url}\n`;
        for (const dependency of this.dependencies) {
            s += `  -> ${dependency}\n`;
        }
        for (const dependant of this.dependants) {
            s += `  <- ${dependant}\n`;
        }
        return s;
    }
}
/**
 * Maintains bidirectional indexes of the dependency graph, for quick querying.
 */
class DependencyGraph {
    constructor(from) {
        this._documents = new Map();
        if (!from)
            return;
        // Deep copy of `from`
        for (const entry of from._documents.entries()) {
            this._documents.set(entry[0], DocumentRecord.from(entry[1]));
        }
    }
    _getRecordFor(url) {
        let record = this._documents.get(url);
        if (record == null) {
            record = new DocumentRecord(url);
            this._documents.set(url, record);
        }
        return record;
    }
    /**
     * Add dependencies of the given path.
     *
     * @param url The url of a document.
     * @param newDependencies The paths of that document's direct dependencies.
     */
    addDocument(url, dependencies) {
        const record = this._getRecordFor(url);
        for (const dependency of dependencies) {
            record.dependencies.add(dependency);
            const dependencyRecord = this._getRecordFor(dependency);
            dependencyRecord.dependants.add(url);
        }
        record.dependenciesDeferred.resolve(record.dependencies);
    }
    rejectDocument(url, error) {
        this._getRecordFor(url).dependenciesDeferred.reject(error);
    }
    /**
     * Returns a Promise that resolves when the given document and all
     * of its transitive dependencies have been resolved or rejected. This
     * Promise never rejects, if the document or any dependencies are rejected,
     * the Promise still resolves.
     */
    whenReady(url) {
        return __awaiter(this, void 0, void 0, function* () {
            yield this._whenReady(url, new Set());
        });
    }
    _whenReady(key, visited) {
        return __awaiter(this, void 0, void 0, function* () {
            if (visited.has(key)) {
                return;
            }
            visited.add(key);
            const dependenciesKnown = this._getRecordFor(key).dependenciesKnown;
            const forgivingDependenciesKnown = dependenciesKnown.catch((_) => []);
            const deps = yield forgivingDependenciesKnown;
            for (const dep of deps) {
                yield this._whenReady(dep, visited);
            }
        });
    }
    /**
     * Returns a fork of this graph without the documents at the given paths.
     */
    invalidatePaths(paths) {
        const fork = new DependencyGraph(this);
        for (const path of paths) {
            const record = fork._documents.get(path);
            if (!record) {
                continue;
            }
            // Tell the dependencies that `path` is no longer one of their dependants.
            for (const dependency of record.dependencies) {
                const dependencyRecord = fork._documents.get(dependency);
                if (dependencyRecord) {
                    dependencyRecord.dependants.delete(path);
                }
            }
            fork._documents.delete(path);
            // If there are dependents on this record, we must preserve them,
            // as they're only added with an addDocument() call, and there are
            // never repeated addDocument() calls for the same path.
            if (record.dependants.size > 0) {
                const newRecord = fork._getRecordFor(record.url);
                for (const dependant of record.dependants) {
                    newRecord.dependants.add(dependant);
                }
            }
        }
        return fork;
    }
    /**
     * Returns the set of transitive dependencies on the given path.
     *
     * So if A depends on B which depends on C, then getAllDependentsOf(C) will
     * be Set([A,B]), and getAllDependantsOf(B) will be Set([A]).
     */
    getAllDependantsOf(path) {
        const result = new Set();
        this._getAllDependantsOf(path, new Set(), result);
        return result;
    }
    _getAllDependantsOf(path, visited, result) {
        if (visited.has(path)) {
            return;
        }
        visited.add(path);
        const record = this._documents.get(path);
        if (!record) {
            return;
        }
        const dependants = record.dependants;
        for (const dependant of dependants) {
            result.add(dependant);
            this._getAllDependantsOf(dependant, visited, result);
        }
    }
    toString() {
        return Array.from(this._documents.values())
            .map((dr) => dr.toString())
            .join('\n');
    }
}
exports.DependencyGraph = DependencyGraph;

//# sourceMappingURL=dependency-graph.js.map
