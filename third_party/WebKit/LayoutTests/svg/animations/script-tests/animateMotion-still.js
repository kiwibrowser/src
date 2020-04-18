description("Test animations that only express an offset");
embedSVGTestCase("resources/animateMotion-still.svg");

// Setup animation test
function sample1() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "100");
}

function sample2() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "200");
}

function sample3() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "200");
}

function sample4() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "0");
}

function sample5() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "0");
}

function executeTest() {
    var rects = rootSVGElement.ownerDocument.getElementsByTagName("rect");
    rect1 = rects[0];

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["anim", 1.0,   sample1],
        ["anim", 2.0,   sample2],
        ["anim", 3.0,   sample3],
        ["anim", 4.0,   sample4],
        ["anim", 5.0,   sample5]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;
