description("Test the parsing of the -webkit-image-set function.");

var result;

function testImageSetRule(description, property, rule, expectedTexts)
{
    debug("");
    debug(description + " : " + rule);

    var div = document.createElement("div");
    rule = "-webkit-image-set(" + rule + ")";
    div.style[property] = rule;
    document.body.appendChild(div);
    result = div.style[property].replace(/url\("[^#]*#/g, 'url("#');
    shouldBeEqualToString("result", rule);
    document.body.removeChild(div);
}

testImageSetRule("Single value for background-image",
                "background-image",
                'url("#a") 1x');

testImageSetRule("Multiple values for background-image",
                "background-image",
                'url("#a") 1x, url("#b") 2x');

testImageSetRule("Multiple values for background-image, out of order",
                "background-image",
                'url("#c") 3x, url("#b") 2x, url("#a") 1x');

testImageSetRule("Single value for content",
                "content",
                'url("#a") 1x');

testImageSetRule("Multiple values for content",
                "content",
                'url("#a") 1x, url("#b") 2x');

testImageSetRule("Single value for border-image",
                "-webkit-border-image",
                'url("#a") 1x');

testImageSetRule("Multiple values for border-image",
                "-webkit-border-image",
                'url("#a") 1x, url("#b") 2x');

testImageSetRule("Single value for -webkit-mask-box-image-source",
                "-webkit-mask-box-image-source",
                'url("#a") 1x');

testImageSetRule("Multiple values for -webkit-mask-box-image-source",
                "-webkit-mask-box-image-source",
                'url("#a") 1x, url("#b") 2x');

successfullyParsed = true;
