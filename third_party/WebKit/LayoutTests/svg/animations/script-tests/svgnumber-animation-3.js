description("Test for SVGNumber animation with invalid units.");
createSVGTestCase();

// Setup test document
var rect = createSVGElement("rect");
rect.setAttribute("id", "rect");
rect.setAttribute("x", "0");
rect.setAttribute("width", "100");
rect.setAttribute("height", "100");
rect.setAttribute("fill", "green");
rect.setAttribute("opacity", "0");
rect.setAttribute("onclick", "executeTest()");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "opacity");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
animate.setAttribute("from", "0px");
animate.setAttribute("to", "1px");
rect.appendChild(animate);
rootSVGElement.appendChild(rect);

// Setup animation test
function sample() {
    // Check initial/end conditions
    shouldBe("getComputedStyle(rect).opacity", "'0'");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.0,   sample],
        ["animation", 2.0,   sample],
        ["animation", 3.999, sample],
        ["animation", 4.001, sample]
    ];

    runAnimationTest(expectedValues);
}

var successfullyParsed = true;
