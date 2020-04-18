description("A copy of the corresponding W3C-SVG-1.1 test, which dumps the animation at certain times");
embedSVGTestCase("../W3C-SVG-1.1/animate-elem-08-t.svg");

// Setup animation test
function sample1() {
    expectMatrix("getTransformToElement(rootSVGElement, path1)", "0", "1", "-1", "0", "224.9", "-25");
    expectMatrix("getTransformToElement(rootSVGElement, path2)", "0", "-1", "1", "0", "-225", "275");
}

function sample2() {
    expectMatrix("getTransformToElement(rootSVGElement, path1)", "1", "0.1", "-0.1", "1", "-70.9", "-182.6");
    expectMatrix("getTransformToElement(rootSVGElement, path2)", "-1", "-0.1", "0.1", "-1", "319.3", "210.7");
}

function sample3() {
    expectMatrix("getTransformToElement(rootSVGElement, path1)", "0.7", "-0.7", "0.7", "0.7", "-265.1", "-17.7");
    expectMatrix("getTransformToElement(rootSVGElement, path2)", "-0.7", "0.7", "-0.7", "-0.7", "442", "-159");
}

function executeTest() {
    path1 = rootSVGElement.ownerDocument.getElementsByTagName("path")[1];
    path2 = rootSVGElement.ownerDocument.getElementsByTagName("path")[3];

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["an1", 0.0,  sample1],
        ["an1", 3.0,  sample2],
        ["an1", 6.0,  sample3],
        ["an1", 60.0, sample3]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;
