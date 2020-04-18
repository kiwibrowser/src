if (window.testRunner)
    testRunner.dumpAsText();

var testStyles = document.createElement('style');
testStyles.innerText = ".test-pass { color: green; } .test-fail { color: red; }";
document.querySelector('head').appendChild(testStyles);

var g_testIndex = 0;
var g_log = ["You should see a series of PASS lines."];
function mockResults()
{
    return {
        result_links: [],
        tests: {}
    };
}
function mockArchivedResults(results_set)
{
    return {
        archived_results : results_set
    }
}
function logPass(msg)
{
    g_log.push('TEST-' + g_testIndex + ': <span class="test-pass">' + msg + '</span>')
}

function logFail(msg)
{
    g_log.push('TEST-' + g_testIndex + ': <span class="test-fail">' + msg + '</span>')
}

function assertTrue(bool)
{
    if (bool)
        logPass('PASS');
    else
        logFail('FAIL');
}
function runTest(results, assertions, opt_localStorageValue)
{
    document.body.innerHTML = '';
    g_testIndex++;
    g_state = undefined;
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
function runTests()
{
    var results = mockResults();
    results.result_links.push('dir1/results.html');
    results.result_links.push('dir2/results.html');
    results.result_links.push('dir3/results.html');
    results.tests['foo-1.html'] = mockArchivedResults(['PASS', 'FAIL', 'SKIP']);
    results.tests['foo-2.html'] = mockArchivedResults(['FAIL', 'PASS', 'PASS']);
    var subtree = results.tests["virtual"] = {};
    subtree["foo-3.html"] = mockArchivedResults(['SKIP', 'PASS', 'SKIP']);

    var result = '';
    runTest(results, function() {
        var table = document.getElementById('results-table');
        assertTrue(table.rows.length == 5);
        assertTrue(table.rows[2].cells.length == 5);
        assertTrue(table.rows[2].cells[1].innerHTML == 'foo-1.html');
        assertTrue(table.rows[4].cells[1].innerHTML == 'virtual/foo-3.html');
        assertTrue(table.rows[2].cells[2].className == 'test-pass');
        assertTrue(table.rows[2].cells[3].className == 'test-fail');
        assertTrue(table.rows[2].cells[4].className == 'test-skip');
        var row = table.rows[2];
        var dummyhref = document.createElement("a");
        dummyhref.href = 'dir1/results.html';
        assertTrue(row.cells[2].getElementsByTagName('a')[0] == dummyhref.href);
        dummyhref.href = 'dir2/results.html';
        assertTrue(row.cells[3].getElementsByTagName('a')[0] == dummyhref.href);
        dummyhref.href = 'dir3/results.html';
        assertTrue(row.cells[4].getElementsByTagName('a')[0] == dummyhref.href);
    });
    document.body.innerHTML = '<pre>' + g_log.join('\n') + '</pre>';
}

var originalGeneratePage = generatePage;
generatePage = runTests;
