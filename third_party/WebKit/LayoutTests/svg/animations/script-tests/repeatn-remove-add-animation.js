description("This removes and adds an animation element while the animation is repeating");
embedSVGTestCase("resources/repeatn-remove-add-animation.svg");

// Setup animation test
function sample1() {
    expectFillColor(rect1, 0, 255, 0);
    expectFillColor(rect2, 255, 0, 0);
    expectFillColor(rect3, 255, 0, 0);
    expectFillColor(rect4, 255, 0, 0);
}

function sample2() {
    expectFillColor(rect1, 0, 255, 0);
    expectFillColor(rect2, 0, 255, 0);
    expectFillColor(rect3, 255, 0, 0);
    expectFillColor(rect4, 255, 0, 0);
}

function sample3() {
    expectFillColor(rect1, 0, 255, 0);
    expectFillColor(rect2, 0, 255, 0);
    expectFillColor(rect3, 0, 255, 0);
    expectFillColor(rect4, 255, 0, 0);
}

function sample4() {
    expectFillColor(rect1, 0, 255, 0);
    expectFillColor(rect2, 0, 255, 0);
    expectFillColor(rect3, 0, 255, 0);
    expectFillColor(rect4, 0, 255, 0);
}

function recreate() {
    var anim1 = rootSVGElement.ownerDocument.getElementById("anim");
    anim1.parentNode.removeChild(anim1);
    var anim2 = createSVGElement("animate");
    anim2.setAttribute("id", "anim");
    anim2.setAttribute("attributeName", "visibility");
    anim2.setAttribute("to", "visible");
    anim2.setAttribute("begin", "0s");
    anim2.setAttribute("dur", "2s");
    anim2.setAttribute("repeatCount", "4");
    rootSVGElement.appendChild(anim2);
}

function executeTest() {
    var rects = rootSVGElement.ownerDocument.getElementsByTagName("rect");
    rect1 = rects[0];
    rect2 = rects[1];
    rect3 = rects[2];
    rect4 = rects[3];

    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["anim", 0.0, sample1],
        ["anim", 0.001, sample1],
        ["anim", 2.0, sample1],
        ["anim", 2.001, sample2],
        ["anim", 4.0, sample2],
        ["anim", 4.001, sample3],
        ["anim", 5.0, recreate],
        ["anim", 6.0, sample3],
        ["anim", 6.001, sample4]
    ];

    runAnimationTest(expectedValues);
}

window.animationStartsImmediately = true;
var successfullyParsed = true;
