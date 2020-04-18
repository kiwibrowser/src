description("Test calcMode spline with to animation. You should see a green 100x100 path and only PASS messages");
createSVGTestCase();

// Setup test document
var path = createSVGElement("path");
path.setAttribute("id", "path");
path.setAttribute("d", "M 40 40 L 60 40 L 60 60 L 40 60 Z");
path.setAttribute("fill", "green");
path.setAttribute("onclick", "executeTest()");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "d");
animate.setAttribute("to", "M 0 0 L 100 0 L 100 100 L 0 100 z");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
path.appendChild(animate);
rootSVGElement.appendChild(path);

// Setup animation test
function sample1() {
    // Check initial/end conditions
    shouldBeEqualToString("path.getAttribute('d')", "M 40 40 L 60 40 L 60 60 L 40 60 Z");
}

function sample2() {
    shouldBeEqualToString("path.getAttribute('d')", "M 20 20 L 80 20 L 80 80 L 20 80 Z");
}

function sample3() {
    shouldBeEqualToString("path.getAttribute('d')", "M 0.00999928 0.00999928 L 99.99 0.00999928 L 99.99 99.99 L 0.00999928 99.99 Z");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.0,   sample1],
        ["animation", 2.0,   sample2],
        ["animation", 3.999, sample3],
        ["animation", 4.001, sample1]
    ];

    runAnimationTest(expectedValues);
}

var successfullyParsed = true;
