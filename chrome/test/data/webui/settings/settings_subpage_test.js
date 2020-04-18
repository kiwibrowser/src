// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_subpage', function() {
  suite('SettingsSubpage', function() {
    setup(function() {
      PolymerTest.clearBody();
    });

    test('clear search (event)', function() {
      const subpage = document.createElement('settings-subpage');
      // Having a searchLabel will create the settings-subpage-search.
      subpage.searchLabel = 'test';
      document.body.appendChild(subpage);
      Polymer.dom.flush();
      const search = subpage.$$('settings-subpage-search');
      assertTrue(!!search);
      search.setValue('Hello');
      subpage.fire('clear-subpage-search');
      Polymer.dom.flush();
      assertEquals('', search.getValue());
    });

    test('clear search (click)', function() {
      const subpage = document.createElement('settings-subpage');
      // Having a searchLabel will create the settings-subpage-search.
      subpage.searchLabel = 'test';
      document.body.appendChild(subpage);
      Polymer.dom.flush();
      const search = subpage.$$('settings-subpage-search');
      assertTrue(!!search);
      search.setValue('Hello');
      assertEquals(null, search.root.activeElement);
      search.$.clearSearch.click();
      Polymer.dom.flush();
      assertEquals('', search.getValue());
      assertEquals(search.$.searchInput, search.root.activeElement);
    });

    test('navigates to parent when there is no history', function() {
      // Pretend that we initially started on the CERTIFICATES route.
      window.history.replaceState(
          undefined, '', settings.routes.CERTIFICATES.path);
      settings.initializeRouteFromUrl();
      assertEquals(settings.routes.CERTIFICATES, settings.getCurrentRoute());

      const subpage = document.createElement('settings-subpage');
      document.body.appendChild(subpage);

      MockInteractions.tap(subpage.$$('button'));
      assertEquals(settings.routes.PRIVACY, settings.getCurrentRoute());
    });

    test('navigates to any route via window.back()', function(done) {
      settings.navigateTo(settings.routes.BASIC);
      settings.navigateTo(settings.routes.SYNC);
      assertEquals(settings.routes.SYNC, settings.getCurrentRoute());

      const subpage = document.createElement('settings-subpage');
      document.body.appendChild(subpage);

      MockInteractions.tap(subpage.$$('button'));

      window.addEventListener('popstate', function(event) {
        assertEquals(settings.routes.BASIC, settings.getCurrentRoute());
        done();
      });
    });
  });

  suite('SettingsSubpageSearch', function() {
    test('host autofocus propagates to <input>', function() {
      PolymerTest.clearBody();
      const element = document.createElement('settings-subpage-search');
      element.setAttribute('autofocus', true);
      document.body.appendChild(element);

      assertTrue(element.$$('input').hasAttribute('autofocus'));

      element.removeAttribute('autofocus');
      assertFalse(element.$$('input').hasAttribute('autofocus'));
    });
  });
});
