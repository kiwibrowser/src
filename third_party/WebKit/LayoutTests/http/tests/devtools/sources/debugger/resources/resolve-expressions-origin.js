function foo(parameter1, parameter2)
{
    var object =  new ClassA();
    object.prop1 = parameter1;
    this.prop2 = parameter2;
    object["prop3"] = "property";
    debugger;
    return object.prop1 + this.prop2;
}

function testFunction()
{
    foo.call({}, "param1", "param2");
}

function ClassA()
{
}