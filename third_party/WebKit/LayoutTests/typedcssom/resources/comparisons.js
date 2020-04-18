function assert_array_approx_equals(actual, expected, epsilon) {
  for (var i = 0; i < actual.length; i++) {
    assert_approx_equals(actual[i], expected[i], epsilon);
  }
}

/**
 * Compares two instances of DOMMatrix.
 */
function assert_matrix_approx_equals(actual, expected, epsilon) {
  assert_array_approx_equals(
      actual.toFloat64Array(), expected.toFloat64Array(), epsilon);
}

