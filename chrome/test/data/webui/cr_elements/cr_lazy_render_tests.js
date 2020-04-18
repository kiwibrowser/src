// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('cr-lazy-render', function() {
  let lazy;
  let bind;

  suiteSetup(function() {
    return PolymerTest.importHtml(
        'chrome://resources/cr_elements/cr_checkbox/cr_checkbox.html');
  });

  setup(function() {
    PolymerTest.clearBody();
    const template = `
        <template is="dom-bind" id="bind">
          <cr-lazy-render id="lazy">
            <template>
              <h1>
                <cr-checkbox checked="{{checked}}"></cr-checkbox>
                {{name}}
              </h1>
            </template>
          </cr-lazy-render>
        </template>`;
    document.body.innerHTML = template;
    lazy = document.getElementById('lazy');
    bind = document.getElementById('bind');
  });

  test('stamps after get()', function() {
    assertFalse(!!document.body.querySelector('h1'));
    assertFalse(!!lazy.getIfExists());

    const inner = lazy.get();
    assertEquals('H1', inner.nodeName);
    assertEquals(inner, document.body.querySelector('h1'));
  });

  test('one-way binding works', function() {
    bind.name = 'Wings';

    const inner = lazy.get();
    assertNotEquals(-1, inner.textContent.indexOf('Wings'));
    bind.name = 'DC';
    assertNotEquals(-1, inner.textContent.indexOf('DC'));
  });

  test('two-way binding works', function() {
    bind.checked = true;

    const inner = lazy.get();
    const checkbox = document.querySelector('cr-checkbox');
    assertTrue(checkbox.checked);
    MockInteractions.tap(checkbox);
    assertFalse(checkbox.checked);
    assertFalse(bind.checked);
  });
});
