// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

QUnit.module('l10n', {
  beforeEach: function() {
    sinon.stub(chrome.i18n, 'getMessage');
  },
  afterEach: function() {
    $testStub(chrome.i18n.getMessage).restore();
  }
});

QUnit.test('getTranslationOrError(tag) should return tag on error',
    function(assert) {
  var translation = l10n.getTranslationOrError('non_existent_tag');
  assert.equal(translation, 'non_existent_tag');
});

QUnit.test('localizeElementFromTag() should replace innerText by default',
  function(assert) {
    var element = document.createElement('div');
    $testStub(chrome.i18n.getMessage).withArgs('tag')
          .returns('<b>Hello World</b>');

    l10n.localizeElementFromTag(element, 'tag');

    assert.equal(element.innerHTML, '&lt;b&gt;Hello World&lt;/b&gt;');
});

QUnit.test('localizeElementFromTag() should replace innerHTML if flag is set',
  function(assert) {
    var element = document.createElement('div');
    $testStub(chrome.i18n.getMessage).withArgs('tag')
          .returns('<b>Hello World</b>');

    l10n.localizeElementFromTag(element, 'tag', null, true);

    assert.equal(element.innerHTML, '<b>Hello World</b>');
});

QUnit.test(
  'localizeElement() should replace innerText using the "i18n-content" ' +
  'attribute as the tag',
  function(assert) {
    var element = document.createElement('div');
    element.setAttribute('i18n-content', 'tag');
    $testStub(chrome.i18n.getMessage).withArgs('tag')
          .returns('<b>Hello World</b>');

    l10n.localizeElement(element);

    assert.equal(element.innerHTML, '&lt;b&gt;Hello World&lt;/b&gt;');
});

QUnit.test(
  'localize() should replace element title using the "i18n-title" ' +
  'attribute as the tag',
  function(assert) {
    var fixture = document.getElementById('qunit-fixture');
    fixture.innerHTML = '<div class="target" i18n-title="tag"></div>';
    $testStub(chrome.i18n.getMessage)
        .withArgs('tag')
        .returns('localized title');

    l10n.localize();

    var target = document.querySelector('.target');
    assert.equal(target.title, 'localized title');
});

QUnit.test('localize() should support string substitutions', function(assert) {
  var fixture = document.getElementById('qunit-fixture');
  fixture.innerHTML =
  '<div class="target" ' +
      'i18n-content="tag" ' +
      'i18n-value-1="param1" ' +
      'i18n-value-2="param2">' +
  '</div>';

  $testStub(chrome.i18n.getMessage).withArgs('tag', ['param1', 'param2'])
      .returns('localized');

  l10n.localize();

  var target = document.querySelector('.target');
  assert.equal(target.innerText, 'localized');
});

QUnit.test('localize() should support tag substitutions', function(assert) {
  var fixture = document.getElementById('qunit-fixture');
  fixture.innerHTML =
      '<div class="target" i18n-content="tag"' +
      ' i18n-value-name-1="tag1" i18n-value-name-2="tag2"></div>';

  $testStub(chrome.i18n.getMessage).withArgs('tag1').returns('param1');
  $testStub(chrome.i18n.getMessage).withArgs('tag2').returns('param2');
  $testStub(chrome.i18n.getMessage)
      .withArgs('tag', ['param1', 'param2'])
      .returns('localized');

  l10n.localize();

  var target = document.querySelector('.target');
  assert.equal(target.innerText, 'localized');
});

})();
