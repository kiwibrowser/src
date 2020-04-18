<!DOCTYPE html>
<script src=../../../resources/js-test.js>></script>
<script src=../../forms/resources/picker-common.js>></script>
<input type=datetime-local id=datetimelocal1>
<script>
description('Check if changing type on input event handler doesn\'t crash. This requires ASAN build.');
function handleInput(event) {
    this.removeAttribute('type');
    setTimeout(function() {
        testPassed('if not crash');
        finishJSTest();
    }, 0);
}

datetimelocal1.addEventListener('input', handleInput, false);
openPicker(datetimelocal1, test1);

function test1() {
    eventSender.keyDown('ArrowLeft');
    eventSender.keyDown('Enter');
    waitUntilClosing(test1AfterClosing);
}

function test1AfterClosing() {
    datetimelocal1.value = '2013-01-21T17:49';
}
</script>
