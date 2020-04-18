// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This Polymer element is used to show information about issues related
// to casting.
Polymer({
  is: 'issue-banner',

  properties: {
    /**
     * Maps an issue action type to the resource identifier of the text shown
     * in the action button.
     * This is a property of issue-banner because it is used in tests. This
     * property should always be set before |issue| is set or updated.
     * @private {!Array<string>}
     */
    actionTypeToButtonTextResource_: {
      type: Array,
      readOnly: true,
      value: function() {
        return ['dismissButton', 'learnMoreText'];
      },
    },

    /**
     * The text shown in the default action button.
     * @private {string|undefined}
     */
    defaultActionButtonText_: {
      type: String,
    },

    /**
     * The issue to show.
     * @type {?media_router.Issue|undefined}
     */
    issue: {
      type: Object,
      observer: 'updateActionButtonText_',
    },

    /**
     * The text shown in the secondary action button.
     * @private {string|undefined}
     */
    secondaryActionButtonText_: {
      type: String,
    },
  },

  behaviors: [
    I18nBehavior,
  ],

  /**
   * @param {?media_router.Issue} issue
   * @return {boolean} Whether or not to hide the blocking issue UI.
   * @private
   */
  computeIsBlockingIssueHidden_: function(issue) {
    return !issue || !issue.isBlocking;
  },

  /**
   * @param {?media_router.Issue} issue The current issue.
   * @return {string} The class for the overall issue-banner.
   * @private
   */
  computeIssueClass_: function(issue) {
    if (!issue)
      return '';

    return issue.isBlocking ? 'blocking' : 'non-blocking';
  },

  /**
   * @param {?media_router.Issue} issue
   * @return {boolean} Whether or not to hide the non-blocking issue UI.
   * @private
   */
  computeOptionalActionHidden_: function(issue) {
    return !issue || issue.secondaryActionType === undefined;
  },

  /**
   * Fires an issue-action-click event.
   *
   * @param {number} actionType The type of issue action.
   * @private
   */
  fireIssueActionClick_: function(actionType) {
    this.fire('issue-action-click', {
      id: this.issue.id,
      actionType: actionType,
      helpPageId: this.issue.helpPageId
    });
  },

  /**
   * Called when a default issue action is clicked.
   *
   * @param {!Event} event The event object.
   * @private
   */
  onClickDefaultAction_: function(event) {
    this.fireIssueActionClick_(this.issue.defaultActionType);
  },

  /**
   * Called when an optional issue action is clicked.
   *
   * @param {!Event} event The event object.
   * @private
   */
  onClickOptAction_: function(event) {
    this.fireIssueActionClick_(
        /** @type {number} */ (this.issue.secondaryActionType));
  },

  /**
   * Called when |issue| is updated. This updates the default and secondary
   * action button text.
   *
   * @private
   */
  updateActionButtonText_: function() {
    var defaultText = '';
    var secondaryText = '';
    if (this.issue) {
      defaultText = this.i18n(
          this.actionTypeToButtonTextResource_[this.issue.defaultActionType]);

      if (this.issue.secondaryActionType !== undefined) {
        secondaryText = this.i18n(
            this.actionTypeToButtonTextResource_[this.issue
                                                     .secondaryActionType]);
      }
    }

    this.defaultActionButtonText_ = defaultText;
    this.secondaryActionButtonText_ = secondaryText;
  },
});
