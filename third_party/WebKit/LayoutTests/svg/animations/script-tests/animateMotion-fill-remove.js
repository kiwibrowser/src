description("Test for animation freeze when repeatDur is not a multiple of dur");
embedSVGTestCase("resources/animateMotion-fill-remove.svg");

// Setup animation test
function sample1() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "0");
}

function sample2() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "50");
}

function sample3() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "0");
}

function sample4() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "0");
}

function executeTest() {
    var rects = rootSVGElement.ownerDocument.getElementsByTagName("rect");
    rect1 = rects[0];

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["anim", 0.0,   sample1],
        ["anim", 2.0,   sample2],
        ["anim", 4.0,   sample3],
        ["anim", 6.0,   sample4]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;
