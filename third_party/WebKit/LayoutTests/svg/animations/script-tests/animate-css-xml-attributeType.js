description("Tests that XML and CSS attributeTypes can be switched between.");
createSVGTestCase();

// Setup test document
var polygon = createSVGElement("polygon");
polygon.setAttribute("id", "polygon");
polygon.setAttribute("points", "100 0 200 0 200 100 100 100");
polygon.setAttribute("fill", "green");
polygon.setAttribute("onclick", "executeTest()");

var set = createSVGElement("set");
set.setAttribute("id", "set");
set.setAttribute("attributeName", "points");
set.setAttribute("attributeType", "XML");
set.setAttribute("to", "300 0 400 0 400 100 300 100");
set.setAttribute("begin", "click");
polygon.appendChild(set);
rootSVGElement.appendChild(polygon);

// Setup animation test
function sample1() {
    shouldBeCloseEnough("polygon.animatedPoints.getItem(0).x", "100");
    shouldBe("polygon.points.getItem(0).x", "100");
}

function sample2() {
    shouldBeCloseEnough("polygon.animatedPoints.getItem(0).x", "300");
    // change the animationType to CSS which is invalid.
    set.setAttribute("attributeType", "CSS");
}

function sample3() {
    // verify that the animation resets.
    shouldBeCloseEnough("polygon.animatedPoints.getItem(0).x", "100");
    // change the animation to a CSS animatable value.
    set.setAttribute("attributeName", "opacity");
    set.setAttribute("to", "0.8");
}

function sample4() {
    shouldBeCloseEnough("parseFloat(getComputedStyle(polygon).opacity)", "0.8");
    // change the animation to a non-CSS animatable value.
    set.setAttribute("attributeName", "points");
    set.setAttribute("to", "200 0 300 0 300 100 200 100");
}

function sample5() {
    // verify that the animation does not run.
    shouldBeCloseEnough("polygon.animatedPoints.getItem(0).x", "100");
    shouldBeCloseEnough("parseFloat(getComputedStyle(polygon).opacity)", "1.0");
    // change the animationType to XML which is valid.
    set.setAttribute("attributeType", "XML");
}

function sample6() {
    shouldBeCloseEnough("polygon.animatedPoints.getItem(0).x", "200");
    shouldBe("polygon.points.getItem(0).x", "100");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["set", 0.0, sample1],
        ["set", 0.5, sample2],
        ["set", 1.0, sample3],
        ["set", 1.5, sample4],
        ["set", 2.0, sample5],
        ["set", 2.5, sample6]
    ];

    runAnimationTest(expectedValues);
}

window.clickX = 150;
var successfullyParsed = true;
