description("Test the parsing of the -webkit-image-set function.");

// These have to be global for the test helpers to see them.
var cssRule;

function testInvalidImageSet(description, rule)
{
    debug("");
    debug(description + " : " + rule);

    var div = document.createElement("div");
    div.style.backgroundImage = "-webkit-image-set(" + rule + ")";
    document.body.appendChild(div);

    cssRule = div.style.backgroundImage;
    shouldBeEmptyString("cssRule");

    document.body.removeChild(div);
}

testInvalidImageSet("Too many url parameters", "url(#a #b)");
testInvalidImageSet("No x", "url('#a') 1");
testInvalidImageSet("No comma", "url('#a') 1x url('#b') 2x");
testInvalidImageSet("Too many scale factor parameters", "url('#a') 1x 2x");
testInvalidImageSet("Scale factor is 0", "url('#a') 0x");
testInvalidImageSet("No url function", "'#a' 1x");

successfullyParsed = true;
