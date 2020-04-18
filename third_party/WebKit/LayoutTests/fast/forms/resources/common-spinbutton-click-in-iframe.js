window.jsTestIsAsync = true;

var iframe;
var testInput;

function getSpinButton(input)
{
    if (!window.internals)
        return null;
    return getElementByPseudoId(internals.shadowRoot(input), "-webkit-inner-spin-button");
}

function mouseClick()
{
    if (!window.eventSender)
        return;
    eventSender.mouseDown();
    eventSender.mouseUp();
}

function mouseMoveTo(x, y)
{
    if (!window.eventSender)
        return;
    eventSender.mouseMoveTo(x, y);
}

function runIFrameLoaded(config)
{
    testInput = iframe.contentDocument.getElementById('test');
    testInput.focus();
    var spinButton = getSpinButton(testInput);
    if (spinButton) {
        var rect = spinButton.getBoundingClientRect();
        mouseMoveTo(
            iframe.offsetLeft + rect.left + rect.width / 2,
            iframe.offsetTop + rect.top + rect.height / 4);
    }
    mouseClick();
    shouldBeEqualToString('testInput.value', config['expectedValue']);
    iframe.parentNode.removeChild(iframe);
    finishJSTest();
}

function testClickSpinButtonInIFrame(config)
{
    description('Checks mouse click on spin button in iframe.');
    if (!window.eventSender)
        debug('Please run in DumpRenderTree');

    iframe = document.createElement('iframe');
    iframe.addEventListener('load', function () { runIFrameLoaded(config) });
    iframe.srcdoc = '<input id=test type=' + config['inputType'] + ' value="' + config['initialValue'] + '">';
    document.body.appendChild(iframe);
}
