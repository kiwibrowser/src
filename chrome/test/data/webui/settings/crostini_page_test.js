// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @type {?SettingsCrostiniPageElement} */
let crostiniPage = null;

/** @type {?TestCrostiniBrowserProxy} */
let crostiniBrowserProxy = null;

const setCrostiniEnabledValue = function(newValue) {
  crostiniPage.prefs = {crostini: {enabled: {value: newValue}}};
  Polymer.dom.flush();
};

suite('CrostiniPageTests', function() {
  setup(function() {
    crostiniBrowserProxy = new TestCrostiniBrowserProxy();
    settings.CrostiniBrowserProxyImpl.instance_ = crostiniBrowserProxy;
    PolymerTest.clearBody();
    crostiniPage = document.createElement('settings-crostini-page');
    document.body.appendChild(crostiniPage);
    testing.Test.disableAnimationsAndTransitions();
  });

  teardown(function() {
    crostiniPage.remove();
  });

  suite('Main Page', function() {
    setup(function() {
      setCrostiniEnabledValue(false);
    });

    test('Enable', function() {
      const button = crostiniPage.$$('#enable');
      assertTrue(!!button);
      assertFalse(!!crostiniPage.$$('.subpage-arrow'));

      MockInteractions.tap(button);
      Polymer.dom.flush();
      setCrostiniEnabledValue(
          crostiniBrowserProxy.prefs.crostini.enabled.value);
      assertTrue(crostiniPage.prefs.crostini.enabled.value);

      assertTrue(!!crostiniPage.$$('.subpage-arrow'));
    });
  });

  suite('SubPage', function() {
    let subpage;

    function flushAsync() {
      Polymer.dom.flush();
      return new Promise(resolve => {
        crostiniPage.async(resolve);
      });
    }

    /**
     * Returns a new promise that resolves after a window 'popstate' event.
     * @return {!Promise}
     */
    function whenPopState() {
      return new Promise(function(resolve) {
        window.addEventListener('popstate', function callback() {
          window.removeEventListener('popstate', callback);
          resolve();
        });
      });
    }

    setup(function() {
      setCrostiniEnabledValue(true);
      settings.navigateTo(settings.routes.CROSTINI);
      MockInteractions.tap(crostiniPage.$$('#crostini'));
      return flushAsync().then(() => {
        subpage = crostiniPage.$$('settings-crostini-subpage');
        assertTrue(!!subpage);
      });
    });

    test('Sanity', function() {
      assertTrue(!!subpage.$$('#remove'));
    });

    test('Remove', function() {
      assertTrue(!!subpage.$$('.subpage-arrow'));
      MockInteractions.tap(subpage.$$('.subpage-arrow'));
      setCrostiniEnabledValue(
          crostiniBrowserProxy.prefs.crostini.enabled.value);
      assertFalse(crostiniPage.prefs.crostini.enabled.value);
      return whenPopState().then(function() {
        assertEquals(settings.getCurrentRoute(), settings.routes.CROSTINI);
        assertTrue(!!crostiniPage.$$('#enable'));
      });
    });

    test('HideOnDisable', function() {
      assertEquals(
          settings.getCurrentRoute(), settings.routes.CROSTINI_DETAILS);
      setCrostiniEnabledValue(false);
      return whenPopState().then(function() {
        assertEquals(settings.getCurrentRoute(), settings.routes.CROSTINI);
      });
    });
  });
});
