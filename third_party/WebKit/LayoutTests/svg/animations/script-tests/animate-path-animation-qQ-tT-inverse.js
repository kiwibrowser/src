description("Test path animation where coordinate modes of start and end differ. You should see PASS messages");
createSVGTestCase();

// Setup test document
var path = createSVGElement("path");
path.setAttribute("id", "path");
path.setAttribute("d", "M -30 -30 q 30 0 30 30 t -30 30 Z");
path.setAttribute("fill", "green");
path.setAttribute("onclick", "executeTest()");
path.setAttribute("transform", "translate(50, 50)");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "d");
animate.setAttribute("from", "M -30 -30 q 30 0 30 30 t -30 30 Z");
animate.setAttribute("to", "M -30 -30 Q 30 -30 30 0 T -30 30 Z");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
path.appendChild(animate);
rootSVGElement.appendChild(path);

// Setup animation test
function sample1() {
    // Check initial/end conditions
    shouldBeEqualToString("path.getAttribute('d')", "M -30 -30 q 30 0 30 30 t -30 30 Z");
}

function sample2() {
    shouldBeEqualToString("path.getAttribute('d')", "M -30 -30 q 37.5 0 37.5 30 t -37.5 30 Z");
}

function sample3() {
    shouldBeEqualToString("path.getAttribute('d')", "M -30 -30 Q 22.5 -30 22.5 0 T -30 30 Z");
}

function sample4() {
    shouldBeEqualToString("path.getAttribute('d')", "M -30 -30 Q 29.9925 -30 29.9925 0 T -30 30 Z");
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

window.clickX = 40;
window.clickY = 70;
var successfullyParsed = true;
