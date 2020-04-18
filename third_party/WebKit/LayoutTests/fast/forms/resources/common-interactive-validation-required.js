var queryValues = {};

function testInteractiveValidationRequired(config) {
    description("Test interactive validation with required attribute. This test checks if an empty required field prevents form submission and checks if a non-empty required field doesn't prevent form submission.");

    var expectedValue = config['expectedValue'];

    var keyValuePairs = window.location.search.replace('?', '').split('&');
    for (var index = 0; index < keyValuePairs.length; ++index) {
        var keyValue = keyValuePairs[index].split('=');
        queryValues[keyValue[0]] = unescape(keyValue[1]);
    }

    if (queryValues['submitted']) {
        shouldBeEqualToString('queryValues["test"]', expectedValue);
        finishJSTest();
        return;
    }

    var form = document.createElement("form");
    form.setAttribute("action", window.location);
    form.innerHTML = '<input type=hidden name=submitted value=1><input  id=submit type=submit><input id=test name=test type=' + config['inputType'] + ' required>';
    document.body.appendChild(form);

    debug('Submit without required value');
    document.getElementById('submit').click();

    if (document.activeElement.id != 'test') {
        testFailed('Focus should be on test element.');
        finishJSTest();
        return;
    }

    debug('Submit with required value');
    document.getElementById('test').value = expectedValue;
    document.getElementById('submit').click();
    setTimeout(function() {
        // This is executed only if the test runs not as expected.
        testFailed('The form was not submitted.');
        finishJSTest();
    }, 1000);
}

jsTestIsAsync = true;
wasPostTestScriptParsed = true;
