// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Kiosk apps menu implementation.
 */

cr.define('login', function() {
  'use strict';

  var Menu = cr.ui.Menu;
  var MenuButton = cr.ui.MenuButton;

  /**
   * Creates apps menu button.
   * @constructor
   * @extends {cr.ui.MenuButton}
   */
  var AppsMenuButton = cr.ui.define('button');

  AppsMenuButton.prototype = {
    __proto__: MenuButton.prototype,

    /**
     * Flag of whether to rebuild the menu.
     * @type {boolean}
     * @private
     */
    needsRebuild_: true,

    /**
     * Array to hold apps info.
     * @type {Array}
     */
    data_: null,
    get data() {
      return this.data_;
    },
    set data(data) {
      this.data_ = data;
      this.needsRebuild_ = true;
    },

    /** @override */
    decorate: function() {
      MenuButton.prototype.decorate.call(this);
      this.menu = new Menu;
      cr.ui.decorate(this.menu, Menu);
      document.body.appendChild(this.menu);

      this.anchorType = cr.ui.AnchorType.ABOVE;
      chrome.send('initializeKioskApps');
    },

    /** @override */
    showMenu: function(shouldSetFocus) {
      if (this.needsRebuild_) {
        this.menu.textContent = '';
        this.data_.forEach(this.addItem_, this);
        this.needsRebuild_ = false;
      }

      if (this.data.length > 0)
        MenuButton.prototype.showMenu.apply(this, arguments);
    },

    /**
     * Invoked when apps menu becomes visible.
     */
    didShow: function() {
      window.setTimeout(function() {
        if (!$('apps-header-bar-item').hidden)
          chrome.send('checkKioskAppLaunchError');
      }, 500);
    },

    findAndRunAppForTesting: function(id, opt_diagnostic_mode) {
      for (var i = 0; i < this.data.length; i++) {
        if (this.data[i].id == id) {
          this.launchApp_(this.data[i], !!opt_diagnostic_mode);
          break;
        }
      }
    },

    /**
     * Launch the app. If |diagnosticMode| is true, ask user to confirm.
     * @param {Object} app App data.
     * @param {boolean} diagnosticMode Whether to run the app in diagnostic
     *     mode.
     */
    launchApp_: function(app, diagnosticMode) {
      if (app.isAndroidApp) {
        chrome.send('launchArcKioskApp', [app.account_email]);
        return;
      }
      if (!diagnosticMode) {
        chrome.send('launchKioskApp', [app.id, false]);
        return;
      }

      if (!this.confirmDiagnosticMode_) {
        this.confirmDiagnosticMode_ =
            new cr.ui.dialogs.ConfirmDialog(document.body);
        this.confirmDiagnosticMode_.setOkLabel(
            loadTimeData.getString('confirmKioskAppDiagnosticModeYes'));
        this.confirmDiagnosticMode_.setCancelLabel(
            loadTimeData.getString('confirmKioskAppDiagnosticModeNo'));
      }

      this.confirmDiagnosticMode_.show(
          loadTimeData.getStringF(
              'confirmKioskAppDiagnosticModeFormat', app.label),
          function() {
            chrome.send('launchKioskApp', [app.id, true]);
          });
    },

    /**
     * Adds an app to the menu.
     * @param {Object} app An app info object.
     * @private
     */
    addItem_: function(app) {
      var menuItem = this.menu.addMenuItem(app);
      menuItem.classList.add('apps-menu-item');
      menuItem.addEventListener('activate', function(e) {
        var diagnosticMode = e.originalEvent && e.originalEvent.ctrlKey;
        this.launchApp_(app, diagnosticMode);
      }.bind(this));
    }
  };

  /**
   * Sets apps to be displayed in the apps menu.
   * @param {!Array<!Object>} apps An array of app info objects.
   */
  AppsMenuButton.setApps = function(apps) {
    $('show-apps-button').data = apps;
    $('login-header-bar').hasApps =
        apps.length > 0 || loadTimeData.getBoolean('kioskAppHasLaunchError');
    chrome.send('kioskAppsLoaded');
  };

  /**
   * Shows the given error message.
   * @param {!string} message Error message to show.
   */
  AppsMenuButton.showError = function(message) {
    /** @const */ var BUBBLE_OFFSET = 25;
    /** @const */ var BUBBLE_PADDING = 12;
    $('bubble').showTextForElement(
        $('show-apps-button'), message, cr.ui.Bubble.Attachment.TOP,
        BUBBLE_OFFSET, BUBBLE_PADDING);
  };


  /**
   * Runs app with a given id from the list of loaded apps.
   * @param {!string} id of an app to run.
   * @param {boolean=} opt_diagnostic_mode Whether to run the app in diagnostic
   *     mode.  Default is false.
   */
  AppsMenuButton.runAppForTesting = function(id, opt_diagnostic_mode) {
    $('show-apps-button').findAndRunAppForTesting(id, opt_diagnostic_mode);
  };

  return {AppsMenuButton: AppsMenuButton};
});
