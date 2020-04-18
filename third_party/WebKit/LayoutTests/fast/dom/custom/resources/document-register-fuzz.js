
function setupObjectHooks(hooks)
{
    // Wrapper for these object should be materialized before setting hooks.
    console.log;
    document.register;
    HTMLSpanElement.prototype;

    Object.defineProperty(Object.prototype, "prototype", {
        get: function() { return hooks.prototypeGet(); },
        set: function(value) { return hooks.prototypeSet(value); }
    });

    Object.defineProperty(Object.prototype, "constructor", {
        get: function() { return hooks.constructorGet(); },
        set: function(value) { return hooks.constructorSet(value); }
    });

    return hooks;
}

function exerciseDocumentRegister()
{
    register('x-a', {});
    register('x-b', {prototype: Object.create(HTMLElement.prototype)});
}

function register(name, options)
{
    var myConstructor = null;
    try {
        myConstructor = document.registerElement(name, options);
    } catch (e) { }

    try {
        if (!myConstructor) {
            debug("Constructor object isn't created.");
            return;
        }

        if (options.prototype !== undefined && myConstructor.prototype != options.prototype) {
            console.log("FAIL: bad prototype");
            return;
         }

        var element = new myConstructor();
        if (!element)
            return;
        if (element.constructor != myConstructor) {
            console.log("FAIL: bad constructor");
            return;
         }
    } catch (e) { console.log(e); }
}
