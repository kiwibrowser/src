window.expect = (function() {
  var testElement = document.getElementById('target');

  function assert_equals_or_matches(actual, expected, description) {
    return expected instanceof RegExp ?
        assert_regexp_match(actual, expected, description) :
        assert_equals(actual, expected, description);
  }
  return function expect(property, camelProperty, input) {
    return {
      parsesAs: function(output) {
        test(function() {
          assert_true(CSS.supports(property, input), 'CSS.supports');
          testElement.style[camelProperty] = input;
          assert_equals(testElement.style[camelProperty], output);
        }, '"' + property + ': ' + input + ';" should parse as "' + output + '"');
        return this;
      },
      isComputedTo: function(output) {
        test(function() {
          testElement.style[camelProperty] = input;
          assert_equals_or_matches(getComputedStyle(testElement)[camelProperty], output);
        }, '"' + property + ': ' + input + ';" should be computed to "' + output + '"');
      },
      isInvalid: function() {
        test(function() {
          assert_false(CSS.supports(property, input), 'CSS.supports');
        }, '"' + property + ': ' + input + ';" should be invalid');
      },
    };
  }
})();
