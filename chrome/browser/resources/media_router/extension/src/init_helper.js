/**
 * @fileoverview Definitions that are specific to the open-source
 * version of the extension.  The functions here don't do anything
 * useful, but they need to be here because they're called by code
 * that's shared with the closed-source version.
 */
goog.module('mr.InitHelper');
goog.module.declareLegacyNamespace();

const DialProvider = goog.require('mr.DialProvider');
const EventListener = goog.require('mr.EventListener');
const Provider = goog.require('mr.Provider');
const ProviderManager = goog.forwardDeclare('mr.ProviderManager');


/**
 * @param {!ProviderManager} providerManager
 * @return {!Array<!Provider>}
 */
function getProviders(providerManager) {
  return [new DialProvider(providerManager)];
}


/**
 * @return {!Array<!EventListener>}
 */
function getListeners() {
  return [];
}


/**
 * @return {void}
 */
function addEventListeners() {}


/**
 * @return {!Function}
 */
function getInternalMessageHandler() {
  return () => {};
}


/**
 * @return {!Function}
 */
function getExternalMessageHandler() {
  return () => {};
}


exports = {
  getProviders,
  getListeners,
  addEventListeners,
  getInternalMessageHandler,
  getExternalMessageHandler,
};
