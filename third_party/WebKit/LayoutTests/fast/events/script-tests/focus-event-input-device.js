function runFocusTestWithEventGenerator(generateEvent, expectedFiresTouchEvents) {
    // TODO(lanwei): The select drop-down menu on Linux works differently from how it behaves on ChromeOS. On ChromeOS,
    // the second tap will defocus both the popup menu and select element, will correct this behavior first and then
    // add select to this test, see https://crbug.com/510950.
    var testElement = function(ele) {
        var focusEventHandler = function(event) {
            debug(event.type);
            shouldBeNonNull("event.sourceCapabilities");
            shouldBe("event.sourceCapabilities.firesTouchEvents", expectedFiresTouchEvents);
        }

        var e = document.createElement(ele);
        if (ele == "a") {
            e.href = "#";
        }
        if (ele == "div") {
            e.tabIndex = 1;
        }
        e.style.width = "100px";
        e.style.height = "100px";
        e.style.display = "block";
        for (var evt of ['focus', 'focusin', 'blur', 'focusout']) {
            e.addEventListener(evt, focusEventHandler, false);
        }

        document.body.insertBefore(e, document.body.firstChild);
        debug("tests on " + ele);

        if (generateEvent.name == "sendKeyboardTabEvent")
            sendKeyboardTabEvent();
        else {
            generateEvent(10, 10);
            if (ele == "select") {
                generateEvent(160, 160);
            }
            generateEvent(160, 160);
            document.body.removeChild(e);
        }
    }

    for (var ele of ['div', 'input', 'button', 'textarea', 'select', 'a']) {
        if (ele == "select" && generateEvent.name == "sendGestureTapEvent")
            continue;
        testElement(ele, generateEvent);
        debug("");
    }
}
