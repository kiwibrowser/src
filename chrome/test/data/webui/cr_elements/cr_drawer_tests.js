// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('cr-drawer', function() {
  setup(function() {
    PolymerTest.clearBody();
  });

  function createDrawer(align) {
    document.body.innerHTML = `
      <cr-drawer id="drawer" align="${align}">
        <div class="drawer-header">Test</div>
        <div class="drawer-content">Test content</div>
      </cr-drawer>
    `;
    Polymer.dom.flush();
    return document.getElementById('drawer');
  }

  test('open and close', function() {
    const drawer = createDrawer('ltr');
    drawer.openDrawer();

    return test_util.eventToPromise('transitionend', drawer)
        .then(() => {
          assertTrue(drawer.open);

          // Clicking the content does not close the drawer.
          MockInteractions.tap(document.querySelector('.drawer-content'));
          assertFalse(drawer.classList.contains('closing'));

          const whenClosed = test_util.eventToPromise('close', drawer);
          drawer.$.dialog.dispatchEvent(new MouseEvent('click', {
            bubbles: true,
            cancelable: true,
            clientX: 300,  // Must be larger than the drawer width (256px).
            clientY: 300,
          }));

          return whenClosed;
        })
        .then(() => {
          assertFalse(drawer.open);
        });
  });

  test('opened event', function() {
    const drawer = createDrawer('ltr');
    const whenOpen = test_util.eventToPromise('cr-drawer-opened', drawer);
    drawer.openDrawer();
    return whenOpen;
  });

  test('align=ltr', function() {
    createDrawer('ltr').openDrawer();
    return test_util.eventToPromise('transitionend', drawer).then(() => {
      const rect = drawer.$.dialog.getBoundingClientRect();
      assertEquals(0, rect.left);
      assertNotEquals(0, rect.right);
    });
  });

  test('align=rtl', function() {
    createDrawer('rtl').openDrawer();
    return test_util.eventToPromise('transitionend', drawer).then(() => {
      const rect = drawer.$.dialog.getBoundingClientRect();
      assertNotEquals(0, rect.left);
      assertEquals(window.innerWidth, rect.right);
    });
  });
});
