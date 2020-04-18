var throwOnToStringObject = { };
throwOnToStringObject.toString = function () { throw "Cannot call toString on this object." };

var throwOnGetLengthObject = { };
throwOnGetLengthObject.__defineGetter__("length", function () { throw "Cannot get length of this object."; });

var throwOnGetZeroObject = { length: 1 };
throwOnGetZeroObject.__defineGetter__("0", function () { throw "Cannot get 0 property of this object."; });

function createIterable(iterations) {
    return {
        [Symbol.iterator]() {
            var i = 0;
            return {next: () => iterations[i++]};
        },
    };
}
var emptyIterableObject = createIterable([{done: true}]);
var singleItemIterableObject = createIterable([{done: false, value: "arg0"}, {done: true}]);

var expectNoException = [
    'null',
    'undefined',
    '0',
    '""',
    '"", null',
    '"", undefined',
    '"", []',
    '"", [ "arg0" ]',
    '"", emptyIterableObject',
    '"", singleItemIterableObject',
    '"", null, null',
    '"", null, undefined',
    '"", null, function(){}',
    // SQLStatementCallback will not be invoked at all.
    '"", null, function(){ throw "exception"; }',
    '"", null, { }',
    '"", null, null, null',
    '"", null, null, undefined',
    '"", null, null, function(){}',
    // SQLStatementErrorCallback will not be invoked at all.
    '"", null, null, function(){ throw "exception"; }',
    '"", null, null, { }',
];

var expectException = [
    '',
    'throwOnToStringObject',
    '"", throwOnGetLengthObject',
    '"", throwOnGetZeroObject',
    '"", [ throwOnToStringObject ]',
    '"", 0',
    '"", ""',
    '"", { }',
    '"", { length: 0 }',
    '"", { length: 1, 0: "arg0" }',
    '"", null, 0',
    '"", null, ""',
    '"", null, null, 0',
    '"", null, null, ""',
];

function tryExecuteSql(transaction, parameterList)
{
    try {
        eval('transaction.executeSql(' + parameterList + ')');
        return null;
    } catch (exception) {
        return exception;
    }
}

function runTransactionTest(transaction, parameterList, expectException)
{
    var exception = tryExecuteSql(transaction, parameterList);
    if (expectException) {
        if (exception)
            log("PASS. executeSql(" + parameterList + ") threw an exception as expected.");
        else
            log("*FAIL*. executeSql(" + parameterList + ") did not throw an exception");
    } else {
        if (exception)
            log("*FAIL*. executeSql(" + parameterList + ") threw an exception: " + exception);
        else
            log("PASS. executeSql(" + parameterList + ") did not throw an exception");
    }
}

function runTransactionTests(transaction)
{
    for (i in expectNoException)
        runTransactionTest(transaction, expectNoException[i], false);
    for (i in expectException)
        runTransactionTest(transaction, expectException[i], true);

    if (window.testRunner)
        testRunner.notifyDone();
}

function runTest()
{

    var db = openDatabaseWithSuffix("ExecuteSQLArgsTest", "1.0", "Test of handling of the arguments to SQLTransaction.executeSql", 1);
    db.transaction(runTransactionTests);
}
