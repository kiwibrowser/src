description("Test for animation freeze when repeatDur is not a multiple of dur");
embedSVGTestCase("resources/animate-fill-freeze-with-repeatDur.svg");

// Setup animation test
function sample1() {
    shouldBeCloseEnough("rect1.x.animVal.value", "0");
    shouldBe("rect1.x.baseVal.value", "0");
}

function sample2() {
    shouldBeCloseEnough("rect1.x.animVal.value", "150");
    shouldBe("rect1.x.baseVal.value", "0");
}

function executeTest() {
    var rects = rootSVGElement.ownerDocument.getElementsByTagName("rect");
    rect1 = rects[0];

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["anim", 0.0,   sample1],
        ["anim", 6.0,   sample2]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;
