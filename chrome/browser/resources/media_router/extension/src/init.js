// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

goog.provide('mr.Init');

goog.require('mr.Config');
goog.require('mr.ExtensionSelector');
goog.require('mr.InitHelper');
goog.require('mr.LogManager');
goog.require('mr.Logger');
goog.require('mr.MediaRouterService');
goog.require('mr.MediumTiming');
goog.require('mr.PersistentDataManager');
goog.require('mr.ProviderManager');
goog.require('mr.TestProvider');


/** @private {mr.Logger} */
mr.Init.logger_ = mr.Logger.getInstance('mr.Init');


/** @type {string} */
mr.Init.FIRST_WAKE_DURATION = 'MediaRouter.Provider.FirstWakeDuration';


/** @type {string} */
mr.Init.WAKE_DURATION = 'MediaRouter.Provider.WakeDuration';


/** @private {?mr.MediumTiming} */
mr.Init.wakeTiming_;


/** @private {?mr.ProviderManager} */
mr.Init.providerManager_;


/**
 * @param {!mr.ProviderManager} providerManager
 * @return {!Array.<!mr.Provider>}
 * @private
 */
mr.Init.getProviders_ = function(providerManager) {
  const providers = mr.InitHelper.getProviders(providerManager);
  if (!mr.Config.isPublicChannel) {
    providers.push(new mr.TestProvider(providerManager));
  }
  return providers;
};


/**
 * @return {!Promise}
 * @private
 */
mr.Init.initProviderManager_ = function() {
  return mr.ExtensionSelector.shouldStart()
      .then(mr.MediaRouterService.getInstance)
      .then(result => {
        if (!result['mrService']) {
          throw Error('Failed to get MR service');
        }
        const mrInstanceId = result['mrInstanceId'];
        if (!mrInstanceId) {
          throw Error('Failed to get MR instance ID.');
        }
        mr.Init.logger_.info('MR instance ID: ' + mrInstanceId);
        const mediaRouterService =
            /** @type {!mr.MediaRouterService} */ (result['mrService']);
        if (!mr.Init.providerManager_) {
          throw Error('providerManager not initialized.');
        }
        /** @type {!mr.ProviderManager} */
        const providerManager = mr.Init.providerManager_;
        mediaRouterService.setHandlers(providerManager);

        if (mr.PersistentDataManager.isChromeReloaded(mrInstanceId)) {
          mr.Init.wakeTiming_.setName(mr.Init.FIRST_WAKE_DURATION);
        }
        chrome.runtime.onSuspend.addListener(
            mr.Init.wakeTiming_.end.bind(mr.Init.wakeTiming_));

        mr.PersistentDataManager.initialize(mrInstanceId);

        mr.LogManager.getInstance().registerDataManager();

        const providers = mr.Init.getProviders_(providerManager);
        if (!mr.Config.isDebugChannel) {
          // Log unhandled promise rejections for external channels,
          // but leave them as thrown exceptions for internal.
          window.addEventListener('unhandledrejection', event => {
            let e = event.reason;
            if (!e.stack) {
              e = Error(e);
            }
            mr.Init.logger_.error(
                'Unhandled promise rejection.',
                /** @type {Error} */ (e));
          });
        }
        providerManager.initialize(
            mediaRouterService, providers, result['mrConfig']);
      })
      .then(undefined, error => {
        mr.Init.logger_.warning(error.message);
        throw error;
      });
};


/**
 * Exposed for testing.

 * @return {!Array<!mr.EventListener>}
 * @private
 */
mr.Init.getAllListeners_ = function() {
  return [
    ...mr.InitHelper.getListeners(),
  ];
};


/**
 * Registers all event listeners.
 * @private
 */
mr.Init.addEventListeners_ = function() {
  mr.Init.getAllListeners_().forEach(
      eventListener => eventListener.addOnStartup());
  mr.InitHelper.addEventListeners();

  // Listen for an event that always get invoked on browser startup. This is
  // necessary because Media Router must know the extension ID in order to wake
  // the extension up, and MR gets the ID when the event page activates for the
  // first time. If the event page never activates, then MR will never be able
  // to connect to it.

  chrome.runtime.onStartup.addListener(() => {});
};


/**
 * @return {!Promise}
 */
mr.Init.init = function() {
  mr.LogManager.getInstance().init();
  mr.Init.wakeTiming_ = new mr.MediumTiming(mr.Init.WAKE_DURATION);

  /** @type {!mr.ProviderManager} */
  const providerManager = new mr.ProviderManager();
  mr.Init.providerManager_ = providerManager;
  const promise = mr.Init.initProviderManager_();
  mr.Init.addEventListeners_();
  return promise;
};
