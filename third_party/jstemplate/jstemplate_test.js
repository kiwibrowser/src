// Copyright 2006 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.
/**
 * @author Steffen Meschkat (mesch@google.com)
 * @fileoverview Unittest and examples for jstemplates.
 */

function jstWrap(data, template) {
  return jstProcess(new JsEvalContext(data), template);
}

function testJstSelect() {
  // Template cardinality from jsselect.
  var t = document.getElementById('t1');
  var d = {
    items: [ 'A', 'B', 'C', '' ]
  }
  jstWrap(d, t);

  var h = t.innerHTML;
  var clone = domCloneNode(t);
  assertTrue(/>A<\/div>/.test(h));
  assertTrue(/>B<\/div>/.test(h));
  assertTrue(/>C<\/div>/.test(h));
  assertTrue(/><\/div>/.test(h));

  // Reprocessing with identical data.
  jstWrap(d, t);
  assertAttributesMatch(t, clone);

  // Reprocessing with changed data.
  d.items[1] = 'BB';
  jstWrap(d, t);

  h = t.innerHTML;
  assertTrue(/>A<\/div>/.test(h));
  assertFalse(/>B<\/div>/.test(h));
  assertTrue(/>BB<\/div>/.test(h));
  assertTrue(/>C<\/div>/.test(h));

  // Reprocessing with dropped data.
  d.items.pop();
  d.items.pop();
  jstWrap(d, t);
  h = t.innerHTML;
  assertTrue(/>A<\/div>/.test(h));
  assertTrue(/>BB<\/div>/.test(h));
  assertFalse(/>C<\/div>/.test(h));
  assertFalse(/><\/div>/.test(h));

  // Reprocessing with dropped data, once more.
  d.items.pop();
  jstWrap(d, t);
  h = t.innerHTML;
  assertTrue(/>A<\/div>/.test(h));
  assertFalse(/>BB<\/div>/.test(h));
  assertFalse(/>C<\/div>/.test(h));

  // Reprocessing with empty data -- the last template instance is
  // preserved, and only hidden.
  d.items.pop();
  jstWrap(d, t);

  assertTrue(/>A<\/div>/.test(h));
  assertFalse(/>BB<\/div>/.test(h));
  assertFalse(/>C<\/div>/.test(h));

  // Reprocessing with added data.
  d.items.push('D');
  jstWrap(d, t);
  h = t.innerHTML;
  assertFalse(/>A<\/div>/.test(h));
  assertTrue(/>D<\/div>/.test(h));
}

function testJstDisplay() {
  var t = document.getElementById('t2');
  var d = {
    display: true
  }
  jstWrap(d, t);

  var h = t.innerHTML;
  assertFalse(/display:\s*none/.test(h));

  d.display = false;
  jstWrap(d, t);

  h = t.innerHTML;
  assertTrue(/display:\s*none/.test(h));

  // Check that 'this' within js expressions is the template node
  t = document.getElementById('t2a');
  d = {
    showId: 'x'
  };
  jstWrap(d, t);

  h = t.innerHTML;
  assertFalse(/display:\s*none/.test(h));

  d.showId = 'y';
  jstWrap(d, t);

  h = t.innerHTML;
  assertTrue(/display:\s*none/.test(h));
}

function stringContains(str, sub) {
  return str.indexOf(sub) != -1;
}

function testJseval() {
  var data = {};

  var counter = 0;
  var ctx = new JsEvalContext(data);
  ctx.setVariable("callback1", function() {
    ++counter;
  });
  ctx.setVariable("callback2", function() {
    counter *= 2;
  });

  jstProcess(ctx, document.getElementById('testJseval1'));
  assertEquals("testJseval1", 1, counter);

  jstProcess(ctx, document.getElementById('testJseval2'));
  assertEquals("testJseval2", 4, counter);
}

function testJstValues() {
  var t = document.getElementById('t3');
  var d = {};
  jstWrap(d, t);
  var h = t.innerHTML;
  assertTrue(stringContains(h, 'http://maps.google.com/'));
  var t3a = document.getElementById('t3a');
  assertEquals('http://maps.google.com/', t3a.foo.bar.baz);
  assertEquals('http://maps.google.com/', t3a.bar);
  assertEquals('red', t3a.style.backgroundColor);
}

function testJstTransclude() {
  var t = document.getElementById('t4');
  var p = document.getElementById('parent');
  var d = {};
  jstWrap(d, t);
  var h = p.innerHTML;
  assertTrue(h, stringContains(h, 'http://maps.google.com/'));
}

function assertAttributesMatch(first, second) {
  assertEquals('assertAttributesMatch: number of child nodes',
               jsLength(first.childNodes), jsLength(second.childNodes));
  var b = second.firstChild;
  for (var a = first.firstChild; a; a = a.nextSibling) {
    var att = a.attributes;
    if (att) {
      assertTrue(b.attributes != null);
      assertEquals('assertAttributesMatch: number of attribute nodes',
                   att.length, b.attributes.length);
      for (var i = 0; i < jsLength(att); i++) {
        var a = att[i];
        assertEquals('assertAttributesMatch: value of attribute ' + a.name,
                     a.value, b.getAttribute(a.name));
      }
    } else {
      assertNull(b.attributes);
    }
    b = b.nextSibling;
  }
}

function testJsskip() {
  var div = domCreateElement(document, "DIV");
  div.innerHTML = [
      '<div jseval="outercallback()" jsskip="1">',
      '<div jseval="innercallback()">',
      '</div>',
      '</div>'
  ].join('');

  var data = {};
  var ctx = new JsEvalContext(data);
  var outerCalled = false;
  ctx.setVariable("outercallback", function() {
    outerCalled = true;
  });
  var innerCalled = false;
  ctx.setVariable("innercallback", function() {
    innerCalled = true;
  });
  jstProcess(ctx, div);

  assertTrue(outerCalled);
  assertFalse(innerCalled);
}

function testScalarContext() {
  var t = document.getElementById('testScalarContext');

  jstWrap(true, t);
  assertTrue(/>true</.test(t.innerHTML));

  jstWrap(false, t);
  assertTrue(/>false</.test(t.innerHTML));

  jstWrap(0, t);
  assertTrue(/>0</.test(t.innerHTML));

  jstWrap("foo", t);
  assertTrue(/>foo</.test(t.innerHTML));

  jstWrap(undefined, t);
  assertTrue(/>undefined</.test(t.innerHTML));

  jstWrap(null, t);
  assertTrue(/>null</.test(t.innerHTML));
}

function testJstLoadTemplate() {
  var wrapperId = 'testJstLoadTemplateWrapper';
  var id = 'testJstLoadTemplate';
  jstLoadTemplate_(document, '<div id="' + id + '">content</div>', wrapperId);
  var wrapperElem = document.getElementById(wrapperId);
  assertTrue('Expected wrapper element to be in document',
             !!wrapperElem);
  var newTemplate = document.getElementById(id);
  assertTrue('Expected newly loaded template to be in document',
             !!newTemplate);
  assertTrue('Expected wrapper to be grandparent of template',
             newTemplate.parentNode.parentNode == wrapperElem);

  // Make sure the next template loaded with the same wrapper id re-uses the
  // wrapper element.
  var id2 = 'testJstLoadTemplate2';
  jstLoadTemplate_(document, '<div id="' + id2 + '">content</div>', wrapperId);
  var newTemplate2 = document.getElementById(id2);
  assertTrue('Expected newly loaded template to be in document',
             !!newTemplate2);
  assertTrue('Expected wrapper to be grandparent of template',
             newTemplate2.parentNode.parentNode == wrapperElem);
}

function testJstGetTemplateFromDom() {
  var element;
  // Get by id a template in the document
  // Success
  element = jstGetTemplate('t1');
  assertTrue("Asserted jstGetTemplate('t1') to return a dom element",
             !!element);
  // Failure
  element = jstGetTemplate('asdf');
  assertFalse("Asserted jstGetTemplate('asdf') to return null",
              !!element);
}

function testJstGetTemplateFromFunction() {
  var element;
  // Fetch a jstemplate by id from within a html string, passed via a function.
  function returnHtmlWithId(id) {
    var html =
        '<div>' +
        '<div id="' + id + '">Here is the template</div>' +
        '</div>';
    return html;
  }
  // Success
  element = jstGetTemplate('template',
                           partial(returnHtmlWithId, 'template'));
  assertTrue("Expected jstGetTemplate('template') to return a dom element",
             !!element);

  // Failure
  element = jstGetTemplate('asdf',
                           partial(returnHtmlWithId, 'zxcv'));
  assertFalse("Expected jstGetTemplate('zxcv') to return null",
              !!element);
}

function testPrepareNode() {
  var id, node;
  // Reset the cache so we're testing from a known state.
  JstProcessor.jstCache_ = {};
  JstProcessor.jstCache_[0] = {};

  // Skip pre-processed nodes.  Preprocessed nodes are those with a
  // PROP_jstcache property.
  var t = document.getElementById('t1');
  var caches = [];
  caches.push(JstProcessor.prepareNode_(t));
  caches.push(JstProcessor.prepareNode_(t));
  assertEquals('The same cache should be returned on each call to prepareNode',
               caches[0], caches[1]);

  // Preprocessing a node with a jst attribute should return a valid struct
  id = 'testPrepareNodeWithAttributes';
  jstLoadTemplate_(document, '<div id="' + id + '" jsskip="1"></div>');
  node = document.getElementById(id);
  var cache = JstProcessor.prepareNode_(node);
  try {
    var jsskip = cache['jsskip']({}, {});
  } catch (e) {
    fail('Exception when evaluating jsskip from cache');
  }
  assertEquals(1, jsskip);
}


function testPrepareNodeWithNoAttributes() {
  // Preprocessing a node with no jst attributes should return null
  var id = 'testPrepareNodeNoAttributes';
  jstLoadTemplate_(document, '<div id="' + id + '"></div>');
  var node = document.getElementById(id);
  assertEquals('prepareNode with no jst attributes should return default',
               JstProcessor.jstcache_[0], JstProcessor.prepareNode_(node));
}


function testJsVars() {
  var template = document.createElement('div');
  document.body.appendChild(template);
  template.innerHTML = '<div jsvars="foo:\'foo\';bar:true;$baz:1"></div>';

  var context = new JsEvalContext;
  jstProcess(context, template);

  assertEquals('foo', context.getVariable('foo'));
  assertEquals(1, context.getVariable('$baz'));
  assertTrue(context.getVariable('bar'));
  assertUndefined(context.getVariable('foobar'));
}


function testCacheReuse() {
  var template = document.createElement('div');
  document.body.appendChild(template);
  template.innerHTML = 
    '<div jsvars="foo:\'foo\';bar:true;$baz:1"></div>' +
    '<span jsvars="foo:\'foo\';bar:true;$baz:1"></span>';
  JstProcessor.prepareTemplate_(template);
  assertEquals(template.firstChild.getAttribute(ATT_jstcache),
               template.lastChild.getAttribute(ATT_jstcache));
}
