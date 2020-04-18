description("Test path animation where coordinate modes of start and end differ. You should see PASS messages");
createSVGTestCase();

// Setup test document
var path = createSVGElement("path");
path.setAttribute("id", "path");
path.setAttribute("d", "M -20 -20 C 20 -20 20 -20 20 20 S 20 40 -20 20 Z");
path.setAttribute("fill", "green");
path.setAttribute("onclick", "executeTest()");
path.setAttribute("transform", "translate(50, 50)");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "d");
animate.setAttribute("from", "M -20 -20 C 20 -20 20 -20 20 20 S 20 40 -20 20 Z");
animate.setAttribute("to", "M -20 -20 c 0 40 0 40 40 40 s 40 0 0 -40 z");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
path.appendChild(animate);
rootSVGElement.appendChild(path);

// Setup animation test
function sample1() {
    // Check initial/end conditions
    shouldBeEqualToString("path.getAttribute('d')", "M -20 -20 C 20 -20 20 -20 20 20 S 20 40 -20 20 Z");
}

function sample2() {
    shouldBeEqualToString("path.getAttribute('d')", "M -20 -20 C 10 -10 10 -10 20 20 S 30 35 -10 10 Z");
}

function sample3() {
    shouldBeEqualToString("path.getAttribute('d')", "M -20 -20 c 10 30 10 30 40 40 s 30 5 -10 -30 Z");
}

function sample4() {
    shouldBeEqualToString("path.getAttribute('d')", "M -20 -20 c 0.00999832 39.99 0.00999832 39.99 40 40 s 39.99 0.00499916 -0.00999832 -39.99 Z");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.0,   sample1],
        ["animation", 1.0,   sample2],
        ["animation", 3.0,   sample3],
        ["animation", 3.999, sample4],
        ["animation", 4.001, sample1]
    ];

    runAnimationTest(expectedValues);
}

var successfullyParsed = true;
