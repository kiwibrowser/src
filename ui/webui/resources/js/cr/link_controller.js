// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview This file provides a class that can be used to open URLs based
 * on user interactions. It ensures a consistent behavior when it comes to
 * holding down Ctrl and Shift while clicking or activating the a link.
 *
 * This depends on the {@code chrome.windows} and {@code chrome.tabs}
 * extensions API.
 */

/**
 * The kind of link open we want to perform.
 * @enum {number}
 */
cr.LinkKind = {
  FOREGROUND_TAB: 0,
  BACKGROUND_TAB: 1,
  WINDOW: 2,
  SELF: 3,
  INCOGNITO: 4
};

cr.define('cr', function() {
  /**
   * This class is used to handle opening of links based on user actions. The
   * following actions are currently implemented:
   *
   * * Press Ctrl and click a link. Or click a link with your middle mouse
   *   button (or mousewheel). Or press Enter while holding Ctrl.
   *     Opens the link in a new tab in the background .
   * * Press Ctrl+Shift and click a link. Or press Shift and click a link with
   *   your middle mouse button (or mousewheel). Or press Enter while holding
   *   Ctrl+Shift.
   *     Opens the link in a new tab and switches to the newly opened tab.
   * * Press Shift and click a link. Or press Enter while holding Shift.
   *     Opens the link in a new window.
   *
   * On Mac, uses Command instead of Ctrl.
   * For keyboard support you need to use keydown.
   *
   * @param {!LoadTimeData} localStrings The local strings object which is used
   *     to localize the warning prompt in case the user tries to open a lot of
   *     links.
   * @constructor
   */
  function LinkController(localStrings) {
    this.localStrings_ = localStrings;
  }

  LinkController.prototype = {
    /**
     * The number of links that can be opened before showing a warning confirm
     * message.
     */
    warningLimit: 15,

    /**
     * The DOM window that we want to open links into in case we are opening
     * links in the same window.
     * @type {!Window}
     */
    window: window,

    /**
     * This method is used for showing the warning confirm message when the
     * user is trying to open a lot of links.
     * @param {number} count The number of URLs to open.
     * @return {string} The message to show the user.
     */
    getWarningMessage: function(count) {
      return this.localStrings_.getStringF('should_open_all', String(count));
    },

    /**
     * Open an URL from a mouse or keyboard event.
     * @param {string} url The URL to open.
     * @param {!Event} e The event triggering the opening of the URL.
     */
    openUrlFromEvent: function(url, e) {
      // We only support keydown Enter and non right click events.
      if (e.type == 'keydown' && e.key == 'Enter' || e.button != 2) {
        var kind;
        var ctrl = cr.isMac && e.metaKey || !cr.isMac && e.ctrlKey;

        if (e.button == 1 || ctrl)  // middle, ctrl or keyboard
          kind = e.shiftKey ? cr.LinkKind.FOREGROUND_TAB :
                              cr.LinkKind.BACKGROUND_TAB;
        else  // left or keyboard
          kind = e.shiftKey ? cr.LinkKind.WINDOW : cr.LinkKind.SELF;

        this.openUrls([url], kind);
      }
    },


    /**
     * Opens a URL in a new tab, window or incognito window.
     * @param {string} url The URL to open.
     * @param {cr.LinkKind} kind The kind of open we want to do.
     */
    openUrl: function(url, kind) {
      this.openUrls([url], kind);
    },

    /**
     * Opens URLs in new tab, window or incognito mode.
     * @param {!Array<string>} urls The URLs to open.
     * @param {cr.LinkKind} kind The kind of open we want to do.
     */
    openUrls: function(urls, kind) {
      if (urls.length < 1)
        return;

      if (urls.length > this.warningLimit) {
        if (!this.window.confirm(this.getWarningMessage(urls.length)))
          return;
      }

      // Fix '#124' URLs since opening those in a new window does not work. We
      // prepend the base URL when we encounter those.
      var base = this.window.location.href.split('#')[0];
      urls = urls.map(function(url) {
        return url[0] == '#' ? base + url : url;
      });

      var incognito = kind == cr.LinkKind.INCOGNITO;
      if (kind == cr.LinkKind.WINDOW || incognito) {
        chrome.windows.create({url: urls, incognito: incognito});
      } else if (
          kind == cr.LinkKind.FOREGROUND_TAB ||
          kind == cr.LinkKind.BACKGROUND_TAB) {
        urls.forEach(function(url, i) {
          chrome.tabs.create(
              {url: url, selected: kind == cr.LinkKind.FOREGROUND_TAB && !i});
        });
      } else {
        this.window.location.href = urls[0];
      }
    }
  };

  // Export
  return {
    LinkController: LinkController,
  };
});
