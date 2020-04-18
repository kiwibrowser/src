function foo(parameter1, parameter2)
{
    try {
        throw "boom!";
    } catch (error) {
        var longObject = {};
        var longMap = new Map();
        longMap.set(parameter1, parameter2);
        debugger;
        return longMap.get(longObject);
    }
}

function testFunction()
{
    foo(100, "hello");
}