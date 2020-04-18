// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This Polymer element contains the entire media router interface. It handles
 * hiding and showing specific components.
 * @record
 */
function MediaRouterContainerInterface() {}

/**
 * The list of available sinks.
 * @type {!Array<!media_router.Sink>}
 */
MediaRouterContainerInterface.prototype.allSinks;

/**
 * The list of CastModes to show.
 * @type {!Array<!media_router.CastMode>|undefined}
 */
MediaRouterContainerInterface.prototype.castModeList;

/**
 * The URL to open when the device missing link is clicked.
 * @type {string|undefined}
 */
MediaRouterContainerInterface.prototype.deviceMissingUrl;

/**
 * The URL to open when the cloud services pref learn more link is clicked.
 * @type {string|undefined}
 */
MediaRouterContainerInterface.prototype.firstRunFlowCloudPrefLearnMoreUrl;

/**
 * The URL to open when the first run flow learn more link is clicked.
 * @type {string|undefined}
 */
MediaRouterContainerInterface.prototype.firstRunFlowLearnMoreUrl;

/**
 * The header element.
 * @type {!MediaRouterHeaderElement}
 */
MediaRouterContainerInterface.prototype.header;

/**
 * The header text for the sink list.
 * @type {string|undefined}
 */
MediaRouterContainerInterface.prototype.headerText;

/**
 * The header text tooltip. This would be descriptive of the
 * source origin, whether a host name, tab URL, etc.
 * @type {string|undefined}
 */
MediaRouterContainerInterface.prototype.headerTextTooltip;

/**
 * The issue to show.
 * @type {?media_router.Issue}
 */
MediaRouterContainerInterface.prototype.issue;

/**
 * The list of current routes.
 * @type {!Array<!media_router.Route>|undefined}
 */
MediaRouterContainerInterface.prototype.routeList;

/**
 * Whether the search input should be padded as if it were at the bottom of
 * the dialog.
 * @type {boolean}
 */
MediaRouterContainerInterface.prototype.searchUseBottomPadding;

/**
 * Whether to show the user domain of sinks associated with identity.
 * @type {boolean|undefined}
 */
MediaRouterContainerInterface.prototype.showDomain;

/**
 * Whether to show the first run flow.
 * @type {boolean|undefined}
 */
MediaRouterContainerInterface.prototype.showFirstRunFlow;

/**
 * Whether to show the cloud preference setting in the first run flow.
 * @type {boolean|undefined}
 */
MediaRouterContainerInterface.prototype.showFirstRunFlowCloudPref;

/**
 * Whether the WebUI route controls should be shown instead of the
 * extensionview in the route details view.
 * @type {boolean}
 */
MediaRouterContainerInterface.prototype.useWebUiRouteControls;

/**
 * Adds an event listener callback for an event.
 * @param {string} eventName
 * @param {function(!Event)} callback
 */
MediaRouterContainerInterface.prototype.addEventListener = function(
    eventName, callback) {};

/**
 * Fires a 'report-initial-action' event when the user takes their first
 * action after the dialog opens. Also fires a 'report-initial-action-close'
 * event if that initial action is to close the dialog.
 * @param {!media_router.MediaRouterUserAction} initialAction
 */
MediaRouterContainerInterface.prototype.maybeReportUserFirstAction = function(
    initialAction) {};

/**
 * Updates |currentView_| if the dialog had just opened and there's
 * only one local route.
 */
MediaRouterContainerInterface.prototype.maybeShowRouteDetailsOnOpen =
    function() {};

/**
 * Handles response of previous create route attempt.
 * @param {string} sinkId The ID of the sink to which the Media Route was
 *     creating a route.
 * @param {?media_router.Route} route The newly created route that
 *     corresponds to the sink if route creation succeeded; null otherwise.
 * @param {boolean} isForDisplay Whether or not |route| is for display.
 */
MediaRouterContainerInterface.prototype.onCreateRouteResponseReceived =
    function(sinkId, route, isForDisplay) {};

/**
 * Handles the result of a requested file dialog.
 * @param {string} fileName The name of the file that has been selected.
 */
MediaRouterContainerInterface.prototype.onFileDialogSuccess = function(
    fileName) {};

/**
 * Called when a search has completed up to route creation. |sinkId|
 * identifies the sink that should be in |allSinks|, if a sink was found.
 * @param {string} sinkId The ID of the sink that is the result of the
 *     currently pending search.
 */
MediaRouterContainerInterface.prototype.onReceiveSearchResult = function(
    sinkId) {};

/**
 * Called when the connection to the route controller is invalidated. Switches
 * from route details view to the sink list view.
 */
MediaRouterContainerInterface.prototype.onRouteControllerInvalidated =
    function() {};

/**
 * Sets the selected cast mode to the one associated with |castModeType|,
 * and rebuilds sinks to reflect the change.
 * @param {number} castModeType The type of the selected cast mode.
 */
MediaRouterContainerInterface.prototype.selectCastMode = function(
    castModeType) {};

/**
 * Update the max dialog height and update the positioning of the elements.
 * @param {number} height The max height of the Media Router dialog.
 */
MediaRouterContainerInterface.prototype.updateMaxDialogHeight = function(
    height) {};
