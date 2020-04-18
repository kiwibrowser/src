if (this.importScripts)
    importScripts('../../../resources/js-test.js');

function update() {
    onmessage = undefined;
}
for (var i = 0; i < 8; ++i)
    update();

testPassed("onmessage repeatedly updated ok.");
finishJSTest();
