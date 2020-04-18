description("Test for checking position of the svg element when multiple animateMotion are acting on it");
embedSVGTestCase("resources/animateMotion-multiple.svg");

// Setup animation test
function sample1() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "20");
}

function sample2() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "20");
}

function sample3() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "40");
}

function sample4() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "60");
}

function sample5() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "20");
}

function sample6() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "20");
}

function executeTest() {
    var rects = rootSVGElement.ownerDocument.getElementsByTagName("rect");
    rect1 = rects[0];

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["anim", 0.0,   sample1],
        ["anim", 1.0,   sample2],
        ["anim", 2.0,   sample3],
        ["anim", 4.0,   sample4],
        ["anim", 6.0,   sample5],
        ["anim", 7.0,   sample6]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;
