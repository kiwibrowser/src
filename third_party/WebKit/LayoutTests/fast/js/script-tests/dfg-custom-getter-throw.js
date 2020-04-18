description(
"Tests that DFG custom getter caching does not break the world if the getter throws an exception."
);

function foo(x) {
    return x.responseText;
}

function bar(binary) {
    var x = new XMLHttpRequest();
    x.open("GET", "http://foo.bar.com/");
    if (binary)
        x.responseType = "arraybuffer";
    try {
        return "Returned result: " + foo(x);
    } catch (e) {
        return "Threw exception: " + e;
    }
}

for (var i = 0; i < 200; ++i) {
    shouldBe("bar(i >= 100)", i >= 100 ? "\"Threw exception: InvalidStateError: Failed to read the 'responseText' property from 'XMLHttpRequest': The value is only accessible if the object's 'responseType' is '' or 'text' (was 'arraybuffer').\"" : "\"Returned result: \"");
}


