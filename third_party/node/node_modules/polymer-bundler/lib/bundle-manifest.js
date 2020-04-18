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
const clone = require("clone");
/**
 * A bundle is a grouping of files which serve the need of one or more
 * entrypoint files.
 */
class Bundle {
    constructor(entrypoints, files) {
        // Set of imports which should be removed when encountered.
        this.stripImports = new Set();
        // Set of imports which could not be loaded.
        this.missingImports = new Set();
        // These sets are updated as bundling occurs.
        this.inlinedHtmlImports = new Set();
        this.inlinedScripts = new Set();
        this.inlinedStyles = new Set();
        this.entrypoints = entrypoints || new Set();
        this.files = files || new Set();
    }
}
exports.Bundle = Bundle;
/**
 * Represents a bundle assigned to an output URL.
 */
class AssignedBundle {
}
exports.AssignedBundle = AssignedBundle;
/**
 * A bundle manifest is a mapping of urls to bundles.
 */
class BundleManifest {
    /**
     * Given a collection of bundles and a BundleUrlMapper to generate urls for
     * them, the constructor populates the `bundles` and `files` index properties.
     */
    constructor(bundles, urlMapper) {
        this.bundles = urlMapper(Array.from(bundles));
        this._bundleUrlForFile = new Map();
        for (const bundleMapEntry of this.bundles) {
            const bundleUrl = bundleMapEntry[0];
            const bundle = bundleMapEntry[1];
            for (const fileUrl of bundle.files) {
                console.assert(!this._bundleUrlForFile.has(fileUrl));
                this._bundleUrlForFile.set(fileUrl, bundleUrl);
            }
        }
    }
    // Returns a clone of the manifest.
    fork() {
        return clone(this);
    }
    // Convenience method to return a bundle for a constituent file url.
    getBundleForFile(url) {
        const bundleUrl = this._bundleUrlForFile.get(url);
        if (bundleUrl) {
            return { bundle: this.bundles.get(bundleUrl), url: bundleUrl };
        }
    }
}
exports.BundleManifest = BundleManifest;
/**
 * Chains multiple bundle strategy functions together so the output of one
 * becomes the input of the next and so-on.
 */
function composeStrategies(strategies) {
    return strategies.reduce((s1, s2) => (b) => s2(s1(b)));
}
exports.composeStrategies = composeStrategies;
/**
 * Given an index of files and their dependencies, produce an array of bundles,
 * where a bundle is defined for each set of dependencies.
 *
 * For example, a dependency index representing the graph:
 *   `a->b, b->c, b->d, e->c, e->f`
 *
 * Would produce an array of three bundles:
 *   `[a]->[a,b,d], [e]->[e,f], [a,e]->[c]`
 */
function generateBundles(depsIndex) {
    const bundles = [];
    // TODO(usergenic): Assert a valid transitive dependencies map; i.e.
    // entrypoints should include themselves as dependencies and entrypoints
    // should *probably* not include other entrypoints as dependencies.
    const invertedIndex = invertMultimap(depsIndex);
    for (const entry of invertedIndex.entries()) {
        const dep = entry[0];
        const entrypoints = entry[1];
        // Find the bundle defined by the specific set of shared dependant
        // entrypoints.
        let bundle = bundles.find((bundle) => setEquals(entrypoints, bundle.entrypoints));
        if (!bundle) {
            bundle = new Bundle(entrypoints);
            bundles.push(bundle);
        }
        bundle.files.add(dep);
    }
    return bundles;
}
exports.generateBundles = generateBundles;
/**
 * Creates a bundle url mapper function which takes a prefix and appends an
 * incrementing value, starting with `1` to the filename.
 */
function generateCountingSharedBundleUrlMapper(urlPrefix) {
    return generateSharedBundleUrlMapper((sharedBundles) => {
        let counter = 0;
        return sharedBundles.map((b) => `${urlPrefix}${++counter}.html`);
    });
}
exports.generateCountingSharedBundleUrlMapper = generateCountingSharedBundleUrlMapper;
/**
 * Generates a strategy function which finds all non-entrypoint bundles which
 * are dependencies of the given entrypoint and merges them into that
 * entrypoint's bundle.
 */
function generateEagerMergeStrategy(entrypoint) {
    return generateMatchMergeStrategy((b) => b.files.has(entrypoint) ||
        b.entrypoints.has(entrypoint) && !getBundleEntrypoint(b));
}
exports.generateEagerMergeStrategy = generateEagerMergeStrategy;
/**
 * Generates a strategy function which finds all bundles matching the predicate
 * function and merges them into the bundle containing the target file.
 */
function generateMatchMergeStrategy(predicate) {
    return (bundles) => mergeMatchingBundles(bundles, predicate);
}
exports.generateMatchMergeStrategy = generateMatchMergeStrategy;
/**
 * Creates a bundle url mapper function which maps non-shared bundles to the
 * urls of their single entrypoint and yields responsibility for naming
 * remaining shared bundle urls to the `mapper` function argument.  The
 * mapper function takes a collection of shared bundles and a url map, calling
 * `.set(url, bundle)` for each.
 */
function generateSharedBundleUrlMapper(mapper) {
    return (bundles) => {
        const urlMap = new Map();
        const sharedBundles = [];
        for (const bundle of bundles) {
            const bundleEntrypoint = getBundleEntrypoint(bundle);
            if (bundleEntrypoint) {
                urlMap.set(bundleEntrypoint, bundle);
            }
            else {
                sharedBundles.push(bundle);
            }
        }
        mapper(sharedBundles)
            .forEach((url, i) => urlMap.set(url, sharedBundles[i]));
        return urlMap;
    };
}
exports.generateSharedBundleUrlMapper = generateSharedBundleUrlMapper;
/**
 * Generates a strategy function to merge all bundles where the dependencies
 * for a bundle are shared by at least 2 entrypoints (default; set
 * `minEntrypoints` to change threshold).
 *
 * This function will convert an array of 4 bundles:
 *   `[a]->[a,b], [a,c]->[d], [c]->[c,e], [f,g]->[f,g,h]`
 *
 * Into the following 3 bundles, including a single bundle for all of the
 * dependencies which are shared by at least 2 entrypoints:
 *   `[a]->[a,b], [c]->[c,e], [a,c,f,g]->[d,f,g,h]`
 */
function generateSharedDepsMergeStrategy(maybeMinEntrypoints) {
    const minEntrypoints = maybeMinEntrypoints === undefined ? 2 : maybeMinEntrypoints;
    if (minEntrypoints < 0) {
        throw new Error(`Minimum entrypoints argument must be non-negative`);
    }
    return generateMatchMergeStrategy((b) => b.entrypoints.size >= minEntrypoints && !getBundleEntrypoint(b));
}
exports.generateSharedDepsMergeStrategy = generateSharedDepsMergeStrategy;
/**
 * A bundle strategy function which merges all shared dependencies into a
 * bundle for an application shell.
 */
function generateShellMergeStrategy(shell, maybeMinEntrypoints) {
    const minEntrypoints = maybeMinEntrypoints === undefined ? 2 : maybeMinEntrypoints;
    if (minEntrypoints < 0) {
        throw new Error(`Minimum entrypoints argument must be non-negative`);
    }
    return composeStrategies([
        // Merge all bundles that are direct dependencies of the shell into the
        // shell.
        generateEagerMergeStrategy(shell),
        // Create a new bundle which contains the contents of all bundles which
        // either...
        generateMatchMergeStrategy((bundle) => {
            // ...contain the shell file
            return bundle.files.has(shell) ||
                // or are dependencies of at least the minimum number of entrypoints
                // and are not entrypoints themselves.
                bundle.entrypoints.size >= minEntrypoints &&
                    !getBundleEntrypoint(bundle);
        }),
        // Don't link to the shell from other bundles.
        generateNoBackLinkStrategy([shell]),
    ]);
}
exports.generateShellMergeStrategy = generateShellMergeStrategy;
/**
 * Generates a strategy function that ensures bundles do not link to given urls.
 * Bundles which contain matching files will still have them inlined.
 */
function generateNoBackLinkStrategy(urls) {
    return (bundles) => {
        for (const bundle of bundles) {
            for (const url of urls) {
                if (!bundle.files.has(url)) {
                    bundle.stripImports.add(url);
                }
            }
        }
        return bundles;
    };
}
exports.generateNoBackLinkStrategy = generateNoBackLinkStrategy;
/**
 * Given an Array of bundles, produce a single bundle with the entrypoints and
 * files of all bundles represented.
 */
function mergeBundles(bundles) {
    const newBundle = new Bundle();
    for (const { entrypoints, files, inlinedHtmlImports, inlinedScripts, inlinedStyles, } of bundles) {
        newBundle.entrypoints =
            new Set([...newBundle.entrypoints, ...entrypoints]);
        newBundle.files = new Set([...newBundle.files, ...files]);
        newBundle.inlinedHtmlImports = new Set([...newBundle.inlinedHtmlImports, ...inlinedHtmlImports]);
        newBundle.inlinedScripts =
            new Set([...newBundle.inlinedScripts, ...inlinedScripts]);
        newBundle.inlinedStyles =
            new Set([...newBundle.inlinedStyles, ...inlinedStyles]);
    }
    return newBundle;
}
exports.mergeBundles = mergeBundles;
/**
 * Return a new bundle array where all bundles within it matching the predicate
 * are merged.
 */
function mergeMatchingBundles(bundles, predicate) {
    const newBundles = Array.from(bundles);
    const bundlesToMerge = newBundles.filter(predicate);
    if (bundlesToMerge.length > 1) {
        for (const bundle of bundlesToMerge) {
            newBundles.splice(newBundles.indexOf(bundle), 1);
        }
        newBundles.push(mergeBundles(bundlesToMerge));
    }
    return newBundles;
}
exports.mergeMatchingBundles = mergeMatchingBundles;
/**
 * Return the entrypoint that represents the given bundle, or null if no
 * entrypoint represents the bundle.
 */
function getBundleEntrypoint(bundle) {
    for (const entrypoint of bundle.entrypoints) {
        if (bundle.files.has(entrypoint)) {
            return entrypoint;
        }
    }
    return null;
}
/**
 * Inverts a map of collections such that  `{a:[c,d], b:[c,e]}` would become
 * `{c:[a,b], d:[a], e:[b]}`.
 */
function invertMultimap(multimap) {
    const inverted = new Map();
    for (const entry of multimap.entries()) {
        const value = entry[0], keys = entry[1];
        for (const key of keys) {
            const set = inverted.get(key) || new Set();
            set.add(value);
            inverted.set(key, set);
        }
    }
    return inverted;
}
/**
 * Returns true if both sets contain exactly the same items.  This check is
 * order-independent.
 */
function setEquals(set1, set2) {
    if (set1.size !== set2.size) {
        return false;
    }
    for (const item of set1) {
        if (!set2.has(item)) {
            return false;
        }
    }
    return true;
}
//# sourceMappingURL=bundle-manifest.js.map