// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('extensions', function() {
  /** @interface */
  class ItemDelegate {
    /** @param {string} id */
    deleteItem(id) {}

    /**
     * @param {string} id
     * @param {boolean} isEnabled
     */
    setItemEnabled(id, isEnabled) {}

    /**
     * @param {string} id
     * @param {boolean} isAllowedIncognito
     */
    setItemAllowedIncognito(id, isAllowedIncognito) {}

    /**
     * @param {string} id
     * @param {boolean} isAllowedOnFileUrls
     */
    setItemAllowedOnFileUrls(id, isAllowedOnFileUrls) {}

    /**
     * @param {string} id
     * @param {boolean} isAllowedOnAllSites
     */
    setItemAllowedOnAllSites(id, isAllowedOnAllSites) {}

    /**
     * @param {string} id
     * @param {boolean} collectsErrors
     */
    setItemCollectsErrors(id, collectsErrors) {}

    /**
     * @param {string} id
     * @param {chrome.developerPrivate.ExtensionView} view
     */
    inspectItemView(id, view) {}

    /**
     * @param {string} url
     */
    openUrl(url) {}

    /**
     * @param {string} id
     * @return {!Promise}
     */
    reloadItem(id) {}

    /** @param {string} id */
    repairItem(id) {}

    /** @param {!chrome.developerPrivate.ExtensionInfo} extension */
    showItemOptionsPage(extension) {}

    /** @param {string} id */
    showInFolder(id) {}

    /**
     * @param {string} id
     * @return {!Promise<string>}
     */
    getExtensionSize(id) {}
  }

  const Item = Polymer({
    is: 'extensions-item',

    behaviors: [I18nBehavior, extensions.ItemBehavior],

    properties: {
      // The item's delegate, or null.
      delegate: {
        type: Object,
      },

      // Whether or not dev mode is enabled.
      inDevMode: {
        type: Boolean,
        value: false,
      },

      // The underlying ExtensionInfo itself. Public for use in declarative
      // bindings.
      /** @type {chrome.developerPrivate.ExtensionInfo} */
      data: {
        type: Object,
      },

      // Whether or not the expanded view of the item is shown.
      /** @private */
      showingDetails_: {
        type: Boolean,
        value: false,
      },
    },

    observers: [
      'observeIdVisibility_(inDevMode, showingDetails_, data.id)',
    ],

    /** @private string */
    a11yAssociation_: function() {
      // Don't use I18nBehavior.i18n because of additional checks it performs.
      // Polymer ensures that this string is not stamped into arbitrary HTML.
      // |this.data.name| can contain any data including html tags.
      // ex: "My <video> download extension!"
      return loadTimeData.getStringF(
          'extensionA11yAssociation', this.data.name);
    },

    /** @private */
    observeIdVisibility_: function(inDevMode, showingDetails, id) {
      Polymer.dom.flush();
      const idElement = this.$$('#extension-id');
      if (idElement) {
        assert(this.data);
        idElement.innerHTML = this.i18n('itemId', this.data.id);
      }
    },

    /**
     * @return {boolean}
     * @private
     */
    shouldShowErrorsButton_: function() {
      // When the error console is disabled (happens when
      // --disable-error-console command line flag is used or when in the
      // Stable/Beta channel), |installWarnings| is populated.
      if (this.data.installWarnings && this.data.installWarnings.length > 0)
        return true;

      // When error console is enabled |installedWarnings| is not populated.
      // Instead |manifestErrors| and |runtimeErrors| are used.
      return this.data.manifestErrors.length > 0 ||
          this.data.runtimeErrors.length > 0;
    },

    /** @private */
    onRemoveTap_: function() {
      this.delegate.deleteItem(this.data.id);
    },

    /** @private */
    onEnableChange_: function() {
      this.delegate.setItemEnabled(
          this.data.id, this.$['enable-toggle'].checked);
    },

    /** @private */
    onErrorsTap_: function() {
      if (this.data.installWarnings && this.data.installWarnings.length > 0) {
        this.fire('show-install-warnings', this.data.installWarnings);
        return;
      }

      extensions.navigation.navigateTo(
          {page: Page.ERRORS, extensionId: this.data.id});
    },

    /** @private */
    onDetailsTap_: function() {
      extensions.navigation.navigateTo(
          {page: Page.DETAILS, extensionId: this.data.id});
    },

    /**
     * @param {!{model: !{item: !chrome.developerPrivate.ExtensionView}}} e
     * @private
     */
    onInspectTap_: function(e) {
      this.delegate.inspectItemView(this.data.id, this.data.views[0]);
    },

    /** @private */
    onExtraInspectTap_: function() {
      extensions.navigation.navigateTo(
          {page: Page.DETAILS, extensionId: this.data.id});
    },

    /** @private */
    onReloadTap_: function() {
      this.delegate.reloadItem(this.data.id).catch(loadError => {
        this.fire('load-error', loadError);
      });
    },

    /** @private */
    onRepairTap_: function() {
      this.delegate.repairItem(this.data.id);
    },

    /**
     * @return {boolean}
     * @private
     */
    isControlled_: function() {
      return extensions.isControlled(this.data);
    },

    /**
     * @return {boolean}
     * @private
     */
    isEnabled_: function() {
      return extensions.isEnabled(this.data.state);
    },

    /**
     * @return {boolean}
     * @private
     */
    isEnableToggleEnabled_: function() {
      return extensions.userCanChangeEnablement(this.data);
    },

    /**
     * Returns true if the enable toggle should be shown.
     * @return {boolean}
     * @private
     */
    showEnableToggle_: function() {
      return !this.isTerminated_() && !this.data.disableReasons.corruptInstall;
    },

    /**
     * Returns true if the extension is in the terminated state.
     * @return {boolean}
     * @private
     */
    isTerminated_: function() {
      return this.data.state ==
          chrome.developerPrivate.ExtensionState.TERMINATED;
    },

    /**
     * return {string}
     * @private
     */
    computeClasses_: function() {
      let classes = this.isEnabled_() ? 'enabled' : 'disabled';
      if (this.inDevMode)
        classes += ' dev-mode';
      return classes;
    },

    /**
     * @return {string}
     * @private
     */
    computeSourceIndicatorIcon_: function() {
      switch (extensions.getItemSource(this.data)) {
        case SourceType.POLICY:
          return 'communication:business';
        case SourceType.SIDELOADED:
          return 'input';
        case SourceType.UNKNOWN:
          // TODO(dpapad): Ask UX for a better icon for this case.
          return 'input';
        case SourceType.UNPACKED:
          return 'extensions-icons:unpacked';
        case SourceType.WEBSTORE:
          return '';
      }
      assertNotReached();
    },

    /**
     * @return {string}
     * @private
     */
    computeSourceIndicatorText_: function() {
      if (this.data.locationText)
        return this.data.locationText;

      const sourceType = extensions.getItemSource(this.data);
      return sourceType == SourceType.WEBSTORE ?
          '' :
          extensions.getItemSourceString(sourceType);
    },

    /**
     * @return {boolean}
     * @private
     */
    computeInspectViewsHidden_: function() {
      return !this.data.views || this.data.views.length == 0;
    },

    /**
     * @return {string}
     * @private
     */
    computeFirstInspectTitle_: function() {
      // Note: theoretically, this wouldn't be called without any inspectable
      // views (because it's in a dom-if="!computeInspectViewsHidden_()").
      // However, due to the recycling behavior of iron list, it seems that
      // sometimes it can. Even when it is, the UI behaves properly, but we
      // need to handle the case gracefully.
      return this.data.views.length > 0 ?
          extensions.computeInspectableViewLabel(this.data.views[0]) :
          '';
    },

    /**
     * @return {string}
     * @private
     */
    computeFirstInspectLabel_: function() {
      let label = this.computeFirstInspectTitle_();
      return label && this.data.views.length > 1 ? label + ',' : label;
    },

    /**
     * @return {boolean}
     * @private
     */
    computeExtraViewsHidden_: function() {
      return this.data.views.length <= 1;
    },

    /**
     * @return {boolean}
     * @private
     */
    computeDevReloadButtonHidden_: function() {
      // Only display the reload spinner if the extension is unpacked and
      // enabled. There's no point in reloading a disabled extension, and we'll
      // show a crashed reload buton if it's terminated.
      const showIcon =
          this.data.location == chrome.developerPrivate.Location.UNPACKED &&
          this.data.state == chrome.developerPrivate.ExtensionState.ENABLED;
      return !showIcon;
    },

    /**
     * @return {string}
     * @private
     */
    computeExtraInspectLabel_: function() {
      return this.i18n(
          'itemInspectViewsExtra', (this.data.views.length - 1).toString());
    },

    /**
     * @return {boolean}
     * @private
     */
    hasWarnings_: function() {
      return this.data.disableReasons.corruptInstall ||
          this.data.disableReasons.suspiciousInstall ||
          this.data.runtimeWarnings.length > 0 || !!this.data.blacklistText;
    },

    /**
     * @return {string}
     * @private
     */
    computeWarningsClasses_: function() {
      return this.data.blacklistText ? 'severe' : 'mild';
    },
  });

  return {
    Item: Item,
    ItemDelegate: ItemDelegate,
  };
});
