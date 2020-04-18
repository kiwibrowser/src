// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-animated-pages' is a container for a page and animated subpages.
 * It provides a set of common behaviors and animations.
 *
 * Example:
 *
 *    <settings-animated-pages section="privacy">
 *      <!-- Insert your section controls here -->
 *    </settings-animated-pages>
 */

Polymer({
  is: 'settings-animated-pages',

  behaviors: [settings.RouteObserverBehavior],

  properties: {
    /**
     * Routes with this section activate this element. For instance, if this
     * property is 'search', and currentRoute.section is also set to 'search',
     * this element will display the subpage in currentRoute.subpage.
     *
     * The section name must match the name specified in route.js.
     */
    section: String,

    /**
     * A Map specifying which element should be focused when exiting a subpage.
     * The key of the map holds a settings.Route path, and the value holds a
     * query selector that identifies the desired element.
     * @type {?Map<string, string>}
     */
    focusConfig: Object,
  },

  listeners: {
    'neon-animation-finish': 'onNeonAnimationFinish_',
  },

  /**
   * The last "previous" route reported by the router.
   * @private {?settings.Route}
   */
  previousRoute_: null,

  /** @override */
  created: function() {
    // Observe the light DOM so we know when it's ready.
    this.lightDomObserver_ =
        Polymer.dom(this).observeNodes(this.lightDomChanged_.bind(this));
  },

  /** @private */
  onNeonAnimationFinish_: function() {
    if (settings.lastRouteChangeWasPopstate())
      return;

    // Set initial focus when navigating to a subpage for a11y.
    let subPage = /** @type {SettingsSubpageElement} */ (
        this.querySelector('settings-subpage.iron-selected'));
    if (subPage)
      subPage.initialFocus();
  },

  /**
   * @param {!Event} e
   * @private
   */
  onIronSelect_: function(e) {
    if (!this.focusConfig || !this.previousRoute_)
      return;

    // Don't attempt to focus any anchor element, unless last navigation was a
    // 'pop' (backwards) navigation.
    if (!settings.lastRouteChangeWasPopstate())
      return;

    // Only handle iron-select events from neon-animatable elements and the
    // given whitelist of settings-subpage instances.
    let whitelist = 'settings-subpage#site-settings';

    if (settings.routes.SITE_SETTINGS_COOKIES) {
      whitelist += ', settings-subpage[route-path=\"' +
          settings.routes.SITE_SETTINGS_COOKIES.path + '\"]';
    }

    // <if expr="chromeos">
    if (settings.routes.INTERNET_NETWORKS) {
      whitelist += ', settings-subpage[route-path=\"' +
          settings.routes.INTERNET_NETWORKS.path + '\"]';
    }
    // </if>

    if (!e.detail.item.matches('neon-animatable, ' + whitelist))
      return;

    const selector = this.focusConfig.get(this.previousRoute_.path);
    if (selector) {
      // neon-animatable has "display: none" until the animation finishes, so
      // calling focus() on any of its children has no effect until "display:
      // none" is removed. Therefore, don't set focus from within the
      // currentRouteChanged callback. Using 'iron-select' listener which fires
      // after the animation has finished allows setting focus to work.
      cr.ui.focusWithoutInk(assert(this.querySelector(selector)));
    }
  },

  /**
   * Called initially once the effective children are ready.
   * @private
   */
  lightDomChanged_: function() {
    if (this.lightDomReady_)
      return;

    this.lightDomReady_ = true;
    Polymer.dom(this).unobserveNodes(this.lightDomObserver_);
    this.runQueuedRouteChange_();
  },

  /**
   * Calls currentRouteChanged with the deferred route change info.
   * @private
   */
  runQueuedRouteChange_: function() {
    if (!this.queuedRouteChange_)
      return;
    this.async(this.currentRouteChanged.bind(
        this, this.queuedRouteChange_.newRoute,
        this.queuedRouteChange_.oldRoute));
  },

  /** @protected */
  currentRouteChanged: function(newRoute, oldRoute) {
    this.previousRoute_ = oldRoute;

    if (newRoute.section == this.section && newRoute.isSubpage()) {
      this.switchToSubpage_(newRoute, oldRoute);
    } else {
      this.$.animatedPages.exitAnimation = 'settings-fade-out-animation';
      this.$.animatedPages.entryAnimation = 'settings-fade-in-animation';
      this.$.animatedPages.selected = 'default';
    }
  },

  /**
   * Selects the subpage specified by |newRoute|.
   * @param {!settings.Route} newRoute
   * @param {!settings.Route} oldRoute
   * @private
   */
  switchToSubpage_: function(newRoute, oldRoute) {
    // Don't manipulate the light DOM until it's ready.
    if (!this.lightDomReady_) {
      this.queuedRouteChange_ = this.queuedRouteChange_ || {oldRoute: oldRoute};
      this.queuedRouteChange_.newRoute = newRoute;
      return;
    }

    this.ensureSubpageInstance_();

    if (oldRoute) {
      if (oldRoute.isSubpage() && newRoute.depth > oldRoute.depth) {
        const isRtl = loadTimeData.getString('textdirection') == 'rtl';
        const exit = isRtl ? 'right' : 'left';
        const entry = isRtl ? 'left' : 'right';
        this.$.animatedPages.exitAnimation = 'slide-' + exit + '-animation';
        this.$.animatedPages.entryAnimation =
            'slide-from-' + entry + '-animation';
      } else if (oldRoute.depth > newRoute.depth) {
        const isRtl = loadTimeData.getString('textdirection') == 'rtl';
        const exit = isRtl ? 'left' : 'right';
        const entry = isRtl ? 'right' : 'left';
        this.$.animatedPages.exitAnimation = 'slide-' + exit + '-animation';
        this.$.animatedPages.entryAnimation =
            'slide-from-' + entry + '-animation';
      } else {
        // The old route is not a subpage or is at the same level, so just fade.
        this.$.animatedPages.exitAnimation = 'settings-fade-out-animation';
        this.$.animatedPages.entryAnimation = 'settings-fade-in-animation';

        if (!oldRoute.isSubpage()) {
          // Set the height the expand animation should start at before
          // beginning the transition to the new subpage.
          // TODO(michaelpg): Remove MainPageBehavior's dependency on this
          // height being set.
          this.style.height = this.clientHeight + 'px';
          this.async(function() {
            this.style.height = '';
          });
        }
      }
    }

    this.$.animatedPages.selected = newRoute.path;
  },

  /**
   * Ensures that the template enclosing the subpage is stamped.
   * @private
   */
  ensureSubpageInstance_: function() {
    const routePath = settings.getCurrentRoute().path;
    const template = Polymer.dom(this).querySelector(
        'template[route-path="' + routePath + '"]');

    // Nothing to do if the subpage isn't wrapped in a <template> or the
    // template is already stamped.
    if (!template || template.if)
      return;

    // Set the subpage's id for use by neon-animated-pages.
    const subpage = /** @type {{_content: DocumentFragment}} */ (template)
                        ._content.querySelector('settings-subpage');
    subpage.setAttribute('route-path', routePath);

    // Carry over the 'no-search' attribute from the template to the stamped
    // instance, such that the stamped instance will also be ignored by the
    // searching algorithm.
    if (template.hasAttribute('no-search'))
      subpage.setAttribute('no-search', '');

    // Render synchronously so neon-animated-pages can select the subpage.
    template.if = true;
    template.render();
  },
});
