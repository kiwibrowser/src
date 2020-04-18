// To run these tests, load results.html in a browser.
// You should see a series of PASS lines.
if (window.testRunner) {
    testRunner.dumpAsText();
}

var testStyles = document.createElement('style');
testStyles.innerText = ".test-pass { color: green; } .test-fail { color: red; }";
document.querySelector('head').appendChild(testStyles);

var g_testIndex = 0;
var g_log = ["You should see a series of PASS lines."];

// Make async actually be sync for the sake of simpler testing.
function async(func, args) {
    func.apply(null, args);
}

function mockResults() {
    return {
        tests: {},
        "skipped": 0,
        "num_regressions": 0,
        "version": 0,
        "num_passes": 0,
        "fixable": 0,
        "num_flaky": 0,
        "layout_tests_dir": "/WEBKITROOT",
        "chromium_revision": 12345,
        "pixel_tests_enabled": true
    };
}

function isFailureExpected(expected, actual) {
    var isExpected = true;
    if (actual != 'SKIP') {
        var expectedArray = expected.split(' ');
        var actualArray = actual.split(' ');
        for (var i = 0; i < actualArray.length; i++) {
            var actualValue = actualArray[i];
            if (expectedArray.indexOf(actualValue) == -1 &&
                (expectedArray.indexOf('FAIL') == -1 ||
                 (actualValue != 'TEXT' && actualValue != 'IMAGE+TEXT' && actualValue != 'AUDIO')))
                isExpected = false;
        }
    }
    return isExpected;
}

function mockExpectation(expected, actual) {
    return {
        expected: expected,
        time_ms: 1,
        actual: actual,
        has_stderr: false,
        is_unexpected: !isFailureExpected(expected, actual)
    };
}

function currentTestName() {
    var testName = 'TEST ' + g_testIndex;
    if (g_testName) {
        testName += ' (' + g_testName + ')';
    }
    return testName;
}

function logPass(msg) {
    g_log.push('<span class="test-pass">' + msg + '</span>: ' + currentTestName());
}

function logFail(msg) {
    g_log.push('<span class="test-fail">' + msg + '</span>: ' + currentTestName());
}

function assertTrue(bool) {
    if (bool) {
        logPass('PASS');
    } else {
        logFail('FAIL');
    }
}

function runTest(results, assertions, opt_testName, opt_localStorageValue) {
    document.body.innerHTML = '';
    g_testIndex++;
    g_state = undefined;
    g_testName = opt_testName || '';
    localStorage.setItem(OptionWriter._key, opt_localStorageValue || '');

    try {
        ADD_RESULTS(results);
        originalGeneratePage();
    } catch (e) {
        logFail("FAIL: uncaught exception " + e.toString());
    }

    try {
        assertions();
    } catch (e) {
        logFail("FAIL: uncaught exception executing assertions " + e.toString());
    }
}

function runDefaultSingleRowTest(test, expected, actual, isExpected, textResults, imageResults, opt_testName) {
    var results = mockResults();
    results.tests[test] = mockExpectation(expected, actual);
    runSingleRowTest(results, isExpected, textResults, imageResults, opt_testName);
}

function runSingleRowTest(results, isExpected, textResults, imageResults, opt_testName) {
    for (var key in results.tests)
        var test = key;
    var expected = results.tests[test].expected;
    var actual = results.tests[test].actual;
    runTest(results, function() {
        if (isExpected) {
            assertTrue(document.querySelector('tbody').className == 'expected');
        } else {
            assertTrue(document.querySelector('tbody').className.indexOf('expected') == -1);
        }

        assertTrue(document.querySelector('tbody td:nth-child(1)').textContent == '+' + test + ' \u2691');
        assertTrue(document.querySelector('tbody td:nth-child(2)').textContent == textResults);
        assertTrue(document.querySelector('tbody td:nth-child(3)').textContent == imageResults);
        assertTrue(document.querySelector('tbody td:nth-child(4)').textContent == actual);
        assertTrue(document.querySelector('tbody td:nth-child(5)').textContent == expected);
    }, opt_testName || 'single row test');

}

function runTests() {
    var results = mockResults();
    var subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('PASS', 'TEXT');
    runTest(results, function() {
        assertTrue(document.getElementById('image-results-header').textContent == '');
        assertTrue(document.getElementById('text-results-header').textContent != '');
    }, 'text results header');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'MISSING');
    subtree['bar.html'].is_missing_text = true;
    subtree['bar.html'].is_missing_audio = true;
    subtree['bar.html'].is_missing_image = true;
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));
        assertTrue(document.querySelector('#missing-table .test-link').textContent == 'foo/bar.html');
        assertTrue(document.getElementsByClassName('result-link')[0].textContent == 'audio result');
        assertTrue(document.getElementsByClassName('result-link')[1].textContent == 'result');
        assertTrue(document.getElementsByClassName('result-link')[2].textContent == 'png result');
    }, 'actual result links');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('PASS', 'TEXT');
    subtree['bar.html'].has_stderr = true;
    runTest(results, function() {
        assertTrue(document.getElementById('results-table'));
        assertTrue(document.querySelector('#stderr-table .result-link').textContent == 'stderr');
    }, 'stderr link');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'PASS');
    subtree['bar1.html'] = mockExpectation('CRASH', 'PASS');
    subtree['bar2.html'] = mockExpectation('IMAGE', 'PASS');
    subtree['crash.html'] = mockExpectation('IMAGE', 'CRASH');
    subtree['timeout.html'] = mockExpectation('IMAGE', 'TIMEOUT');
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));

        var testLinks = document.querySelectorAll('#passes-table .test-link');
        assertTrue(testLinks[0].textContent == 'foo/bar.html');
        assertTrue(testLinks[1].textContent == 'foo/bar1.html');
        assertTrue(testLinks[2].textContent == 'foo/bar2.html');

        assertTrue(!document.querySelector('#passes-table .expand-button'));

        var expectationTypes = document.querySelectorAll('#passes-table td:last-of-type');
        assertTrue(expectationTypes[0].textContent == 'TEXT');
        assertTrue(expectationTypes[1].textContent == 'CRASH');
        assertTrue(expectationTypes[2].textContent == 'IMAGE');

        assertTrue(document.getElementById('crash-tests-table'));
        assertTrue(document.getElementById('crash-tests-table').textContent.indexOf('crash log') != -1);
        assertTrue(document.getElementById('timeout-tests-table'));
        assertTrue(document.getElementById('timeout-tests-table').textContent.indexOf('expected actual diff') != -1);
    }, 'crash and timeout tests tables');

    function isExpanded(expandLink)
    {
        var enDash = '\u2013';
        return expandLink.textContent == enDash;
    }

    function isCollapsed(expandLink)
    {
        return expandLink.textContent == '+';
    }

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'PASS');
    subtree['bar-missing.html'] = mockExpectation('TEXT', 'MISSING');
    subtree['bar-missing.html'].is_missing_text = true;
    subtree['bar-stderr.html'] = mockExpectation('PASS', 'TEXT');
    subtree['bar-stderr.html'].has_stderr = true;
    subtree['bar-unexpected-pass.html'] = mockExpectation('TEXT', 'PASS');
    runTest(results, function() {
        assertTrue(document.querySelectorAll('tbody tr').length == 5);
        expandAllExpectations();
        assertTrue(document.querySelectorAll('tbody tr').length == 8);
        var expandLinks = document.querySelectorAll('.expand-button-text');
        for (var i = 0; i < expandLinks.length; i++) {
            assertTrue(isExpanded(expandLinks[i]));
        }

        collapseAllExpectations();
        // Collapsed expectations stay in the dom, but are display:none.
        assertTrue(document.querySelectorAll('tbody tr').length == 8);
        expandLinks = document.querySelectorAll('.expand-button-text');
        for (var i = 0; i < expandLinks.length; i++) {
            assertTrue(isCollapsed(expandLinks[i]));
        }

        expandExpectations(expandLinks[1]);
        assertTrue(isCollapsed(expandLinks[0]));
        assertTrue(isExpanded(expandLinks[1]));

        collapseExpectations(expandLinks[1]);
        assertTrue(expandLinks[1].textContent == '+');
    }, 'collapsing expectation rows');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('PASS', 'TEXT');
    subtree['bar-expected-fail.html'] = mockExpectation('TEXT', 'TEXT');
    runTest(results, function() {
        assertTrue(document.querySelectorAll('.expected').length == 1);
        assertTrue(document.querySelector('.expected .test-link').textContent == 'foo/bar-expected-fail.html');

        assertTrue(window.getComputedStyle(document.querySelectorAll('tbody')[0], null)['display'] == 'none');

        expandAllExpectations();
        assertTrue(visibleExpandLinks().length == 1);
        assertTrue(document.querySelectorAll('.results-row').length == 1);
        assertTrue(window.getComputedStyle(document.querySelectorAll('tbody')[0], null)['display'] == 'none');

        document.getElementById('show-expected-failures').checked = true;
        document.getElementById('show-expected-failures').onchange();

        assertTrue(visibleExpandLinks().length == 2);
        assertTrue(document.querySelectorAll('.results-row').length == 1);
        assertTrue(window.getComputedStyle(document.querySelectorAll('tbody')[0], null)['display'] != 'none');

        expandAllExpectations();
        assertTrue(document.querySelectorAll('.results-row').length == 2);
        assertTrue(window.getComputedStyle(document.querySelectorAll('tbody')[0], null)['display'] != 'none');
    }, 'expanding expectation rows');

    results = mockResults();
    results.tests['only-expected-fail.html'] = mockExpectation('TEXT', 'TEXT');
    runTest(results, function() {
        assertTrue(window.getComputedStyle(document.getElementById('results-table').parentNode, null)['display'] == 'none');
    }, 'only one expected fail result');

    runDefaultSingleRowTest('bar-skip.html', 'TEXT', 'SKIP', true, '', '');
    runDefaultSingleRowTest('bar-flaky-fail.html', 'PASS FAIL', 'TEXT', true, 'expected actual diff pretty diff ', '');
    runDefaultSingleRowTest('bar-flaky-fail-unexpected.html', 'PASS TEXT', 'IMAGE', false, '', 'images diff ');
    runDefaultSingleRowTest('bar-audio.html', 'TEXT', 'AUDIO', false, 'expected audio actual audio ', '');
    runDefaultSingleRowTest('bar-image.html', 'TEXT', 'IMAGE', false, '', 'images diff ');
    runDefaultSingleRowTest('bar-image-plus-text.html', 'TEXT', 'IMAGE+TEXT', false, 'expected actual diff pretty diff ', 'images diff ');

    // Test the mapping for FAIL onto only ['IMAGE+TEXT', 'AUDIO', 'TEXT', 'IMAGE'].
    runDefaultSingleRowTest('bar-image.html', 'FAIL', 'IMAGE+TEXT', true, 'expected actual diff pretty diff ', 'images diff ');
    runDefaultSingleRowTest('bar-image.html', 'FAIL', 'AUDIO', true, 'expected audio actual audio ', '');
    runDefaultSingleRowTest('bar-image.html', 'FAIL', 'TEXT', true, 'expected actual diff pretty diff ', '');
    runDefaultSingleRowTest('bar-image.html', 'FAIL', 'IMAGE', false, '', 'images diff ');

    results = mockResults();
    results.tests['bar-reftest.html'] = mockExpectation('PASS', 'IMAGE');
    results.tests['bar-reftest.html'].reftest_type = ['=='];
    runSingleRowTest(results, false, '', 'ref html images diff ', 'match reftest single row test');

    results = mockResults();
    results.tests['bar-reftest-mismatch.html'] = mockExpectation('PASS', 'IMAGE');
    results.tests['bar-reftest-mismatch.html'].reftest_type = ['!='];
    runSingleRowTest(results, false, '', 'ref mismatch html actual ', 'mismatch reftest single row test');

    results = mockResults();
    results.tests['bar-reftest.html'] = mockExpectation('IMAGE', 'PASS');
    results.tests['bar-reftest.html'].reftest_type = ['=='];
    results.pixel_tests_enabled = false;
    runTest(results, function() {
        assertTrue(document.querySelector('tbody td:nth-child(1)').textContent == 'bar-reftest.html \u2691');
    }, 'match reftest');

    results = mockResults();
    results.tests['bar-reftest-mismatch.html'] = mockExpectation('IMAGE', 'PASS');
    results.tests['bar-reftest-mismatch.html'].reftest_type = ['!='];
    results.pixel_tests_enabled = false;
    runTest(results, function() {
        assertTrue(document.querySelector('tbody td:nth-child(1)').textContent == 'bar-reftest-mismatch.html \u2691');
    }, 'mismatch reftest');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar-flaky-pass.html'] = mockExpectation('PASS TEXT', 'PASS');
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));
        assertTrue(document.getElementById('passes-table'));
        assertTrue(document.body.textContent.indexOf('foo/bar-flaky-pass.html') != -1);
    }, 'expected flaky and passed');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar-flaky-fail.html'] = mockExpectation('PASS TEXT', 'IMAGE PASS');
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));
        assertTrue(document.getElementById('flaky-tests-table'));
        assertTrue(document.body.textContent.indexOf('bar-flaky-fail.html') != -1);
    }, 'expected flaky and image mismatch');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar-flaky-expected.html'] = mockExpectation('PASS FAIL', 'PASS TEXT');
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));
        assertTrue(document.getElementById('flaky-tests-table'));
        assertTrue(document.body.textContent.indexOf('bar-flaky-expected.html') != -1);
        assertTrue(document.querySelector('tbody').className == 'expected');
    }, 'expected flaky and text mismatch');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar-really-long-path-that-should-probably-wrap-because-otherwise-the-table-will-be-too-wide.html'] = mockExpectation('PASS', 'TEXT');
    runTest(results, function() {
        document.body.style.width = '800px';
        var links = document.querySelectorAll('tbody a');
        assertTrue(links[0].getClientRects().length == 2);
        assertTrue(links[1].getClientRects().length == 1);
        document.body.style.width = '';
    }, 'long test name');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'TEXT');
    runTest(results, function() {
        assertTrue(document.querySelector('tbody td:nth-child(2)').textContent.indexOf('pretty diff') != -1);
    }, 'pretty diff link');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'PASS');
    subtree['bar-1.html'] = mockExpectation('TEXT', 'CRASH');
    subtree['bar-5.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    subtree['bar-3.html'] = mockExpectation('PASS', 'TEXT');
    subtree['bar-2.html'] = mockExpectation('PASS', 'IMAGE');
    runTest(results, function() {
        // FIXME: This just ensures we don't get a JS error.
        // Verify that the sort is correct and that inline expanded expectations
        // move along with the test they're attached to.
        TableSorter.sortColumn(0);
        TableSorter.sortColumn(0);
        TableSorter.sortColumn(1);
        TableSorter.sortColumn(1);
        TableSorter.sortColumn(2);
        TableSorter.sortColumn(2);
        TableSorter.sortColumn(3);
        TableSorter.sortColumn(3);
        TableSorter.sortColumn(4);
        TableSorter.sortColumn(4);
        TableSorter.sortColumn(0);
        logPass('PASS');
    }, 'TableSorter.sortColumn does not raise a JS error');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar-5.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    runTest(results, function() {
        expandAllExpectations();
        var png = document.querySelector('[src*="bar-5-expected.png"]');
        var x = png.offsetLeft + 1;
        var y = png.offsetTop + 1;
        var mockEvent = {
            target: png,
            clientX: x,
            clientY: y
        };
        PixelZoomer.showOnDelay = false;
        PixelZoomer.handleMouseMove(mockEvent);
        assertTrue(!!document.querySelector('.pixel-zoom-container'));
        assertTrue(document.querySelectorAll('.zoom-image-container').length == 3);
    }, 'zoom image on hover');

    results = mockResults();
    subtree = results.tests['fullscreen'] = {};
    subtree['full-screen-api.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    runTest(results, function() {
        // Use a regexp to match windows and unix-style paths.
        var expectedRegExp = new RegExp('^file.*' + results.layout_tests_dir + '/fullscreen/full-screen-api.html$');
        assertTrue(expectedRegExp.exec(document.querySelector('tbody td:first-child a').href));
    }, 'local test link');

    var oldShouldUseTracLinks = shouldUseTracLinks;
    shouldUseTracLinks = function() { return true; };

    results = mockResults();
    subtree = results.tests['fullscreen'] = {};
    subtree['full-screen-api.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    runTest(results, function() {
        var expectedHref = 'https://crrev.com/' + results.chromium_revision + '/third_party/WebKit/LayoutTests/fullscreen/full-screen-api.html';
        assertTrue(document.querySelector('tbody td:first-child a').href == expectedHref);
    }, 'chromium revision link');

    results = mockResults();
    subtree = results.tests['fullscreen'] = {};
    subtree['full-screen-api.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    results.chromium_revision = '';
    runTest(results, function() {
        var expectedHref = 'https://chromium.googlesource.com/chromium/src/+/master/third_party/WebKit/LayoutTests/fullscreen/full-screen-api.html';
        assertTrue(document.querySelector('tbody td:first-child a').href == expectedHref);
    }, 'googlesource test link');

    shouldUseTracLinks = oldShouldUseTracLinks;

    results = mockResults();
    results.tests['bar.html'] = mockExpectation('PASS', 'IMAGE');
    runTest(results, function() {
        assertTrue(document.querySelector('tbody td:nth-child(3)').textContent == 'images diff ');

        document.getElementById('toggle-images').checked = false;
        // FIXME: We shouldn't need to call updateTogglingImages. Setting checked above should call it.
        updateTogglingImages();
        // FIXME: We get extra spaces in the DOM every time we enable/disable image toggling.
        assertTrue(document.querySelector('tbody td:nth-child(3)').textContent == 'expected actual  diff ');

        document.getElementById('toggle-images').checked = true;
        updateTogglingImages();
        assertTrue(document.querySelector('tbody td:nth-child(3)').textContent == ' images   diff ');
    }, 'toggle images option');

    results = mockResults();
    results.tests['reading-options-from-localstorage.html'] = mockExpectation('IMAGE+TEXT', 'IMAGE+TEXT');
    runTest(results, function() {
        assertTrue(window.getComputedStyle(document.querySelector('tbody'), null)['display'] != 'none');
        assertTrue(document.querySelector('tbody td:nth-child(3)').textContent == 'expected actual  diff ');
    }, 'reading options from localstorage', '{"toggle-images":false,"show-expected-failures":true}');

    function enclosingNodeWithTagNameHasClassName(node, tagName, className) {
        while (node && (!node.tagName || node.localName != tagName)) {
            node = node.parentNode;
        }
        if (!node) {
            return false;
        }
        return node.className == className;
    }

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['expected-to-pass-but-crashed.html'] = mockExpectation('PASS', 'CRASH');
    subtree['expected-to-pass-or-crash-and-crashed.html'] = mockExpectation('PASS CRASH', 'CRASH');
    subtree['expected-to-pass-but-timeouted.html'] = mockExpectation('PASS', 'CRASH');
    subtree['expected-to-pass-or-timeout-and-timeouted.html'] = mockExpectation('PASS TIMEOUT', 'TIMEOUT');
    subtree['expected-fail-but-passed.html'] = mockExpectation('FAIL', 'PASS');
    subtree['expected-pass-or-fail-and-passed.html'] = mockExpectation('PASS FAIL', 'PASS');
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));

        var testLinks = document.querySelectorAll('.test-link');
        assertTrue(testLinks[0].innerText == 'foo/expected-to-pass-but-crashed.html');
        assertTrue(!enclosingNodeWithTagNameHasClassName(testLinks[0], 'tbody', 'expected'));
        assertTrue(testLinks[1].innerText == 'foo/expected-to-pass-or-crash-and-crashed.html');
        assertTrue(enclosingNodeWithTagNameHasClassName(testLinks[1], 'tbody', 'expected'));
        assertTrue(!enclosingNodeWithTagNameHasClassName(testLinks[0], 'table', 'expected'));

        assertTrue(testLinks[2].innerText == 'foo/expected-to-pass-but-timeouted.html');
        assertTrue(!enclosingNodeWithTagNameHasClassName(testLinks[2], 'tbody', 'expected'));
        assertTrue(testLinks[3].innerText == 'foo/expected-to-pass-or-timeout-and-timeouted.html');
        assertTrue(enclosingNodeWithTagNameHasClassName(testLinks[3], 'tbody', 'expected'));
        assertTrue(!enclosingNodeWithTagNameHasClassName(testLinks[2], 'table', 'expected'));

        assertTrue(testLinks[4].innerText == 'foo/expected-fail-but-passed.html');
        assertTrue(!enclosingNodeWithTagNameHasClassName(testLinks[4], 'tbody', 'expected'));
        assertTrue(testLinks[5].innerText == 'foo/expected-pass-or-fail-and-passed.html');
        assertTrue(enclosingNodeWithTagNameHasClassName(testLinks[5], 'tbody', 'expected'));
        assertTrue(!enclosingNodeWithTagNameHasClassName(testLinks[4], 'table', 'expected'));
    }, 'class names 1');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['expected-to-pass-or-crash-and-crashed.html'] = mockExpectation('PASS CRASH', 'CRASH');
    subtree['expected-to-pass-or-timeout-and-timeouted.html'] = mockExpectation('PASS TIMEOUT', 'TIMEOUT');
    subtree['expected-pass-or-fail-and-passed.html'] = mockExpectation('PASS FAIL', 'PASS');
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));

        var testLinks = document.querySelectorAll('.test-link');
        assertTrue(testLinks[0].innerText == 'foo/expected-to-pass-or-crash-and-crashed.html');
        assertTrue(enclosingNodeWithTagNameHasClassName(testLinks[0], 'tbody', 'expected'));
        assertTrue(enclosingNodeWithTagNameHasClassName(testLinks[0], 'div', 'expected'));

        assertTrue(testLinks[1].innerText == 'foo/expected-to-pass-or-timeout-and-timeouted.html');
        assertTrue(enclosingNodeWithTagNameHasClassName(testLinks[1], 'tbody', 'expected'));
        assertTrue(enclosingNodeWithTagNameHasClassName(testLinks[1], 'div', 'expected'));

        assertTrue(testLinks[2].innerText == 'foo/expected-pass-or-fail-and-passed.html');
        assertTrue(enclosingNodeWithTagNameHasClassName(testLinks[2], 'tbody', 'expected'));
        assertTrue(enclosingNodeWithTagNameHasClassName(testLinks[2], 'div', 'expected'));
    }, 'class names 2');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'PASS');
    subtree['crash.html'] = mockExpectation('IMAGE', 'CRASH');
    subtree['flaky-fail.html'] = mockExpectation('PASS TEXT', 'IMAGE PASS');
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));

        var resultText = document.body.textContent;
        assertTrue(resultText.indexOf('crash.html') != -1);
        assertTrue(resultText.indexOf('flaky-fail.html') != -1);
        assertTrue(resultText.indexOf('crash.html') < resultText.indexOf('flaky-fail.html'));
    }, 'crash and flaky fail test order');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['expected-missing.html'] = mockExpectation('MISSING', 'MISSING');
    subtree['expected-missing.html'].is_missing_text = true;
    subtree['expected-missing.html'].is_missing_image = true;
    subtree['unexpected-missing.html'] = mockExpectation('PASS', 'MISSING');
    subtree['unexpected-missing.html'].is_missing_text = true;
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));
        assertTrue(visibleExpandLinks().length == 1);
        assertTrue(document.querySelector('#missing-table tbody.expected .test-link').textContent == 'foo/expected-missing.html');
        assertTrue(document.querySelector('#missing-table tbody.expected').getElementsByClassName('result-link')[0].textContent == 'result');
        assertTrue(document.querySelector('#missing-table tbody.expected').getElementsByClassName('result-link')[1].textContent == 'png result');
        assertTrue(document.querySelector('#missing-table tbody:not(.expected) .test-link').textContent == 'foo/unexpected-missing.html');
        assertTrue(document.querySelector('#missing-table tbody:not(.expected) .result-link').textContent == 'result');

        document.getElementById('show-expected-failures').checked = true;
        document.getElementById('show-expected-failures').onchange();
        expandAllExpectations();
        assertTrue(visibleExpandLinks().length == 2);
    }, 'missing results table');

    results = mockResults();
    subtree = results.tests['foo'] = {}
    subtree['bar.html'] = mockExpectation('TEXT', 'FAIL');
    subtree['bar1.html'] = mockExpectation('TEXT', 'FAIL');
    subtree['bar2.html'] = mockExpectation('TEXT', 'FAIL');

    runTest(results, function() {
        if (window.eventSender) {
            eventSender.keyDown('k'); // previous
            var testRows = document.querySelectorAll('#results-table tbody');
            assertTrue(!testRows[0].classList.contains('current'));
            assertTrue(!testRows[1].classList.contains('current'));
            assertTrue(testRows[2].classList.contains('current'));
        }
    }, 'keyboard shortcuts: prev');

    runTest(results, function() {
        if (window.eventSender) {
            eventSender.keyDown('j'); // next
            var testRows = document.querySelectorAll('#results-table tbody');
            assertTrue(testRows[0].classList.contains('current'));
            assertTrue(!testRows[1].classList.contains('current'));
            assertTrue(!testRows[2].classList.contains('current'));
        }
    }, 'keyboard shortcuts: next');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'FAIL');
    subtree['bar1.html'] = mockExpectation('TEXT', 'FAIL');
    subtree['bar2.html'] = mockExpectation('TEXT', 'FAIL');
    subtree['bar3.html'] = mockExpectation('TEXT', 'PASS');
    subtree['bar4.html'] = mockExpectation('IMAGE', 'PASS');

    runTest(results, function() {
        assertTrue(document.getElementById('results-table'));
        assertTrue(visibleExpandLinks().length == 3);

        if (window.eventSender) {
            eventSender.keyDown('i', ["metaKey"]);
            eventSender.keyDown('i', ["shiftKey"]);
            eventSender.keyDown('i', ["ctrlKey"]);
            var testRows = document.querySelectorAll('tbody');
            assertTrue(!testRows[0].classList.contains('current'));
            assertTrue(!testRows[1].classList.contains('current'));
            assertTrue(!testRows[2].classList.contains('current'));

            eventSender.keyDown('i'); // first
            assertTrue(testRows[0].classList.contains('current'));
            assertTrue(!testRows[1].classList.contains('current'));
            assertTrue(!testRows[2].classList.contains('current'));

            eventSender.keyDown('j', ["metaKey"]);
            eventSender.keyDown('j', ["shiftKey"]);
            eventSender.keyDown('j', ["ctrlKey"]);
            assertTrue(testRows[0].classList.contains('current'));
            assertTrue(!testRows[1].classList.contains('current'));
            assertTrue(!testRows[2].classList.contains('current'));

            eventSender.keyDown('j'); // next
            assertTrue(!testRows[0].classList.contains('current'));
            assertTrue(testRows[1].classList.contains('current'));
            assertTrue(!testRows[2].classList.contains('current'));

            eventSender.keyDown('k', ["metaKey"]);
            eventSender.keyDown('k', ["shiftKey"]);
            eventSender.keyDown('k', ["ctrlKey"]);
            assertTrue(!testRows[0].classList.contains('current'));
            assertTrue(testRows[1].classList.contains('current'));
            assertTrue(!testRows[2].classList.contains('current'));

            eventSender.keyDown('k'); // previous
            assertTrue(testRows[0].classList.contains('current'));
            assertTrue(!testRows[1].classList.contains('current'));
            assertTrue(!testRows[2].classList.contains('current'));

            eventSender.keyDown('l', ["metaKey"]);
            eventSender.keyDown('l', ["shiftKey"]);
            eventSender.keyDown('l', ["ctrlKey"]);
            assertTrue(testRows[0].classList.contains('current'));
            assertTrue(!testRows[1].classList.contains('current'));
            assertTrue(!testRows[2].classList.contains('current'));

            eventSender.keyDown('l'); // last
            assertTrue(!testRows[0].classList.contains('current'));
            assertTrue(!testRows[1].classList.contains('current'));
            assertTrue(testRows[4].classList.contains('current'));

            var flaggedTestsTextbox = document.getElementById('flagged-tests');

            eventSender.keyDown('f'); // flag
            assertTrue(flaggedTestsTextbox.innerText == 'foo/bar4.html');
            eventSender.keyDown('f'); // unflag

            eventSender.keyDown('i'); // first

            eventSender.keyDown('e', ["metaKey"]);
            eventSender.keyDown('e', ["shiftKey"]);
            eventSender.keyDown('e', ["ctrlKey"]);
            var expandLinks = document.querySelectorAll('.expand-button-text');
            assertTrue(!isExpanded(expandLinks[0]));

            eventSender.keyDown('e'); // expand
            assertTrue(isExpanded(expandLinks[0]));

            eventSender.keyDown('c', ["metaKey"]);
            eventSender.keyDown('c', ["shiftKey"]);
            eventSender.keyDown('c', ["ctrlKey"]);
            assertTrue(isExpanded(expandLinks[0]));

            eventSender.keyDown('c'); // collapse
            assertTrue(isCollapsed(expandLinks[0]));

            eventSender.keyDown('f', ["metaKey"]);
            eventSender.keyDown('f', ["shiftKey"]);
            eventSender.keyDown('f', ["ctrlKey"]);
            assertTrue(flaggedTestsTextbox.innerText == '');

            eventSender.keyDown('f'); // flag
            assertTrue(flaggedTestsTextbox.innerText == 'foo/bar.html');

            eventSender.keyDown('j'); // next
            eventSender.keyDown('f'); // flag
            assertTrue(flaggedTestsTextbox.innerText == 'foo/bar.html\nfoo/bar1.html');

            document.getElementById('use-newlines').checked = false;
            TestNavigator.updateFlaggedTestTextBox();
            assertTrue(flaggedTestsTextbox.innerText == 'foo/bar.html foo/bar1.html');

            eventSender.keyDown('f'); // unflag
            assertTrue(flaggedTestsTextbox.innerText == 'foo/bar.html');

            eventSender.keyDown('k'); // previous
            eventSender.keyDown('f'); // flag
            assertTrue(flaggedTestsTextbox.innerText == '');
        }
    }, 'keyboard shortcuts');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'PASS');
    subtree['bar-1.html'] = mockExpectation('TEXT', 'CRASH');
    subtree['bar-2.html'] = mockExpectation('PASS', 'IMAGE');
    subtree['bar-3.html'] = mockExpectation('PASS', 'TEXT');
    subtree['bar-4.html'] = mockExpectation('TEXT', 'TEXT');
    subtree['bar-5.html'] = mockExpectation('TEXT', 'IMAGE+TEXT');
    subtree['bar-stderr-expected.html'] = mockExpectation('IMAGE', 'IMAGE');
    subtree['bar-stderr-expected.html'].has_stderr = true;
    subtree['bar-expected-timeout.html'] = mockExpectation('TIMEOUT', 'TIMEOUT');
    subtree['bar-expected-crash.html'] = mockExpectation('CRASH', 'CRASH');
    subtree['bar-missing.html'] = mockExpectation('TEXT', 'MISSING');
    subtree['bar-missing.html'].is_missing_text = true;
    runTest(results, function() {
        var titles = document.getElementsByTagName('h1');
        assertTrue(titles[0].textContent == 'Tests that crashed (1): [flag all] [unflag all]');
        assertTrue(titles[1].textContent == 'Tests that failed text/pixel/audio diff (3): [flag all] [unflag all]');
        assertTrue(titles[2].textContent == 'Tests that had no expected results (probably new) (1): [flag all] [unflag all]');
        assertTrue(titles[3].textContent == 'Tests that timed out (0): [flag all] [unflag all]');
        assertTrue(titles[4].textContent == 'Tests that had stderr output (1): [flag all] [unflag all]');
        assertTrue(titles[5].textContent == 'Tests expected to fail but passed (1): [flag all] [unflag all]');

        document.getElementById('show-expected-failures').checked = true;
        document.getElementById('show-expected-failures').onchange();

        assertTrue(titles[0].textContent == 'Tests that crashed (2): [flag all] [unflag all]');
        assertTrue(titles[1].textContent == 'Tests that failed text/pixel/audio diff (5): [flag all] [unflag all]');
        assertTrue(titles[2].textContent == 'Tests that had no expected results (probably new) (1): [flag all] [unflag all]');
        assertTrue(titles[3].textContent == 'Tests that timed out (1): [flag all] [unflag all]');
        assertTrue(titles[4].textContent == 'Tests that had stderr output (1): [flag all] [unflag all]');
        assertTrue(titles[5].textContent == 'Tests expected to fail but passed (1): [flag all] [unflag all]');
    }, 'table titles 1');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('', 'PASS TEXT');
    subtree['bar-1.html'] = mockExpectation('', 'TEXT IMAGE');
    subtree['bar-2.html'] = mockExpectation('IMAGE TEXT', 'TEXT IMAGE');
    subtree['bar-3.html'] = mockExpectation('PASS TEXT', 'TEXT PASS');
    runTest(results, function() {
        var titles = document.getElementsByTagName('h1');
        assertTrue(titles[0].textContent == 'Tests that failed text/pixel/audio diff (1): [flag all] [unflag all]');
        assertTrue(titles[1].textContent == 'Flaky tests (failed the first run and passed on retry) (1): [flag all] [unflag all]');

        assertTrue(document.querySelectorAll('#results-table tbody').length == 2);
        assertTrue(document.querySelectorAll('#flaky-tests-table tbody').length == 2);
    }, 'table titles 2');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'IMAGE');
    subtree['bar1.html'] = mockExpectation('TEXT', 'TEXT');
    subtree['bar2.html'] = mockExpectation('TEXT', 'TEXT');
    runTest(results, function() {
        flagAll(document.querySelector('.flag-all'));
        var flaggedTestsTextbox = document.getElementById('flagged-tests');
        assertTrue(flaggedTestsTextbox.innerText == 'foo/bar.html');

        document.getElementById('show-expected-failures').checked = true;
        document.getElementById('show-expected-failures').onchange();

        flagAll(document.querySelector('.flag-all'));
        assertTrue(flaggedTestsTextbox.innerText == 'foo/bar.html\nfoo/bar1.html\nfoo/bar2.html');

        unflag(document.querySelector('.flag'));
        assertTrue(flaggedTestsTextbox.innerText == 'foo/bar1.html\nfoo/bar2.html');
    }, 'flagging tests');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'IMAGE');
    subtree['bar1.html'] = mockExpectation('TEXT', 'IMAGE');
    runTest(results, function() {
        flagAll(document.querySelector('.flag-all'));
        var flaggedTestsTextbox = document.getElementById('flagged-tests');
        assertTrue(flaggedTestsTextbox.innerText == 'foo/bar.html\nfoo/bar1.html');
    }, 'flagged tests with newlines', '{"use-newlines":true}');

    results = mockResults();
    subtree = results.tests['foo'] = {};
    subtree['bar.html'] = mockExpectation('TEXT', 'IMAGE');
    subtree['bar1.html'] = mockExpectation('TEXT', 'IMAGE');
    runTest(results, function() {
        flagAll(document.querySelector('.flag-all'));
        var flaggedTestsTextbox = document.getElementById('flagged-tests');
        assertTrue(flaggedTestsTextbox.innerText == 'foo/bar.html foo/bar1.html');
    }, 'flagged tests with spaces', '{"use-newlines":false}');

    results = mockResults();
    results.tests['foo/bar-image.html'] = mockExpectation('PASS', 'TEXT IMAGE+TEXT');
    results.pixel_tests_enabled = false;
    runTest(results, function() {
        assertTrue(document.querySelector('tbody td:nth-child(3) a').getAttribute('href') == 'retries/foo/bar-image-diffs.html');
    }, 'image diff links with IMAGE+TEXT result');

    results = mockResults();
    results.tests['foo/bar-image.html'] = mockExpectation('PASS', 'TEXT IMAGE');
    results.pixel_tests_enabled = false;
    runTest(results, function() {
        assertTrue(!document.getElementById('results-table'));
        assertTrue(document.querySelector('#flaky-tests-table td:nth-child(3) a').getAttribute('href') == 'retries/foo/bar-image-diffs.html');
    }, 'image diff links flaky TEXT IMAGE result');

    results = mockResults();
    results.tests['foo'] = mockExpectation('PASS', 'TEXT');
    results.tests['foo'].has_repaint_overlay = true;
    runTest(results, function() {
        assertTrue(document.querySelector('tbody td:nth-child(2)').textContent.indexOf('overlay') != -1);
    }, 'repaint overlay');

    document.body.innerHTML = '<pre>' + g_log.join('\n') + '</pre>';
}

var originalGeneratePage = generatePage;
generatePage = runTests;
