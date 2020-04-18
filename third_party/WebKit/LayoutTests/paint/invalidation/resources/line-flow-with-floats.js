var frameDoc;
var loadCount = 0;

function loaded()
{
    loadCount++;
    if (loadCount == 2)
        runRepaintAndPixelTest();
}

function repaintTest()
{
    test(document.getElementById("iframe").contentDocument);
}
