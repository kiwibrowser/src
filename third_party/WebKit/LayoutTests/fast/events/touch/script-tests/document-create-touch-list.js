description("This tests support for the document.createTouchList API.");

shouldBeTrue('"createTouchList" in document');

// Test createTouchList with no arguments.
var touchList = document.createTouchList();
shouldBeNonNull("touchList");
shouldBe("touchList.length", "0");
shouldBeNull("touchList.item(0)");
shouldBeNull("touchList.item(1)");
shouldThrow("touchList.item()");

// Test createTouchList with Touch objects as arguments.
try {
    var t = new Touch({identifier: 12341, target: document.body, clientX: 60, clientY:65, screenX:100, screenY: 105});
    var t2 = new Touch({identifier: 12341, target: document.body, clientX: 50, clientY:55, screenX:115, screenY: 120});
    var tl = document.createTouchList(t, t2);

    var evt = new TouchEvent("touchstart", {
        view: window,
        touches: tl,
        targetTouches: tl,
        changedTouches: tl,
        ctrlKey: true,
    });

    document.body.addEventListener("touchstart", function handleTouchStart(ev) {
        ts = ev;
        shouldBeTrue("ts instanceof TouchEvent");
        shouldBeTrue("ts.touches instanceof TouchList");
        shouldBe("ts.touches.length", "2");
        shouldBeTrue("ts.touches[0] instanceof Touch");
        shouldBe("ts.touches[0].identifier", "12341");
        shouldBe("ts.touches[0].clientX", "60");
        shouldBe("ts.touches[1].screenY", "120");
        shouldBe("ts.ctrlKey", "true");
    });

    document.body.dispatchEvent(evt);
} catch(e) {
    testFailed("An exception was thrown: " + e.message);
}

// Test createTouchList with invalid arguments which throws exceptions.
try {
    var tl = document.createTouchList(1, 2);
} catch(e) {
    testPassed("An exception was thrown: " + e.message);
}
isSuccessfullyParsed();

