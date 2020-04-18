// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Contains definition for modules and the module loader.
 * The Media Router extension is logically separated into modules. Each module
 * and their corresponding bundle path are registered at startup
 * time. When a module is required, they will be loaded on-demand.
 */

goog.provide('mr.Module');
goog.provide('mr.ModuleId');

goog.require('mr.Logger');
goog.require('mr.PromiseResolver');


/**
 * Identifier for each module.
 * @enum {string}
 */
mr.ModuleId = {
  CAST_CHANNEL_SERVICE: 'mr.cast.ChannelService',
  CAST_SINK_DISCOVERY_SERVICE: 'mr.cast.SinkDiscoveryService',
  CAST_STREAMING_SERVICE: 'mr.mirror.cast.Service',
  SLARTI_SINK_DISCOVERY_SERVICE: 'mr.cloud.slarti.SinkDiscoveryService',
  WEAVE_SINK_DISCOVERY_SERVICE: 'mr.cloud.discovery.WeaveSinkDiscoveryService',
  HANGOUTS_SERVICE: 'mr.mirror.hangouts.HangoutsService',
  MEETINGS_SERVICE: 'mr.mirror.hangouts.MeetingsService',
  PROVIDER_MANAGER: 'mr.ProviderManager',
  WEBRTC_STREAMING_SERVICE: 'mr.mirror.webrtc.WebRtcService'
};


/**
 * Identifier for bundles. A bundle is a js file containing a collection of
 * modules.

 * @enum {string}
 */
mr.Bundle = {
  MAIN: 'background_script.js',
  MIRRORING_CAST_STREAMING: 'mirroring_cast_streaming.js',
  MIRRORING_HANGOUTS: 'mirroring_hangouts.js',
  MIRRORING_WEBRTC: 'mirroring_webrtc.js'
};


/**
 * Base class for a module. When a module is loaded and initialized, it should
 * call mr.Module.onModuleLoaded to inform its dependencies that it is ready.
 */
mr.Module = class {
  /**
   * Returns the module with the given ID if it is already initialized, null
   * otherwise.
   * @param {mr.ModuleId} moduleId
   * @return {?mr.Module}
   */
  static get(moduleId) {
    return mr.Module.moduleById_.get(moduleId) || null;
  }

  /**
   * Loads the module with the given ID. If the module is already loaded, the
   * Promise is resolved immediately.
   * @param {mr.ModuleId} moduleId
   * @return {!Promise<!mr.Module>} Resolved with the module when
   *     it is loaded.
   */
  static load(moduleId) {
    const module = mr.Module.get(moduleId);
    if (module) {
      return Promise.resolve(module);
    }
    let resolver = mr.Module.resolverByModuleId_.get(moduleId);
    if (!resolver) {
      resolver = new mr.PromiseResolver();
      mr.Module.resolverByModuleId_.set(moduleId, resolver);
      mr.Module.loadBundleForModule_(moduleId, resolver);
    }

    return resolver.promise;
  }

  /**
   * Loads the bundle corresponding to the given module. Called the first time
   * a module is requested.
   * @param {mr.ModuleId} moduleId
   * @param {!mr.PromiseResolver<!mr.Module>} resolver Rejected if the bundle
   *     associated with the module won't be loaded due to permanent error.
   * @private
   */
  static loadBundleForModule_(moduleId, resolver) {
    const bundle = mr.Module.getBundle_(moduleId);
    if (!bundle) {
      resolver.reject(new Error(`No corresponding bundle for ${moduleId}`));
      return;
    }

    if (mr.Module.DEFAULT_LOAD_BUNDLES_.has(bundle)) {
      return;
    }

    // Check if the bundle has been not requested previously.
    let bundlePromise = mr.Module.bundlePromises_.get(bundle);
    if (!bundlePromise) {

      mr.Module.logger_.info(`Loading bundle ${bundle} for module ${moduleId}`);
      bundlePromise = mr.Module.doLoadBundle_(bundle);
      mr.Module.bundlePromises_.set(bundle, bundlePromise);
    }

    // Add an error handler to reject the module load request.
    bundlePromise.catch(e => {
      resolver.reject(e);
    });
  }

  /**
   * Returns the bundle associated with a module.
   * @param {mr.ModuleId} moduleId
   * @return {?mr.Bundle}
   * @private
   */
  static getBundle_(moduleId) {
    return mr.Module.MODULE_TO_BUNDLE_MAPPING_.get(moduleId) || null;
  }

  /**
   * Called when a bundle needs to be loaded.
   * @param {mr.Bundle} bundle Name of bundle to load.
   * @return {!Promise<void>} Resolves when the bundle is loaded or rejected if
   *     it failed to load.
   * @private
   */
  static doLoadBundle_(bundle) {
    let resolver = new mr.PromiseResolver();
    resolver.promise.then(
        () => {
          mr.Module.logger_.info(`Bundle ${bundle} loaded`);
        },
        e => {
          mr.Module.logger_.error(`Failed to load bundle ${bundle}`);
          throw e;
        });
    let script = document.createElement('script');
    script.src = chrome.extension.getURL(bundle);
    script.setAttribute('type', 'text/javascript');
    script.async = true;

    script.onload = () => resolver.resolve(undefined);
    script.onerror = () =>
        resolver.reject(new Error(`Failed to load bundle ${bundle}`));
    document.head.appendChild(script);
    return resolver.promise;
  }

  /**
   * Called by a module when it is done loading and initializing. Registers the
   * module and resolves the outstanding promise returned by |load(moduleId)|.
   * @param {mr.ModuleId} moduleId Identifier of the module. No two modules can
   *     have the same identifier.
   * @param {!mr.Module} module The module that is ready.
   */
  static onModuleLoaded(moduleId, module) {
    if (mr.Module.moduleById_.has(moduleId)) {
      throw new Error('Duplicate module ' + moduleId);
    }
    mr.Module.moduleById_.set(moduleId, module);
    const resolver = mr.Module.resolverByModuleId_.get(moduleId);
    if (resolver) {
      resolver.resolve(module);
    }
  }

  /**
   * Used for testing only.
   */
  static clearForTest() {
    mr.Module.moduleById_.clear();
    mr.Module.resolverByModuleId_.clear();
    mr.Module.bundlePromises_.clear();
  }

  /**
   * Subclasses should override this if a mr.EventListener designated this
   * module to forward the events to.
   * @param {*} event The event delivered to the handler. It is the handler's
   *     responsibility to verify that it can handle the event.
   * @param {...*} args Arguments for the event.
   */
  handleEvent(event, ...args) {
    throw new Error('Not implemented');
  }
};


/**
 * Maps a module ID to a bundle ID. Used for loading the bundle that contains
 * a required module.
 * @private @const {!Map<mr.ModuleId, mr.Bundle>}
 */
mr.Module.MODULE_TO_BUNDLE_MAPPING_ = new Map([
  [mr.ModuleId.CAST_CHANNEL_SERVICE, mr.Bundle.MAIN],
  [mr.ModuleId.CAST_SINK_DISCOVERY_SERVICE, mr.Bundle.MAIN],
  [mr.ModuleId.CAST_STREAMING_SERVICE, mr.Bundle.MIRRORING_CAST_STREAMING],
  [mr.ModuleId.SLARTI_SINK_DISCOVERY_SERVICE, mr.Bundle.MAIN],
  [mr.ModuleId.WEAVE_SINK_DISCOVERY_SERVICE, mr.Bundle.MAIN],
  [mr.ModuleId.HANGOUTS_SERVICE, mr.Bundle.MIRRORING_HANGOUTS],
  [mr.ModuleId.MEETINGS_SERVICE, mr.Bundle.MIRRORING_HANGOUTS],
  [mr.ModuleId.PROVIDER_MANAGER, mr.Bundle.MAIN],
  [mr.ModuleId.WEBRTC_STREAMING_SERVICE, mr.Bundle.MIRRORING_WEBRTC]
]);


/**
 * Set of bundles that are loaded by default.
 * @private @const {!Set<mr.Bundle>}
 */
mr.Module.DEFAULT_LOAD_BUNDLES_ = new Set([mr.Bundle.MAIN]);


/** @private {mr.Logger} */
mr.Module.logger_ = mr.Logger.getInstance('mr.Module');

/**
 * The set of modules currently loaded and initialized in the extension, keyed
 * by their IDs.
 * @private {!Map<mr.ModuleId, !mr.Module>}
 */
mr.Module.moduleById_ = new Map();


/**
 * Holds the outstanding promise while a module is being loaded.
 * @private {!Map<mr.ModuleId, !mr.PromiseResolver<!mr.Module>>}
 */
mr.Module.resolverByModuleId_ = new Map();


/**
 * Holds the outstanding promise while a bundle is being loaded.
 * @private {!Map<mr.Bundle, !Promise<void>>}
 */
mr.Module.bundlePromises_ = new Map();
