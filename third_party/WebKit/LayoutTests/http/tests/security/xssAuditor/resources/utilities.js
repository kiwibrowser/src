function checkIfFrameLocationMatchesSrcAndCallDone(frameId)
{
    if (!window.testRunner)
        return;

    var actualURL = 'unavailable', frame = document.getElementById(frameId);
    try {
        actualURL = frame.contentWindow.location.href;
    }
    catch (e) {
        actualURL = '[Location object access threw exception]';
    }

    if (actualURL != frame.src)
        alert('URL mismatch: \'' + actualURL + '\' vs. \'' + frame.src + '\'');

    testRunner.notifyDone();
}

function checkFrameIsCrossOriginAndCallDone(frameId) {
    if (!window.testRunner)
        return;

    var actualURL = 'unavailable', frame = document.getElementById(frameId);
    try {
        actualURL = frame.contentWindow.location.href;
        console.log('FAIL: "' + frameId + '" loaded "' + actualURL + '".');
    }
    catch (e) {
        actualURL = '[Location object access threw exception]';
        console.log('PASS: "' + frameId + '" is cross-origin.');
    }

    testRunner.notifyDone();
}

function sendRequestFromIFrame(url, params, HTTPMethod, callbackWhenDone)
{
    if (!params || !params.length)
        return;
        
    if (!HTTPMethod)
        HTTPMethod = 'GET';
        
    if (document.getElementById('frame'))
        document.body.removeChild(document.getElementById('frame'));
    var iFrameObj = document.createElement('iframe');
    iFrameObj.name = 'frame';
    document.body.appendChild(iFrameObj);
    var frameContent = iFrameObj.contentDocument;
    frameContent.open();
    frameContent.write('<form method="' + HTTPMethod + '" name="form" id="form" action="' + url + '">');
    if (params.length > 0) {
        var paramArray = params.split('&');
        for (var i = 0; i < paramArray.length; ++i) {
            var paramElement = paramArray[i].split('=', 2);
            frameContent.write('<input type="text" name="' + paramElement[0] + '" value="' + paramElement[1] + '">');
        }   
    }
    frameContent.write('</form>');
    frameContent.close();
    if (callbackWhenDone)
        iFrameObj.onload = callbackWhenDone;
    frameContent.getElementById('form').submit();
}


function notifyDoneAfterReceivingLoaded()
{
    window.addEventListener("message", function(event) {
        if (event.data == "loaded")
            testRunner.notifyDone();
    }, false);
}

