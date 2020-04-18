description("Test animate intervals having begin-value without a matching end-value");
embedSVGTestCase("resources/animate-no-matching-end-value.svg");

// Setup animation test
function sample1() {
    shouldBeCloseEnough("rect1.x.animVal.value", "0");
    shouldBe("rect1.x.baseVal.value", "0");
}

function sample2() {
    shouldBeCloseEnough("rect1.x.animVal.value", "100");
    shouldBe("rect1.x.baseVal.value", "0");
}

function executeTest() {
    var rects = rootSVGElement.ownerDocument.getElementsByTagName("rect");
    rect1 = rects[0];

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["anim", 0.0,   sample1],
        ["anim", 1.0,   sample2],
        ["anim", 2.0,   sample1],
        ["anim", 3.0,   sample2],
        ["anim", 4.0,   sample1],
        ["anim", 5.0,   sample1]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;
