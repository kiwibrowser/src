embedSVGTestCase("resources/animation-policy.svg");

function executeTest() {
    rect = rootSVGElement.ownerDocument.getElementsByTagName("rect")[0];

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.0, sample1],
        ["animation", 1.0, sample2],
        ["animation", 1.999, sample3],
        ["animation", 2.0, sample4],
        ["animation", 3.1, sample5],
        ["animation", 3.999, sample6]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;
