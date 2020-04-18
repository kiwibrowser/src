/**
 * testharness-helpers contains userful extensions to testharness.js
 * to allow them to be used across multiple tests.
 */


// Asserts that two objects |actual| and |expected| are weakly equal under the
// following definition:
//
// |a| and |b| are weakly equal if any of the following are true:
//   1. If |a| is not an 'object', and |a| === |b|.
//   2. If |a| is an 'object', and all of the following are true:
//     2.1 |a.p| is weakly equal to |b.p| for all enumerable properties |p| of
//     |a|, including inherited properties.
//     2.2 |b.p| is weakly equal to |a.p| for all enumerable properties |p| of
//     |b|, including inherited properties.
//
// This is a replacement for the the version of assert_object_equals() in
// testharness.js. The latter doesn't handle own properties correctly. I.e. if
// |a.p| is not an own property, it still requires that |b.p| be an own
// property.
//
// Note that |actual| must not contain cyclic references.
function assert_weak_equals(actual, expected, description) {
  var object_stack = [];

  function _is_equal(actual, expected, prefix) {
    if (typeof actual !== 'object' || actual === null) {
      assert_equals(actual, expected, prefix);
      return;
    }
    assert_true(typeof expected === 'object', prefix);
    assert_equals(object_stack.indexOf(actual), -1,
                  prefix + ' must not contain cyclic references.');

    object_stack.push(actual);

    let checked_set = new Set();
    for(var property in expected) {
      assert_true(property in actual, prefix);
      _is_equal(actual[property], expected[property], prefix + '.' + property);
      checked_set.add(actual[property]);
    }
    for(var property in actual) {
      assert_true(property in expected, prefix);
      if(!checked_set.has(actual[property])){
        _is_equal(actual[property], expected[property], prefix + '.' + property);
      }
    }

    object_stack.pop();
  }

  function _brand(object) {
    return Object.prototype.toString.call(object).match(/^\[object (.*)\]$/)[1];
  }

  _is_equal(actual, expected,
            (description ? description + ': ' : '') + _brand(expected));
};
