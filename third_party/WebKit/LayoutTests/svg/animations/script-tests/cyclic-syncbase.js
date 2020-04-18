description("Test cyclic for svg animations for syncbases");
embedSVGTestCase("resources/cyclic-syncbase.svg");

// Setup animation test
function sample1() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "100");
    shouldBeCloseEnough("rootSVGElement.getBBox().y", "0");
}

function sample2() {
    shouldBeCloseEnough("rootSVGElement.getBBox().x", "0");
    shouldBeCloseEnough("rootSVGElement.getBBox().y", "100");
}

function executeTest() {

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["anim", 0.01,   sample1],
        ["anim", 1.01,   sample2],
        ["anim", 2.01,   sample1],
        ["anim", 3.01,   sample2],
        ["anim", 4.01,   sample1]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;

