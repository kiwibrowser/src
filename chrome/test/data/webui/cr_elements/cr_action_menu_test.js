// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for cr-action-menu element. Runs as an interactive UI
 * test, since many of these tests check focus behavior.
 */
suite('CrActionMenu', function() {
  /** @type {?CrActionMenuElement} */
  let menu = null;

  /** @type {?HTMLDialogElement} */
  let dialog = null;

  /** @type {?NodeList<HTMLElement>} */
  let items = null;

  /** @type {HTMLElement} */
  let dots = null;

  /** @type {HTMLElement} */
  let container = null;

  setup(function() {
    PolymerTest.clearBody();

    document.body.innerHTML = `
      <button id="dots">...</button>
      <cr-action-menu>
        <button slot="item" class="dropdown-item">Un</button>
        <hr slot="item">
        <button slot="item" class="dropdown-item">Dos</button>
        <button slot="item" class="dropdown-item">Tres</button>
      </cr-action-menu>
    `;

    menu = document.querySelector('cr-action-menu');
    dialog = menu.getDialog();
    items = menu.querySelectorAll('.dropdown-item');
    dots = document.querySelector('#dots');
    assertEquals(3, items.length);
  });

  teardown(function() {
    document.body.style.direction = 'ltr';

    if (dialog.open)
      menu.close();
  });

  function down() {
    MockInteractions.keyDownOn(menu, 'ArrowDown', [], 'ArrowDown');
  }

  function up() {
    MockInteractions.keyDownOn(menu, 'ArrowUp', [], 'ArrowUp');
  }

  test('hidden or disabled items', function() {
    menu.showAt(dots);
    down();
    assertEquals(menu.root.activeElement, items[0]);

    menu.close();
    items[0].hidden = true;
    menu.showAt(dots);
    down();
    assertEquals(menu.root.activeElement, items[1]);

    menu.close();
    items[1].disabled = true;
    menu.showAt(dots);
    down();
    assertEquals(menu.root.activeElement, items[2]);
  });

  test('focus after down/up arrow', function() {
    menu.showAt(dots);

    // The menu should be focused when shown, but not on any of the items.
    assertEquals(menu, document.activeElement);
    assertNotEquals(items[0], menu.root.activeElement);
    assertNotEquals(items[1], menu.root.activeElement);
    assertNotEquals(items[2], menu.root.activeElement);

    down();
    assertEquals(items[0], menu.root.activeElement);
    down();
    assertEquals(items[1], menu.root.activeElement);
    down();
    assertEquals(items[2], menu.root.activeElement);
    down();
    assertEquals(items[0], menu.root.activeElement);
    up();
    assertEquals(items[2], menu.root.activeElement);
    up();
    assertEquals(items[1], menu.root.activeElement);
    up();
    assertEquals(items[0], menu.root.activeElement);
    up();
    assertEquals(items[2], menu.root.activeElement);

    items[1].disabled = true;
    up();
    assertEquals(items[0], menu.root.activeElement);
  });

  test('pressing up arrow when no focus will focus last item', function() {
    menu.showAt(dots);
    assertEquals(menu, document.activeElement);

    up();
    assertEquals(items[items.length - 1], menu.root.activeElement);
  });

  test('can navigate to dynamically added items', function() {
    // Can modify children after attached() and before showAt().
    const item = document.createElement('button');
    item.classList.add('dropdown-item');
    item.setAttribute('slot', 'item');
    menu.insertBefore(item, items[0]);
    menu.showAt(dots);

    down();
    assertEquals(item, menu.root.activeElement);
    down();
    assertEquals(items[0], menu.root.activeElement);

    // Can modify children while menu is open.
    menu.removeChild(item);

    up();
    // Focus should have wrapped around to final item.
    assertEquals(items[2], menu.root.activeElement);
  });

  test('close on resize', function() {
    menu.showAt(dots);
    assertTrue(dialog.open);

    window.dispatchEvent(new CustomEvent('resize'));
    assertFalse(dialog.open);
  });

  test('close on popstate', function() {
    menu.showAt(dots);
    assertTrue(dialog.open);

    window.dispatchEvent(new CustomEvent('popstate'));
    assertFalse(dialog.open);
  });

  /** @param {string} key The key to use for closing. */
  function testFocusAfterClosing(key) {
    return new Promise(function(resolve) {
      menu.showAt(dots);
      assertTrue(dialog.open);

      // Check that focus returns to the anchor element.
      dots.addEventListener('focus', resolve);
      MockInteractions.keyDownOn(menu, key, [], key);
      assertFalse(dialog.open);
    });
  }

  test('close on Tab', function() {
    return testFocusAfterClosing('Tab');
  });
  test('close on Escape', function() {
    return testFocusAfterClosing('Escape');
  });

  test('mouse movement focus options', function() {
    function makeMouseoverEvent(node) {
      const e = new MouseEvent('mouseover', {bubbles: true});
      node.dispatchEvent(e);
    }

    menu.showAt(dots);

    // Moving mouse on option 1 should focus it.
    assertNotEquals(items[0], menu.root.activeElement);
    makeMouseoverEvent(items[0]);
    assertEquals(items[0], menu.root.activeElement);

    // Moving mouse on the menu (not on option) should focus the menu.
    makeMouseoverEvent(menu);
    assertNotEquals(items[0], menu.root.activeElement);
    assertEquals(menu, document.activeElement);

    // Moving mouse on a disabled item should focus the menu.
    items[2].setAttribute('disabled', '');
    makeMouseoverEvent(items[2]);
    assertNotEquals(items[2], menu.root.activeElement);
    assertEquals(menu, document.activeElement);

    // Mouse movements should override keyboard focus.
    down();
    down();
    assertEquals(items[1], menu.root.activeElement);
    makeMouseoverEvent(items[0]);
    assertEquals(items[0], menu.root.activeElement);
  });

  test('items automatically given accessibility role', function() {
    const newItem = document.createElement('button');
    newItem.setAttribute('slot', 'item');
    newItem.classList.add('dropdown-item');

    items[1].setAttribute('role', 'checkbox');
    menu.showAt(dots);

    return PolymerTest.flushTasks()
        .then(() => {
          assertEquals('menuitem', items[0].getAttribute('role'));
          assertEquals('checkbox', items[1].getAttribute('role'));

          menu.insertBefore(newItem, items[0]);
          return PolymerTest.flushTasks();
        })
        .then(() => {
          assertEquals('menuitem', newItem.getAttribute('role'));
        });
  });

  test('positioning', function() {
    // A 40x10 box at (100, 250).
    const config = {
      left: 100,
      top: 250,
      width: 40,
      height: 10,
      maxX: 1000,
      maxY: 2000,
    };

    // Show right and bottom aligned by default.
    menu.showAtPosition(config);
    assertTrue(dialog.open);
    assertEquals('100px', dialog.style.left);
    assertEquals('250px', dialog.style.top);
    menu.close();

    // Center the menu horizontally.
    menu.showAtPosition(Object.assign({}, config, {
      anchorAlignmentX: AnchorAlignment.CENTER,
    }));
    const menuWidth = dialog.offsetWidth;
    const menuHeight = dialog.offsetHeight;
    assertEquals(`${120 - menuWidth / 2}px`, dialog.style.left);
    assertEquals('250px', dialog.style.top);
    menu.close();

    // Center the menu in both axes.
    menu.showAtPosition(Object.assign({}, config, {
      anchorAlignmentX: AnchorAlignment.CENTER,
      anchorAlignmentY: AnchorAlignment.CENTER,
    }));
    assertEquals(`${120 - menuWidth / 2}px`, dialog.style.left);
    assertEquals(`${255 - menuHeight / 2}px`, dialog.style.top);
    menu.close();

    // Left and top align the menu.
    menu.showAtPosition(Object.assign({}, config, {
      anchorAlignmentX: AnchorAlignment.BEFORE_END,
      anchorAlignmentY: AnchorAlignment.BEFORE_END,
    }));
    assertEquals(`${140 - menuWidth}px`, dialog.style.left);
    assertEquals(`${260 - menuHeight}px`, dialog.style.top);
    menu.close();

    // Being left and top aligned at (0, 0) should anchor to the bottom right.
    menu.showAtPosition(Object.assign({}, config, {
      anchorAlignmentX: AnchorAlignment.BEFORE_END,
      anchorAlignmentY: AnchorAlignment.BEFORE_END,
      left: 0,
      top: 0,
    }));
    assertEquals(`0px`, dialog.style.left);
    assertEquals(`0px`, dialog.style.top);
    menu.close();

    // Being aligned to a point in the bottom right should anchor to the top
    // left.
    menu.showAtPosition({
      left: 1000,
      top: 2000,
      maxX: 1000,
      maxY: 2000,
    });
    assertEquals(`${1000 - menuWidth}px`, dialog.style.left);
    assertEquals(`${2000 - menuHeight}px`, dialog.style.top);
    menu.close();

    // If the viewport can't fit the menu, align the menu to the viewport.
    menu.showAtPosition({
      left: menuWidth - 5,
      top: 0,
      width: 0,
      height: 0,
      maxX: menuWidth * 2 - 10,
    });
    assertEquals(`${menuWidth - 10}px`, dialog.style.left);
    assertEquals(`0px`, dialog.style.top);
    menu.close();

    // Alignment is reversed in RTL.
    document.body.style.direction = 'rtl';
    menu.showAtPosition(config);
    assertTrue(dialog.open);
    assertEquals(140 - menuWidth, dialog.offsetLeft);
    assertEquals('250px', dialog.style.top);
    menu.close();
  });

  // TODO(scottchen): fix flakiness and re-enable this test.
  test.skip(
      '[auto-reposition] enables repositioning if content changes',
      function(done) {
        menu.autoReposition = true;

        dots.style.marginLeft = '800px';

        let dotsRect = dots.getBoundingClientRect();

        // Anchored at right-top by default.
        menu.showAt(dots);
        assertTrue(dialog.open);
        let menuRect = menu.getBoundingClientRect();
        assertEquals(
            Math.round(dotsRect.left + dotsRect.width),
            Math.round(menuRect.left + menuRect.width));
        assertEquals(dotsRect.top, menuRect.top);

        const lastMenuLeft = menuRect.left;
        const lastMenuWidth = menuRect.width;

        menu.addEventListener('cr-action-menu-repositioned', () => {
          assertTrue(dialog.open);
          menuRect = menu.getBoundingClientRect();
          // Test that menu width got larger.
          assertTrue(menuRect.width > lastMenuWidth);
          // Test that menu upper-left moved further left.
          assertTrue(menuRect.left < lastMenuLeft);
          // Test that right and top did not move since it is anchored there.
          assertEquals(
              Math.round(dotsRect.left + dotsRect.width),
              Math.round(menuRect.left + menuRect.width));
          assertEquals(dotsRect.top, menuRect.top);
          done();
        });

        // Still anchored at the right place after content size changes.
        items[0].textContent = 'this is a long string to make menu wide';
      });

  suite('offscreen scroll positioning', function() {
    const bodyHeight = 10000;
    const bodyWidth = 20000;
    const containerLeft = 5000;
    const containerTop = 10000;
    const containerWidth = 500;
    const containerHeight = 500;
    const menuWidth = 100;
    const menuHeight = 200;

    suiteSetup(function() {
      document.body.innerHTML = `
        <dom-module id="test-element">
          <template>
            <style>
              #container {
                overflow: auto;
                position: absolute;
                top: ${containerTop}px;
                left: ${containerLeft}px;
                right: ${containerLeft}px;
                height: ${containerHeight}px;
                width: ${containerWidth}px;
              }

              #inner-container {
                height: 1000px;
                width: 1000px;
              }

              cr-action-menu {
                --cr-action-menu-dialog: {
                  height: ${menuHeight}px;
                  width: ${menuWidth}px;
                  padding: 0;
                };
              }
            </style>
            <div id="container">
              <div id="inner-container">
                <button id="dots">...</button>
                <cr-action-menu>
                  <button slot="item" class="dropdown-item">Un</button>
                  <hr>
                  <button slot="item" class="dropdown-item">Dos</button>
                  <button slot="item" class="dropdown-item">Tres</button>
                </cr-action-menu>
              </div>
            </div>
          </template>
        </dom-module>
      `;

      Polymer({
        is: 'test-element',
      });
    });

    setup(function() {
      document.body.scrollTop = 0;
      document.body.scrollLeft = 0;
      document.body.innerHTML = `
        <style>
          test-element {
            height: ${bodyHeight}px;
            width: ${bodyWidth}px;
          }
        </style>
        <test-element></test-element>`;

      testElement = document.querySelector('test-element');
      menu = testElement.root.querySelector('cr-action-menu');
      dialog = menu.getDialog();
      dots = testElement.root.querySelector('#dots');
      container = testElement.root.querySelector('#container');
    });

    // Show the menu, scrolling the body to the button.
    test('simple offscreen', function() {
      menu.showAt(dots, {anchorAlignmentX: AnchorAlignment.AFTER_START});
      assertEquals(`${containerLeft}px`, dialog.style.left);
      assertEquals(`${containerTop}px`, dialog.style.top);
      menu.close();
    });

    // Show the menu, scrolling the container to the button, and the body to the
    // button.
    test('offscreen and out of scroll container viewport', function() {
      document.body.scrollLeft = bodyWidth;
      document.body.scrollTop = bodyHeight;

      container.scrollLeft = containerLeft;
      container.scrollTop = containerTop;

      menu.showAt(dots, {anchorAlignmentX: AnchorAlignment.AFTER_START});
      assertEquals(`${containerLeft}px`, dialog.style.left);
      assertEquals(`${containerTop}px`, dialog.style.top);
      menu.close();
    });

    // Show the menu for an already onscreen button. The anchor should be
    // overridden so that no scrolling happens.
    test('onscreen forces anchor change', function() {
      const rect = dots.getBoundingClientRect();
      document.body.scrollLeft = rect.right - document.body.clientWidth + 10;
      document.body.scrollTop = rect.bottom - document.body.clientHeight + 10;

      menu.showAt(dots, {anchorAlignmentX: AnchorAlignment.AFTER_START});
      const buttonWidth = dots.offsetWidth;
      const buttonHeight = dots.offsetHeight;
      assertEquals(containerLeft - menuWidth + buttonWidth, dialog.offsetLeft);
      assertEquals(containerTop - menuHeight + buttonHeight, dialog.offsetTop);
      menu.close();
    });

    test('scroll position maintained for showAtPosition', function() {
      document.body.scrollLeft = 500;
      document.body.scrollTop = 1000;
      menu.showAtPosition({top: 50, left: 50});
      assertEquals(550, dialog.offsetLeft);
      assertEquals(1050, dialog.offsetTop);
      menu.close();
    });

    test('rtl', function() {
      // Anchor to an item in RTL.
      document.body.style.direction = 'rtl';
      menu.showAt(dots, {anchorAlignmentX: AnchorAlignment.AFTER_START});
      assertEquals(
          container.offsetLeft + containerWidth - menuWidth, dialog.offsetLeft);
      assertEquals(containerTop, dialog.offsetTop);
      menu.close();
    });
  });
});
