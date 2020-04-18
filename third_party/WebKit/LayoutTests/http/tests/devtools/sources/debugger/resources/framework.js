// A framework for testing.

var Framework = {};

Framework.safeRun = function(callback, onSuccess, onException, breakOnUncaught)
{
    try {
        callback();
        if (onSuccess)
            Framework.safeRun(onSuccess, undefined, onException, breakOnUncaught);
    } catch (e) {
        if (onException)
            Framework.safeRun(onException, undefined, breakOnUncaught ? Framework.breakInFramework : undefined);
        else if (breakOnUncaught)
            Framework.breakInFramework();
    }
}

Framework.throwFrameworkException = function(msg)
{
    throw Error("FrameworkException" + (msg ? ": " + msg : ""));
}

Framework.breakInFramework = function()
{
    debugger;
}

Framework.empty = function()
{
}

Framework.doSomeWork = function()
{
    const numberOfSteps = 50;
    for (var i = 0; i < numberOfSteps; ++i) {
        if (window["dummy property should not exist!" + i]) // Prevent optimizations.
            return i;
        Framework.safeRun(Framework.empty, Framework.empty, Framework.empty, true);
    }
}

Framework.schedule = function(callback, delay)
{
    setTimeout(callback, delay || 0);
}

Framework.willSchedule = function(callback, delay)
{
    return function Framework_willSchedule() {
        return Framework.schedule(callback, delay);
    };
}

Framework.doSomeAsyncChainCalls = function(callback)
{
    var func1 = Framework.willSchedule(function Framework_inner1() {
        if (callback)
            callback();
    });
    var func2 = Framework.willSchedule(function Framework_inner2() {
        if (window.callbackFromFramework)
            window.callbackFromFramework(func1);
        else
            func1();
    });
    Framework.schedule(func2);
}

Framework.appendChild = function(parent, child)
{
    parent.appendChild(child);
}

Framework.sendXHR = function(url)
{
    var request = new XMLHttpRequest();
    request.open("GET", url, true);
    try { request.send(); } catch (e) {}
}

Framework.addEventListener = function(element, eventType, listener, capture)
{
    function Framework_eventListener()
    {
        var result = listener ? listener() : void 0;
        return result;
    }

    function Framework_remover()
    {
        element.removeEventListener(eventType, Framework_eventListener, capture);
    }

    element.addEventListener(eventType, Framework_eventListener, capture);
    return Framework_remover;
}

Framework.bind = function(func, thisObject, var_args)
{
    var args = Array.prototype.slice.call(arguments, 2);

    function Framework_bound(var_args)
    {
        return func.apply(thisObject, args.concat(Array.prototype.slice.call(arguments)));
    }
    Framework_bound.toString = function()
    {
        return "Framework_bound: " + func;
    };
    return Framework_bound;
}

Framework.throwInNative = function()
{
    var wrongJson = "})";
    window["dummy"] = JSON.parse(wrongJson);
}

Framework.throwInNativeAndCatch = function()
{
    try {
        Framework.throwInNative();
    } catch(e) {
    }
}

Framework.throwFrameworkExceptionAndCatch = function()
{
    try {
        Framework.throwFrameworkException();
    } catch(e) {
    }
}

Framework.scheduleUntilDone = function(callback, delay)
{
    Framework.schedule(Framework_scheduleUntilDone, delay);

    function Framework_scheduleUntilDone()
    {
        if (callback && callback())
            return;
        Framework.schedule(Framework_scheduleUntilDone, delay);
    }
}

Framework.doSomeWorkDoNotChangeTopCallFrame = function()
{
    const numberOfSteps = 5000;
    for (var i = 0; i < numberOfSteps; ++i) {
        if (window["dummy property should not exist!" + i]) // Prevent optimizations.
            return i;
    }
    return -1;
}

Framework.assert = function(var_args)
{
    var args = Array.prototype.slice.call(arguments, 0);
    return console.assert.apply(console, args);
}

Framework.createButtonWithEventListenersAndClick = function(eventListener)
{
    var button = document.createElement("input");
    button.type = "button";
    Framework.addEventListener(button, "click", Framework.empty, true);
    Framework.addEventListener(button, "click", Framework.bind(Framework.empty, null), false);
    Framework.addEventListener(button, "click", Framework.bind(Framework.safeRun, null, Framework.empty, Framework.empty, Framework.empty), true);
    if (eventListener)
        Framework.addEventListener(button, "click", eventListener, true);
    button.click();
    return button;
}
