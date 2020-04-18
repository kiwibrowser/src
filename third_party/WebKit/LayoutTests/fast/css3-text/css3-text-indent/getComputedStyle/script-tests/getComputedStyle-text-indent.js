function testElementStyle(propertyJS, propertyCSS, value)
{
    shouldBe("e.style." + propertyJS, "'" + value + "'");
    shouldBe("e.style.getPropertyValue('" + propertyCSS + "')", "'" + value + "'");
}

function testComputedStyle(propertyJS, propertyCSS, value)
{
    computedStyle = window.getComputedStyle(e, null);
    shouldBe("computedStyle." + propertyJS, "'" + value + "'");
    shouldBe("computedStyle.getPropertyValue('" + propertyCSS + "')", "'" + value + "'");
}

function valueSettingTest(value, expectedValue, computedValue)
{
    debug("Value '" + value + "':");
    e.style.textIndent = value;
    testElementStyle("textIndent", "text-indent", expectedValue);
    testComputedStyle("textIndent", "text-indent", computedValue);
    debug('');
}

function invalidValueSettingTest(value, defaultValue)
{
    debug("Invalid value test - '" + value + "':");
    e.style.textIndent = value;
    testElementStyle("textIndent", "text-indent", defaultValue);
    testComputedStyle("textIndent", "text-indent", defaultValue);
    debug('');
}

description("This test checks that text-indent parses properly the properties from CSS3 Text.");

e = document.getElementById('test');

debug("Test the initial value:");
testComputedStyle("textIndent", "text-indent", '0px');
debug('');

valueSettingTest('100px', '100px', '100px');
valueSettingTest('20em', '20em', '200px');
valueSettingTest('50%', '50%', '50%');
valueSettingTest('calc(10px + 20px)', 'calc(30px)', '30px');
valueSettingTest('100px each-line', '100px each-line', '100px each-line');
valueSettingTest('each-line 100px', 'each-line 100px', '100px each-line');
valueSettingTest('20em each-line', '20em each-line', '200px each-line');
valueSettingTest('each-line 20em', 'each-line 20em', '200px each-line');
valueSettingTest('30% each-line', '30% each-line', '30% each-line');
valueSettingTest('each-line 30%', 'each-line 30%', '30% each-line');
valueSettingTest('calc(10px + 20px) each-line', 'calc(30px) each-line', '30px each-line');
valueSettingTest('each-line calc(10px + 20px)', 'each-line calc(30px)', '30px each-line');
valueSettingTest('100px hanging', '100px hanging', '100px hanging');
valueSettingTest('hanging 100px', 'hanging 100px', '100px hanging');
valueSettingTest('20em hanging', '20em hanging', '200px hanging');
valueSettingTest('hanging 20em', 'hanging 20em', '200px hanging');
valueSettingTest('30% hanging', '30% hanging', '30% hanging');
valueSettingTest('hanging 30%', 'hanging 30%', '30% hanging');
valueSettingTest('calc(10px + 20px) hanging', 'calc(30px) hanging', '30px hanging');
valueSettingTest('hanging calc(10px + 20px)', 'hanging calc(30px)', '30px hanging');
valueSettingTest('100px each-line hanging', '100px each-line hanging', '100px each-line hanging');
valueSettingTest('each-line 100px hanging', 'each-line 100px hanging', '100px each-line hanging');
valueSettingTest('each-line hanging 100px', 'each-line hanging 100px', '100px each-line hanging');
valueSettingTest('100px hanging each-line', '100px hanging each-line', '100px each-line hanging');
valueSettingTest('hanging 100px each-line', 'hanging 100px each-line', '100px each-line hanging');
valueSettingTest('hanging each-line 100px', 'hanging each-line 100px', '100px each-line hanging');
valueSettingTest('30% each-line hanging', '30% each-line hanging', '30% each-line hanging');
valueSettingTest('each-line 30% hanging', 'each-line 30% hanging', '30% each-line hanging');
valueSettingTest('each-line hanging 30%', 'each-line hanging 30%', '30% each-line hanging');
valueSettingTest('30% hanging each-line', '30% hanging each-line', '30% each-line hanging');
valueSettingTest('hanging 30% each-line', 'hanging 30% each-line', '30% each-line hanging');
valueSettingTest('hanging each-line 30%', 'hanging each-line 30%', '30% each-line hanging');
debug('');

defaultValue = '0px'
e.style.textIndent = defaultValue;
invalidValueSettingTest('10m', defaultValue);
invalidValueSettingTest('100px 100px', defaultValue);
invalidValueSettingTest('100px line', defaultValue);
invalidValueSettingTest('100px hang', defaultValue);
invalidValueSettingTest('10m each-line', defaultValue);
invalidValueSettingTest('each-line 10m', defaultValue);
invalidValueSettingTest('10m hangning', defaultValue);
invalidValueSettingTest('hanging 10m', defaultValue);
invalidValueSettingTest('10m each-line hanging', defaultValue);
invalidValueSettingTest('each-line', defaultValue);
invalidValueSettingTest('hanging', defaultValue);
invalidValueSettingTest('each-line hanging', defaultValue);
invalidValueSettingTest('100px each-line 100px', defaultValue);
invalidValueSettingTest('100px hanging 100px', defaultValue);
invalidValueSettingTest('each-line 100px each-line', defaultValue);
invalidValueSettingTest('hanging 100px hanging', defaultValue);
invalidValueSettingTest('100px line hanging', defaultValue);
invalidValueSettingTest('100px each-line hang', defaultValue);
