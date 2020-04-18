description("Test the behavior of one discard applied on another discard");
embedSVGTestCase("resources/discard-on-discard.svg");

// Setup animation test
function sample1() {
    expectFillColor(rect1, 255, 0, 0);
}

function sample2() {
    expectFillColor(rect1, 0, 255, 0);
}

function executeTest() {
    var rects = rootSVGElement.ownerDocument.getElementsByTagName("rect");
    rect1 = rects[0];

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["anim", 0.0,   sample1],
        ["anim", 0.01,   sample1],
        ["anim", 2.0,   sample2],
        ["anim", 2.01,   sample2],
        ["anim", 3.0,   sample2],
        ["anim", 3.01,   sample2]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;
