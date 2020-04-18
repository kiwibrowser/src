function f2()
{
    return 1;
}

function f1()
{
    var x = 1;
    return x + f2();
}
