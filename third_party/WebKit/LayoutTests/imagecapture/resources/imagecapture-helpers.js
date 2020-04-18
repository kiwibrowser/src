'use strict';

function assert_point2d_array_approx_equals(actual, expected, epsilon) {
  assert_equals(actual.length, expected.length, 'length');
  for (var i = 0; i < actual.length; ++i) {
    assert_approx_equals(actual[i].x, expected[i].x, epsilon, 'x');
    assert_approx_equals(actual[i].y, expected[i].y, epsilon, 'y');
  }
}
