description('Test that setting and getting grid-template-columns and grid-template-rows works as expected');

debug("Test getting |grid-template-columns| and |grid-template-rows| set through CSS");
testGridDefinitionsValues(document.getElementById("gridWithFixedElement"), "7px 11px", "17px 2px");
testGridDefinitionsValues(document.getElementById("gridWithPercentElement"), "400px 800px", "162px 312px");
testGridDefinitionsValues(document.getElementById("gridWithPercentWithoutSize"), "3.5px 7px", "11px 0px");
testGridDefinitionsValues(document.getElementById("gridWithAutoElement"), "0px 17px", "0px 3px");
testGridDefinitionsValues(document.getElementById("gridWithEMElement"), "100px 120px", "150px 170px");
testGridDefinitionsValues(document.getElementById("gridWithThreeItems"), "15px 0px 100px", "120px 18px 0px");
testGridDefinitionsValues(document.getElementById("gridWithPercentAndViewportPercent"), "400px 120px", "210px 168px");
testGridDefinitionsValues(document.getElementById("gridWithFitContentAndFitAvailable"), "none", "none");
testGridDefinitionsValues(document.getElementById("gridWithMinMaxContent"), "0px 0px", "0px 0px");
testGridDefinitionsValues(document.getElementById("gridWithMinMaxContentWithChildrenElement"), "7px 17px", "11px 3px");
testGridDefinitionsValues(document.getElementById("gridWithMinMaxAndFixed"), "240px 15px", "120px 210px");
testGridDefinitionsValues(document.getElementById("gridWithMinMaxAndMinMaxContent"), "240px 15px", "120px 210px");
testGridDefinitionsValues(document.getElementById("gridWithFractionFraction"), "320px 480px", "225px 375px");
testGridDefinitionsValues(document.getElementById("gridWithFractionMinMax"), "45px 755px", "586px 14px");
testGridDefinitionsValues(document.getElementById("gridWithCalcCalc"), "200px 100px", "150px 75px");
testGridDefinitionsValues(document.getElementById("gridWithCalcAndFixed"), "400px 80px", "88px 150px");
testGridDefinitionsValues(document.getElementById("gridWithCalcAndMinMax"), "190px 80px", "150px 53px");
testGridDefinitionsValues(document.getElementById("gridWithCalcInsideMinMax"), "400px 120px", "150px 175px");
testGridDefinitionsValues(document.getElementById("gridWithAutoInsideMinMax"), "0px 30px", "132px 60px");

debug("");
debug("Test the initial value");
var element = document.createElement("div");
document.body.appendChild(element);
shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-columns')", "'none'");
shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-rows')", "'none'");

debug("");
debug("Test getting and setting grid-template-rows and grid-template-columns through JS");
testGridDefinitionsSetJSValues("18px 22px", "66px 70px");
testGridDefinitionsSetJSValues("55% 80%", "40% 63%", "440px 640px", "240px 378px");
testGridDefinitionsSetJSValues("auto auto", "auto auto", "0px 0px", "0px 0px");
testGridDefinitionsSetJSValues("auto 16em 22px", "56% 10em auto", "0px 160px 22px", "336px 100px 0px");
testGridDefinitionsSetJSValues("16em minmax(16px, 20px)", "minmax(10%, 15%) auto", "160px 20px", "90px 0px");
testGridDefinitionsSetJSValues("16em 2fr", "14fr auto", "160px 640px", "600px 0px");
testGridDefinitionsSetJSValues("50% 12vw", "5% 85vh", "400px 96px", "30px 510px");
testGridDefinitionsSetJSValues("calc(25px) calc(2em)", "auto calc(10%)", "25px 20px", "0px 60px", "calc(25px) calc(2em)", "auto calc(10%)");
testGridDefinitionsSetJSValues("calc(25px + 40%) minmax(min-content, calc(10% + 12px))", "minmax(calc(75% - 350px), max-content) auto", "345px 92px", "100px 0px", "calc(25px + 40%) minmax(min-content, calc(10% + 12px))", "minmax(calc(75% - 350px), max-content) auto");
testGridDefinitionsSetJSValues("auto minmax(16px, auto)", "minmax(auto, 15%) 10vw", "0px 16px", "90px 80px");

debug("");
debug("Test getting wrong values set from CSS");
var gridWithNoneAndAuto = document.getElementById("gridWithNoneAndAuto");
shouldBe("getComputedStyle(gridWithNoneAndAuto, '').getPropertyValue('grid-template-columns')", "'none'");
shouldBe("getComputedStyle(gridWithNoneAndAuto, '').getPropertyValue('grid-template-rows')", "'none'");

var gridWithNoneAndFixed = document.getElementById("gridWithNoneAndFixed");
shouldBe("getComputedStyle(gridWithNoneAndFixed, '').getPropertyValue('grid-template-columns')", "'none'");
shouldBe("getComputedStyle(gridWithNoneAndFixed, '').getPropertyValue('grid-template-rows')", "'none'");

debug("");
debug("Test setting and getting wrong values from JS");
testGridDefinitionsSetBadJSValues("none auto", "none auto");
testGridDefinitionsSetBadJSValues("none 16em", "none 56%");
testGridDefinitionsSetBadJSValues("none none", "none none");
testGridDefinitionsSetBadJSValues("auto none", "auto none");
testGridDefinitionsSetBadJSValues("auto none 16em", "auto 18em none");
testGridDefinitionsSetBadJSValues("-webkit-fit-content -webkit-fit-content", "-webkit-fit-available -webkit-fit-available");
// Negative values are not allowed.
testGridDefinitionsSetBadJSValues("-10px minmax(16px, 32px)", "minmax(10%, 15%) -10vw");
testGridDefinitionsSetBadJSValues("10px minmax(16px, -1vw)", "minmax(-1%, 15%) 10vw");
// Invalid expressions with calc
testGridDefinitionsSetBadJSValues("10px calc(16px 30px)", "calc(25px + auto) 2em");
testGridDefinitionsSetBadJSValues("minmax(min-content, calc() 250px", "calc(2em(");

function testInherit()
{
    var parentElement = document.createElement("div");
    document.body.appendChild(parentElement);
    parentElement.style.display = "grid";
    parentElement.style.width = "800px";
    parentElement.style.height = "600px";
    parentElement.style.font = "10px Ahem"; // Used to resolve em font consistently.
    parentElement.style.gridTemplateColumns = "50px 1fr [last]";
    parentElement.style.gridTemplateRows = "2em [middle] 45px";
    testGridDefinitionsValues(parentElement, "50px 750px [last]", "20px [middle] 45px");

    element = document.createElement("div");
    parentElement.appendChild(element);
    element.style.display = "grid";
    element.style.gridTemplateColumns = "inherit";
    element.style.gridTemplateRows = "inherit";
    testGridDefinitionsValues(element, "50px 0px [last]", "20px [middle] 45px");

    document.body.removeChild(parentElement);
}
debug("");
debug("Test setting grid-template-columns and grid-template-rows to 'inherit' through JS");
testInherit();

function testInitial()
{
    element = document.createElement("div");
    document.body.appendChild(element);
    element.style.display = "grid";
    element.style.width = "800px";
    element.style.height = "600px";
    element.style.gridTemplateColumns = "150% [middle] 55px";
    element.style.gridTemplateRows = "1fr [line] 2fr [line]";
    testGridDefinitionsValues(element, "1200px [middle] 55px", "200px [line] 400px [line]");

    element.style.gridTemplateColumns = "initial";
    element.style.gridTemplateRows = "initial";
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-columns')", "'none'");
    shouldBe("getComputedStyle(element, '').getPropertyValue('grid-template-rows')", "'none'");

    document.body.removeChild(element);
}
debug("");
debug("Test setting grid-template-columns and grid-template-rows to 'initial' through JS");
testInitial();
