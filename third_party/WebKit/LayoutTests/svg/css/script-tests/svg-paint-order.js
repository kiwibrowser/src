description("Test paint-order.")

if (window.testRunner)
    testRunner.dumpAsText();
createSVGTestCase();

var text = createSVGElement("text");
text.setAttribute("id", "text");
text.setAttribute("x", "100px");
text.setAttribute("y", "100px");
rootSVGElement.appendChild(text);

function test(valueString, expectedValue) {
    // Reset paint-order.
    text.removeAttribute("style");

    // Run test
    text.setAttribute("style", "paint-order: " + valueString);
    shouldBeEqualToString("getComputedStyle(text).paintOrder", expectedValue);
}

function test_attr(valueString, expectedValue) {
    // Reset paint-order.
    text.removeAttribute("paint-order");

    // Run test
    text.setAttribute("paint-order", valueString);
    shouldBeEqualToString("getComputedStyle(text).paintOrder", expectedValue);
}

debug("");
debug("Test pre-normalized correct variants of 'paint-order'");
test("fill stroke markers", "fill stroke markers");
test("fill markers stroke", "fill markers stroke");
test("stroke fill markers", "stroke fill markers");
test("stroke markers fill", "stroke markers fill");
test("markers stroke fill", "markers stroke fill");
test("markers fill stroke", "markers fill stroke");

debug("");
debug("Test correct single keyword value of 'paint-order'");
test("normal", "fill stroke markers");
test("fill", "fill stroke markers");
test("stroke", "stroke fill markers");
test("markers", "markers fill stroke");

debug("");
debug("Test correct dual keyword values of 'paint-order'");
test("fill stroke", "fill stroke markers");
test("fill markers", "fill markers stroke");
test("stroke fill", "stroke fill markers");
test("stroke markers", "stroke markers fill");
test("markers fill", "markers fill stroke");
test("markers stroke", "markers stroke fill");

debug("");
debug("Test invalid values of 'paint-order'");
test("foo", "fill stroke markers");
test("fill foo", "fill stroke markers");
test("stroke foo", "fill stroke markers");
test("markers foo", "fill stroke markers");
test("normal foo", "fill stroke markers");
test("fill markers stroke foo", "fill stroke markers");

debug("");
debug("Test pre-normalized correct variants of 'paint-order' (presentation attribute)");
test_attr("fill stroke markers", "fill stroke markers");
test_attr("fill markers stroke", "fill markers stroke");
test_attr("stroke fill markers", "stroke fill markers");
test_attr("stroke markers fill", "stroke markers fill");
test_attr("markers stroke fill", "markers stroke fill");
test_attr("markers fill stroke", "markers fill stroke");

debug("");
debug("Test correct single keyword value of 'paint-order' (presentation attribute)");
test_attr("normal", "fill stroke markers");
test_attr("fill", "fill stroke markers");
test_attr("stroke", "stroke fill markers");
test_attr("markers", "markers fill stroke");

debug("");
debug("Test correct dual keyword values of 'paint-order' (presentation attribute)");
test_attr("fill stroke", "fill stroke markers");
test_attr("fill markers", "fill markers stroke");
test_attr("stroke fill", "stroke fill markers");
test_attr("stroke markers", "stroke markers fill");
test_attr("markers fill", "markers fill stroke");
test_attr("markers stroke", "markers stroke fill");

debug("");
debug("Test invalid values of 'paint-order' (presentation attribute)");
test_attr("foo", "fill stroke markers");
test_attr("fill foo", "fill stroke markers");
test_attr("stroke foo", "fill stroke markers");
test_attr("markers foo", "fill stroke markers");
test_attr("normal foo", "fill stroke markers");
test_attr("fill markers stroke foo", "fill stroke markers");

var successfullyParsed = true;

completeTest();
