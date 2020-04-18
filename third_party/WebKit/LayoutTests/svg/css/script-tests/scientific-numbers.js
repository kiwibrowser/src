description("Test scientific numbers on <length> values for SVG presentation attributes.")
if (window.testRunner)
    testRunner.dumpAsText();
createSVGTestCase();

var text = createSVGElement("text");
text.setAttribute("id", "text");
text.setAttribute("x", "100px");
text.setAttribute("y", "100px");
rootSVGElement.appendChild(text);

function test(valueString, expectedValue) {
    // Reset baseline-shift to baseline.
    text.style.baselineShift = "baseline";
    shouldBeEqualToString("text.style.baselineShift", "baseline");

    // Run test
    text.style.baselineShift = valueString;
    shouldBeEqualToString("text.style.baselineShift", expectedValue);
}

debug("");
debug("Test positive exponent values with 'e'");
test(".5e2", "50");
test("5e1", "50");
test("0.5e2", "50");
test("+.5e2", "50");
test("+5e1", "50");
test("+0.5e2", "50");
test(".5e+2", "50");
test("5e+1", "50");
test("0.5e+2", "50");

debug("");
debug("Test positive exponent values with 'E'");
test(".5E2", "50");
test("5E1", "50");
test("0.5E2", "50");
test("+.5E2", "50");
test("+5E1", "50");
test("+0.5E2", "50");
test(".5E+2", "50");
test("5E+1", "50");
test("0.5E+2", "50");

debug("");
debug("Test negative exponent values with 'e'");
test("5000e-2", "50");
test("500e-1", "50");
test("+5000e-2", "50");
test("+500e-1", "50");
test("+5000e-2px", "50px");
test("+500e-1px", "50px");

debug("");
debug("Test negative exponent values with 'E'");
test("5000E-2", "50");
test("500E-1", "50");
test("+5000E-2", "50");
test("+500E-1", "50");
test("+5000.00E-2px", "50px");
test("+500E-1px", "50px");

debug("");
debug("Test negative numbers with exponents");
test("-.5e2px", "-50px");
test("-0.5e2px", "-50px");
test("-500e-1px", "-50px");

debug("");
debug("Test if value and 'em' still works");
test("50em", "50em");

debug("");
debug("Test if value and 'ex' still works");
test("50ex", "50ex");

debug("");
debug("Trailing and leading whitespaces");
test("       5e1", "50");
test("5e1      ", "50");

debug("");
debug("Test behavior on overflow");
test("2E+500", "3.40282e+38");
test("-2E+500", "-3.40282e+38");

debug("");
debug("Invalid values");
test("50e0.0", "baseline");
test("50 e0", "baseline");
test("50e 0", "baseline");
test("50.e0", "baseline");

var successfullyParsed = true;

completeTest();
