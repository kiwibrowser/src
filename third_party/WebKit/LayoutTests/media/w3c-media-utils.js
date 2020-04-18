var consoleDiv = null;
function getVideoURI(baseFileName)
{
    var fileExtension =
        (document.createElement("video").
            canPlayType('video/ogg; codecs="theora"') == "") ? "mp4" : "ogv";
    return "./content/" + baseFileName + "." + fileExtension;
}

function consoleWrite(text)
{
    if (!consoleDiv && document.body)
    {
        consoleDiv = document.createElement('div');
        document.body.appendChild(consoleDiv);
    }
    var span = document.createElement("span");
    span.appendChild(document.createTextNode(text));
    span.appendChild(document.createElement('br'));
    consoleDiv.appendChild(span);
}

function waitForEventAndRunStep(eventName, element, func, stepTest)
{
    var eventCallback = function(event)
    {
        consoleWrite("EVENT(" + eventName + ")");
        if (func)
            func(event);
    }
    if (stepTest)
        eventCallback = stepTest.step_func(eventCallback);

    element.addEventListener(eventName, eventCallback, true);
}
