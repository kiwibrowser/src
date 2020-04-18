function foo()
{
    throw new Error();
}

function boo1()
{
    foo();
}

function boo2()
{
    foo();
}

onmessage = function(event) {
    if (event.data === 42)
        boo1();
    else
        boo2();
};
