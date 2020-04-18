// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`This test checks Web Inspector utilities.\n`);


  TestRunner.runTestSuite([
    function remove(next) {
      var testArrays = [
        [], [], [], [1], [1], [1], [1, 2, 3, 4, 5, 4, 3, 2, 1], [1, 3, 4, 5, 4, 3, 2, 1], [1, 3, 4, 5, 4, 3, 1],
        [2, 2, 2, 2, 2], [2, 2, 2, 2], [], [2, 2, 2, 1, 2, 2, 3, 2], [2, 2, 1, 2, 2, 3, 2], [1, 3]
      ];
      for (var i = 0; i < testArrays.length; i += 3) {
        var actual = testArrays[i].slice(0);
        var expected = testArrays[i + 1];
        actual.remove(2, true);
        TestRunner.assertEquals(JSON.stringify(expected), JSON.stringify(actual), 'remove(2, true) passed');
        actual = testArrays[i].slice(0);
        expected = testArrays[i + 2];
        actual.remove(2, false);
        TestRunner.assertEquals(JSON.stringify(expected), JSON.stringify(actual), 'remove(2, false) passed');
      }
      next();
    },

    function orderedMergeIntersect(next) {
      function comparator(a, b) {
        return a - b;
      }
      function count(a, x) {
        return a.upperBound(x) - a.lowerBound(x);
      }
      function testAll(a, b) {
        testOperation(a, b, a.mergeOrdered(b, comparator), Math.max, 'U');
        testOperation(a, b, a.intersectOrdered(b, comparator), Math.min, 'x');
      }
      function testOperation(a, b, actual, checkOperation, opName) {
        var allValues = a.concat(b).concat(actual);
        for (var i = 0; i < allValues.length; ++i) {
          var value = allValues[i];
          expectedCount = checkOperation(count(a, value), count(b, value));
          actualCount = count(actual, value);
          TestRunner.assertEquals(
              expectedCount, actualCount,
              'Incorrect result for value: ' + value + ' at [' + a + '] ' + opName + ' [' + b + '] -> [' + actual +
                  ']');
        }
        TestRunner.assertEquals(JSON.stringify(actual.sort()), JSON.stringify(actual), 'result array is ordered');
      }
      var testArrays = [
        [], [], [1], [], [1, 2, 2, 2, 3], [], [4, 5, 5, 8, 8], [1, 1, 1, 2, 6], [1, 2, 2, 2, 2, 3, 3, 4],
        [2, 2, 2, 3, 3, 3, 3], [1, 2, 3, 4, 5], [1, 2, 3]
      ];
      for (var i = 0; i < testArrays.length; i += 2) {
        testAll(testArrays[i], testArrays[i + 1]);
        testAll(testArrays[i + 1], testArrays[i]);
      }
      next();
    },

    function binaryIndexOfTest(next) {
      var testArrays = [
        [], [1], [1, 10], [1, 10, 11, 12, 13, 14, 100], [-100, -50, 0, 50, 100], [-100, -14, -13, -12, -11, -10, -1]
      ];

      function testArray(array) {
        function comparator(a, b) {
          return a < b ? -1 : (a > b ? 1 : 0);
        }

        for (var i = -100; i <= 100; ++i) {
          var reference = array.indexOf(i);
          var actual = array.binaryIndexOf(i, comparator);
          TestRunner.assertEquals(reference, actual, 'binaryIndexOf');
        }
        return true;
      }

      for (var i = 0, l = testArrays.length; i < l; ++i)
        testArray(testArrays[i]);
      next();
    },

    function lowerBoundTest(next) {
      var testArrays = [[], [1], [-1, -1, 0, 0, 0, 0, 2, 3, 4, 4, 4, 7, 9, 9, 9]];

      function testArray(array, useComparator) {
        function comparator(a, b) {
          return a < b ? -1 : (a > b ? 1 : 0);
        }

        for (var value = -2; value <= 12; ++value) {
          var index = useComparator ? array.lowerBound(value, comparator) : array.lowerBound(value);
          TestRunner.assertTrue(0 <= index && index <= array.length, 'index is within bounds');
          TestRunner.assertTrue(index === 0 || array[index - 1] < value, 'array[index - 1] < value');
          TestRunner.assertTrue(index === array.length || array[index] >= value, 'array[index] >= value');
        }
      }

      for (var i = 0, l = testArrays.length; i < l; ++i) {
        testArray(testArrays[i], false);
        testArray(testArrays[i], true);
      }
      next();
    },

    function upperBoundTest(next) {
      var testArrays = [[], [1], [-1, -1, 0, 0, 0, 0, 2, 3, 4, 4, 4, 7, 9, 9, 9]];

      function testArray(array, useComparator) {
        function comparator(a, b) {
          return a < b ? -1 : (a > b ? 1 : 0);
        }

        for (var value = -2; value <= 12; ++value) {
          var index = useComparator ? array.upperBound(value, comparator) : array.upperBound(value);
          TestRunner.assertTrue(0 <= index && index <= array.length, 'index is within bounds');
          TestRunner.assertTrue(index === 0 || array[index - 1] <= value, 'array[index - 1] <= value');
          TestRunner.assertTrue(index === array.length || array[index] > value, 'array[index] > value');
        }
      }

      for (var i = 0, l = testArrays.length; i < l; ++i) {
        testArray(testArrays[i], false);
        testArray(testArrays[i], true);
      }
      next();
    },

    function qselectTest(next) {
      var testArrays =
          [[], [0], [0, 0, 0, 0, 0, 0, 0, 0], [4, 3, 2, 1], [1, 2, 3, 4, 5], [-1, 3, 2, 7, 7, 7, 10, 12, 3, 4, -1, 2]];

      function testArray(array) {
        function compare(a, b) {
          return a - b;
        }
        var sorted = array.slice(0).sort(compare);

        var reference = {min: sorted[0], median: sorted[Math.floor(sorted.length / 2)], max: sorted[sorted.length - 1]};

        var actual = {
          min: array.slice(0).qselect(0),
          median: array.slice(0).qselect(Math.floor(array.length / 2)),
          max: array.slice(0).qselect(array.length - 1)
        };
        TestRunner.addResult('Array: ' + JSON.stringify(array));
        TestRunner.addResult('Reference: ' + JSON.stringify(reference));
        TestRunner.addResult('Actual:    ' + JSON.stringify(actual));
      }
      for (var i = 0, l = testArrays.length; i < l; ++i)
        testArray(testArrays[i]);
      next();
    },

    function sortRangeTest(next) {
      var testArrays = [[], [1], [2, 1], [6, 4, 2, 7, 10, 15, 1], [10, 44, 3, 6, 56, 66, 10, 55, 32, 56, 2, 5]];

      function testArray(array) {
        function comparator(a, b) {
          return a < b ? -1 : (a > b ? 1 : 0);
        }

        function compareArrays(a, b, message) {
          TestRunner.assertEquals(JSON.stringify(a), JSON.stringify(b), message);
        }

        for (var left = 0, l = array.length - 1; left < l; ++left) {
          for (var right = left, r = array.length; right < r; ++right) {
            for (var first = left; first <= right; ++first) {
              for (var count = 1, k = right - first + 1; count <= k; ++count) {
                var actual = array.slice(0);
                actual.sortRange(comparator, left, right, first, first + count - 1);
                compareArrays(array.slice(0, left), actual.slice(0, left), 'left ' + left + ' ' + right + ' ' + count);
                compareArrays(
                    array.slice(right + 1), actual.slice(right + 1), 'right ' + left + ' ' + right + ' ' + count);
                var middle = array.slice(left, right + 1);
                middle.sort(comparator);
                compareArrays(
                    middle.slice(first - left, first - left + count), actual.slice(first, first + count),
                    'sorted ' + left + ' ' + right + ' ' + first + ' ' + count);
                actualRest = actual.slice(first + count, right + 1);
                actualRest.sort(comparator);
                compareArrays(
                    middle.slice(first - left + count), actualRest,
                    'unsorted ' + left + ' ' + right + ' ' + first + ' ' + count);
              }
            }
          }
        }
      }

      for (var i = 0, len = testArrays.length; i < len; ++i)
        testArray(testArrays[i]);
      next();
    },

    function naturalOrderComparatorTest(next) {
      var testArray = [
        'dup', 'a1',   'a4222',  'a91',       'a07',      'dup', 'a7',        'a007',      'abc00',     'abc0',
        'abc', 'abcd', 'abc000', 'x10y20z30', 'x9y19z29', 'dup', 'x09y19z29', 'x10y22z23', 'x10y19z43', '1',
        '10',  '11',   'dup',    '2',         '2',        '2',   '555555',    '5',         '5555',      'dup',
      ];

      for (var i = 0, n = testArray.length; i < n; ++i)
        TestRunner.assertEquals(
            0, String.naturalOrderComparator(testArray[i], testArray[i]), 'comparing equal strings');

      testArray.sort(String.naturalOrderComparator);
      TestRunner.addResult('Sorted in natural order: [' + testArray.join(', ') + ']');

      // Check comparator's transitivity.
      for (var i = 0, n = testArray.length; i < n; ++i) {
        for (var j = 0; j < n; ++j) {
          var a = testArray[i];
          var b = testArray[j];
          var diff = String.naturalOrderComparator(a, b);
          if (diff === 0)
            TestRunner.assertEquals(a, b, 'zero diff');
          else if (diff < 0)
            TestRunner.assertTrue(i < j);
          else
            TestRunner.assertTrue(i > j);
        }
      }
      next();
    },

    function stableSortPrimitiveTest(next) {
      TestRunner.assertEquals('', [].stableSort().join(','));
      TestRunner.assertEquals('1', [1].stableSort().join(','));
      TestRunner.assertEquals('1,2', [2, 1].stableSort().join(','));
      TestRunner.assertEquals('1,1,1', [1, 1, 1].stableSort().join(','));
      TestRunner.assertEquals('1,2,3,4', [1, 2, 3, 4].stableSort().join(','));

      function defaultComparator(a, b) {
        return a < b ? -1 : (a > b ? 1 : 0);
      }

      function strictNumberCmp(a, b) {
        if (a !== b)
          return false;
        if (a)
          return true;
        return 1 / a === 1 / b;  // Neg zero comparison.
      }

      // The following fails with standard Array.sort()
      var original = [0, -0, 0, -0, -0, -0, 0, 0, 0, -0, 0, -0, -0, -0, 0, 0];
      var sorted = original.slice(0).stableSort(defaultComparator);
      for (var i = 0; i < original.length; ++i)
        TestRunner.assertTrue(strictNumberCmp(original[i], sorted[i]), 'Sorting negative zeros');

      // Test sorted and backward-sorted arrays.
      var identity = [];
      var reverse = [];
      for (var i = 0; i < 100; ++i) {
        identity[i] = i;
        reverse[i] = -i;
      }
      identity.stableSort(defaultComparator);
      reverse.stableSort(defaultComparator);
      for (var i = 1; i < identity.length; ++i) {
        TestRunner.assertTrue(identity[i - 1] < identity[i], 'identity');
        TestRunner.assertTrue(reverse[i - 1] < reverse[i], 'reverse');
      }

      next();
    },

    function stableSortTest(next) {
      function Pair(key1, key2) {
        this.key1 = key1;
        this.key2 = key2;
      }
      Pair.prototype.toString = function() {
        return 'Pair(' + this.key1 + ', ' + this.key2 + ')';
      };

      var testArray = [
        new Pair(1, 1),  new Pair(3, 31),  new Pair(3, 30), new Pair(5, 55), new Pair(5, 54), new Pair(5, 57),
        new Pair(4, 47), new Pair(4, 46),  new Pair(4, 45), new Pair(5, 52), new Pair(5, 59), new Pair(3, 37),
        new Pair(3, 38), new Pair(5, -58), new Pair(5, 58), new Pair(4, 41), new Pair(4, 42), new Pair(4, 43),
        new Pair(3, 33), new Pair(3, 32),  new Pair(9, 9),
      ];

      var testArrayKey2Values = [];
      for (var i = 0, n = testArray.length; i < n; ++i) {
        var value = testArray[i].key2;
        TestRunner.assertTrue(testArrayKey2Values.indexOf(value) === -1, 'FAIL: key2 values should be unique');
        testArrayKey2Values.push(value);
      }

      function comparator(a, b) {
        return a.key1 - b.key1;
      }
      testArray.stableSort(comparator);
      TestRunner.addResult('Stable sort result:\n' + testArray.join('\n'));

      function assertHelper(condition, message, i, a, b) {
        TestRunner.assertTrue(condition, 'FAIL: ' + message + ' at index ' + i + ': ' + a + ' < ' + b);
      }

      for (var i = 1, n = testArray.length; i < n; ++i) {
        var a = testArray[i - 1];
        var b = testArray[i];
        assertHelper(a.key1 <= b.key1, 'primary key order', i, a, b);
        if (a.key1 !== b.key1)
          continue;
        var key2IndexA = testArrayKey2Values.indexOf(a.key2);
        TestRunner.assertTrue(key2IndexA !== -1);
        var key2IndexB = testArrayKey2Values.indexOf(b.key2);
        TestRunner.assertTrue(key2IndexB !== -1);
        assertHelper(key2IndexA < key2IndexB, 'stable order', i, a, b);
      }
      next();
    },

    function stringHashTest(next) {
      var stringA = ' '.repeat(10000);
      var stringB = stringA + ' ';
      var hashA = String.hashCode(stringA);
      TestRunner.assertTrue(hashA !== String.hashCode(stringB));
      TestRunner.assertTrue(isFinite(hashA));
      TestRunner.assertTrue(hashA + 1 !== hashA);
      next();
    },

    function stringTrimURLTest(next) {
      var baseURLDomain = 'www.chromium.org';
      var testArray = [
        'http://www.chromium.org/foo/bar',
        '/foo/bar',
        'https://www.CHromium.ORG/BAZ/zoo',
        '/BAZ/zoo',
        'https://example.com/foo[]',
        'example.com/foo[]',
      ];
      for (var i = 0; i < testArray.length; i += 2) {
        var url = testArray[i];
        var expected = testArray[i + 1];
        TestRunner.assertEquals(expected, url.trimURL(baseURLDomain), url);
      }
      next();
    },

    function stringToBase64Test(next) {
      var testArray = [
        '', '', 'a', 'YQ==', 'bc', 'YmM=', 'def', 'ZGVm', 'ghij', 'Z2hpag==', 'klmno', 'a2xtbm8=', 'pqrstu', 'cHFyc3R1',
        String.fromCharCode(0x444, 0x5555, 0x66666, 0x777777), '0YTllZXmmabnnbc='
      ];
      for (var i = 0; i < testArray.length; i += 2) {
        var string = testArray[i];
        var encodedString = testArray[i + 1];
        TestRunner.assertEquals(encodedString, string.toBase64());
      }
      next();
    },

    function trimMiddle(next) {
      var testArray = [
        '', '!', '\uD83D\uDE48A\uD83D\uDE48L\uD83D\uDE48I\uD83D\uDE48N\uD83D\uDE48A\uD83D\uDE48\uD83D\uDE48', 'test'
      ];
      for (var string of testArray) {
        for (var maxLength = string.length + 1; maxLength > 0; --maxLength) {
          var trimmed = string.trimMiddle(maxLength);
          TestRunner.addResult(trimmed);
          TestRunner.assertTrue(trimmed.length <= maxLength);
        }
      }
      next();
    }
  ]);
})();
