// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The issue object shared between component extension
 *  and Chrome media router.
 */

goog.provide('mr.Issue');
goog.provide('mr.IssueAction');
goog.provide('mr.IssueSeverity');

goog.require('mr.Assertions');

/**
 * Note: keep synced with the issue severity supported by MR's issue manager.
 * @enum {string}
 * @export
 */
mr.IssueSeverity = {
  FATAL: 'fatal',
  WARNING: 'warning',
  NOTIFICATION: 'notification'
};


/**
 * Note: keep synced with the issue actions supported by MR's issue manager.
 * @enum {string}
 * @export
 */
mr.IssueAction = {
  DISMISS: 'dismiss',
  LEARN_MORE: 'learn_more'
};

mr.Issue = class {
  /**
   * @param {string} title
   * @param {mr.IssueSeverity} severity
   */
  constructor(title, severity) {
    /**
     * @type {?string}
     * @export
     */
    this.routeId = null;

    /**
     * @type {mr.IssueSeverity}
     * @export
     */
    this.severity = severity;

    /**
     * When true, this issue takes the whole dialog.
     * @type {boolean}
     * @export
     */
    this.isBlocking = this.severity == mr.IssueSeverity.FATAL ? true : false;

    /**
     * Short description about the issue. Localized string.
     * @type {string}
     * @export
     */
    this.title = title;

    /**
     * Message about issue detail or how to handle issue.
     * Messages should be suitable for end users to decide which actions to
     * take.
     * @type {?string}
     * @export
     */
    this.message = null;

    /**
     * @type {mr.IssueAction}
     * @export
     */
    this.defaultAction = mr.IssueAction.DISMISS;

    /**
     * @type {Array.<mr.IssueAction>}
     * @export
     */
    this.secondaryActions = null;

    /**
     * Required if one action is LEARN_MORE.
     * @type {?number}
     * @export
     */
    this.helpPageId = null;
  }

  /**
   * Sets the action to LEARN_MORE and sets the pageId that is required by the
   * action for targeting.
   * @param {number} pageId
   * @return {!mr.Issue} This object.
   */
  setDefaultActionLearnMore(pageId) {
    mr.Assertions.assert(pageId > 0);
    this.helpPageId = pageId;
    this.defaultAction = mr.IssueAction.LEARN_MORE;
    return this;
  }

  /**
   * @param {Array.<mr.IssueAction>} secondaryActions
   * @return {!mr.Issue} This object.
   */
  setSecondaryActions(secondaryActions) {
    this.secondaryActions = secondaryActions;
    return this;
  }

  /**
   * @param {string} message
   * @return {!mr.Issue} This object.
   */
  setMessage(message) {
    this.message = message;
    return this;
  }

  /**
   * @param {boolean} isBlocking
   * @return {!mr.Issue} This object.
   */
  setIsBlocking(isBlocking) {
    if (!isBlocking && this.severity == mr.IssueSeverity.FATAL) {
      throw Error('All FATAL issues must be blocking.');
    }
    this.isBlocking = isBlocking;
    return this;
  }

  /**
   * @param {string} routeId
   * @return {!mr.Issue} This object.
   */
  setRouteId(routeId) {
    this.routeId = routeId;
    return this;
  }
};
