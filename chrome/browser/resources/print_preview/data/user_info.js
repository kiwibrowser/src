// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('print_preview', function() {
  'use strict';

  class UserInfo extends cr.EventTarget {
    /**
     * Repository which stores information about the user. Events are dispatched
     * when the information changes.
     */
    constructor() {
      super();

      /**
       * Email address of the logged in user or {@code null} if no user is
       * logged in. In case of Google multilogin, can be changed by the user.
       * @private {?string}
       */
      this.activeUser_ = null;

      /**
       * Email addresses of the logged in users or empty array if no user is
       * logged in. {@code null} if not known yet.
       * @private {?Array<string>}
       */
      this.users_ = null;
    }

    /** @return {boolean} Whether user accounts are already retrieved. */
    get initialized() {
      return this.users_ != null;
    }

    /** @return {boolean} Whether user is logged in or not. */
    get loggedIn() {
      return !!this.activeUser;
    }

    /**
     * @return {?string} Email address of the logged in user or {@code null} if
     *     no user is logged.
     */
    get activeUser() {
      return this.activeUser_;
    }

    /**
     * Changes active user.
     * @param {?string} activeUser Email address for the user to be set as
     *     active.
     */
    set activeUser(activeUser) {
      if (!!activeUser && this.activeUser_ != activeUser) {
        this.activeUser_ = activeUser;
        cr.dispatchSimpleEvent(this, UserInfo.EventType.ACTIVE_USER_CHANGED);
      }
    }

    /**
     * @return {?Array<string>} Email addresses of the logged in users or
     *     empty array if no user is logged in. {@code null} if not known yet.
     */
    get users() {
      return this.users_;
    }

    /**
     * Sets logged in user accounts info.
     * @param {string} activeUser Active user account (email).
     * @param {!Array<string>} users List of currently logged in accounts.
     */
    setUsers(activeUser, users) {
      this.activeUser_ = activeUser;
      this.users_ = users || [];
      cr.dispatchSimpleEvent(this, UserInfo.EventType.USERS_CHANGED);
    }
  }

  /**
   * Enumeration of event types dispatched by the user info.
   * @enum {string}
   */
  UserInfo.EventType = {
    ACTIVE_USER_CHANGED: 'print_preview.UserInfo.ACTIVE_USER_CHANGED',
    USERS_CHANGED: 'print_preview.UserInfo.USERS_CHANGED'
  };

  return {UserInfo: UserInfo};
});
