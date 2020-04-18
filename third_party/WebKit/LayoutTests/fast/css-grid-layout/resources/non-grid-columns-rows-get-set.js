description('Test that setting and getting grid-template-columns and grid-template-rows works as expected');

debug("Test getting grid-template-columns and grid-template-rows set through CSS");
testGridDefinitionsValues(document.getElementById("gridWithNoneElement"), "none", "none");
testGridDefinitionsValues(document.getElementById("gridWithFixedElement"), "10px", "15px");
testGridDefinitionsValues(document.getElementById("gridWithPercentElement"), "53%", "27%");
testGridDefinitionsValues(document.getElementById("gridWithAutoElement"), "auto", "auto");
testGridDefinitionsValues(document.getElementById("gridWithEMElement"), "100px", "150px");
testGridDefinitionsValues(document.getElementById("gridWithViewPortPercentageElement"), "64px", "60px");
testGridDefinitionsValues(document.getElementById("gridWithMinMax"), "minmax(10%, 15px)", "minmax(20px, 50%)");
testGridDefinitionsValues(document.getElementById("gridWithMinContent"), "min-content", "min-content");
testGridDefinitionsValues(document.getElementById("gridWithMaxContent"), "max-content", "max-content");
testGridDefinitionsValues(document.getElementById("gridWithFraction"), "1fr", "2fr");
testGridDefinitionsValues(document.getElementById("gridWithCalc"), "150px", "75px");
testGridDefinitionsValues(document.getElementById("gridWithCalcComplex"), "calc(150px + 50%)", "calc(75px + 65%)");
testGridDefinitionsValues(document.getElementById("gridWithCalcInsideMinMax"), "minmax(10%, 15px)", "minmax(20px, 50%)");
testGridDefinitionsValues(document.getElementById("gridWithCalcComplexInsideMinMax"), "minmax(10%, calc(15px + 50%))", "minmax(calc(20px + 10%), 50%)");
testGridDefinitionsValues(document.getElementById("gridWithAutoInsideMinMax"), "minmax(auto, 20px)", "minmax(min-content, auto)");

debug("");
debug("Test getting wrong values for grid-template-columns and grid-template-rows through CSS (they should resolve to the default: 'none')");
var gridWithFitContentElement = document.getElementById("gridWithFitContentElement");
testGridDefinitionsValues(gridWithFitContentElement, "none", "none");

var gridWithFitAvailableElement = document.getElementById("gridWithFitAvailableElement");
testGridDefinitionsValues(gridWithFitAvailableElement, "none", "none");

debug("");
debug("Test the initial value");
var element = document.createElement("div");
document.body.appendChild(element);
testGridDefinitionsValues(element, "none", "none");
shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-columns')", "'none'");
shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-rows')", "'none'");

debug("");
debug("Test getting and setting grid-template-columns and grid-template-rows through JS");
testNonGridDefinitionsSetJSValues("18px", "66px");
testNonGridDefinitionsSetJSValues("55%", "40%");
testNonGridDefinitionsSetJSValues("auto", "auto");
testNonGridDefinitionsSetJSValues("10vw", "25vh", "80px", "150px");
testNonGridDefinitionsSetJSValues("min-content", "min-content");
testNonGridDefinitionsSetJSValues("max-content", "max-content");

debug("");
debug("Test getting and setting grid-template-columns and grid-template-rows to minmax() values through JS");
testNonGridDefinitionsSetJSValues("minmax(55%, 45px)", "minmax(30px, 40%)");
testNonGridDefinitionsSetJSValues("minmax(22em, 8vh)", "minmax(10vw, 5em)", "minmax(220px, 48px)", "minmax(80px, 50px)");
testNonGridDefinitionsSetJSValues("minmax(min-content, 8vh)", "minmax(10vw, min-content)", "minmax(min-content, 48px)", "minmax(80px, min-content)");
testNonGridDefinitionsSetJSValues("minmax(22em, max-content)", "minmax(max-content, 5em)", "minmax(220px, max-content)", "minmax(max-content, 50px)");
testNonGridDefinitionsSetJSValues("minmax(min-content, max-content)", "minmax(max-content, min-content)");
// Unit comparison should be case-insensitive.
testNonGridDefinitionsSetJSValues("3600Fr", "154fR", "3600fr", "154fr", "3600fr", "154fr");
// Float values are allowed.
testNonGridDefinitionsSetJSValues("3.1459fr", "2.718fr");
// A leading '+' is allowed.
testNonGridDefinitionsSetJSValues("+3fr", "+4fr", "3fr", "4fr", "3fr", "4fr");
testNonGridDefinitionsSetJSValues("minmax(auto, 8vh)", "minmax(10vw, auto)", "minmax(auto, 48px)", "minmax(80px, auto)");
// Flex factor values can be zero.
testGridDefinitionsSetJSValues("0fr", ".0fr", "0px", "0px", "0fr", "0fr");
testGridDefinitionsSetJSValues("minmax(auto, 0fr)", "minmax(auto, .0fr)", "0px", "0px", "minmax(auto, 0fr)", "minmax(auto, 0fr)");

debug("");
debug("Test setting grid-template-columns and grid-template-rows to bad values through JS");
// No comma and only 1 argument provided.
testGridDefinitionsSetBadJSValues("minmax(10px 20px)", "minmax(10px)")
// Nested minmax and only 2 arguments are allowed.
testGridDefinitionsSetBadJSValues("minmax(minmax(10px, 20px), 20px)", "minmax(10px, 20px, 30px)");
// No breadth value and no comma.
testGridDefinitionsSetBadJSValues("minmax()", "minmax(30px 30% 30em)");
testGridDefinitionsSetBadJSValues("-2fr", "3ffr");
testGridDefinitionsSetBadJSValues("-2.05fr", "+-3fr");
testGridDefinitionsSetBadJSValues("1f", "1r");
// A dimension doesn't allow spaces between the number and the unit.
testGridDefinitionsSetBadJSValues(".0001 fr", "13 fr");
testGridDefinitionsSetBadJSValues("7.-fr", "-8,0fr");
// Negative values are not allowed.
testGridDefinitionsSetBadJSValues("-1px", "-6em");
testGridDefinitionsSetBadJSValues("minmax(-1%, 32%)", "minmax(2vw, -6em)");
// Flexible lengths are invalid on the min slot of minmax().
testGridDefinitionsSetBadJSValues("minmax(0fr, 100px)", "minmax(.0fr, 200px)");
testGridDefinitionsSetBadJSValues("minmax(1fr, 100px)", "minmax(2.5fr, 200px)");

debug("");
debug("Test setting grid-template-columns and grid-template-rows back to 'none' through JS");
testNonGridDefinitionsSetJSValues("18px", "66px");
testNonGridDefinitionsSetJSValues("none", "none");

function testInherit()
{
    var parentElement = document.createElement("div");
    document.body.appendChild(parentElement);
    parentElement.style.gridTemplateColumns = "50px [last]";
    parentElement.style.gridTemplateRows = "[first] 101%";

    element = document.createElement("div");
    parentElement.appendChild(element);
    element.style.gridTemplateColumns = "inherit";
    element.style.gridTemplateRows = "inherit";
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-columns')", "'50px [last]'");
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-rows')", "'[first] 101%'");

    document.body.removeChild(parentElement);
}
debug("");
debug("Test setting grid-template-columns and grid-template-rows to 'inherit' through JS");
testInherit();

function testInitial()
{
    element = document.createElement("div");
    document.body.appendChild(element);
    element.style.gridTemplateColumns = "150% [last]";
    element.style.gridTemplateRows = "[first] 1fr";
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-columns')", "'150% [last]'");
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-rows')", "'[first] 1fr'");

    element.style.gridTemplateColumns = "initial";
    element.style.gridTemplateRows = "initial";
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-columns')", "'none'");
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-rows')", "'none'");

    document.body.removeChild(element);
}
debug("");
debug("Test setting grid-template-columns and grid-template-rows to 'initial' through JS");
testInitial();
