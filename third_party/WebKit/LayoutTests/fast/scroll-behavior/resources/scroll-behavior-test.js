// A ScrollBehaviorTest runs a set of ScrollBehaviorTestCases. The only
// ScrollBehaviorTest method that should be called by external code is run().

// Creates a ScrollBehaviorTest with arguments:
// scrollElement - Element being scrolled.
// scrollEventTarget - Target for scroll events for |scrollElement|.
// testsCases - Array of ScrollBehaviorTestCases.
// getEndPosition - Callback that takes a test case and start position, and
//                  returns the corresponding end position (where positions
//                  are dictionaries with x and y fields).
// jsScroll - Callback that takes a test case and executes the corresponding
//            js-driven scroll (e.g. by setting scrollLeft/scrollTop or by
//            calling scroll, scrollTo, or scrollBy). This should assume that
//            scrollElement's scroll-behavior CSS property has already been
//            set appropriately.
function ScrollBehaviorTest(scrollElement,
                            scrollEventTarget,
                            testCases,
                            getEndPosition,
                            jsScroll) {
    this.scrollElement = scrollElement;
    this.scrollEventTarget = scrollEventTarget;
    this.testCases = testCases;
    this.currentTestCase = 0;
    this.getEndPosition = getEndPosition;
    this.jsScroll = jsScroll;
}

ScrollBehaviorTest.prototype.scrollListener = function(testCase) {
    var endReached = (this.scrollElement.scrollLeft == testCase.endX && this.scrollElement.scrollTop == testCase.endY);
    if (endReached) {
        this.testCaseComplete();
        return;
    }

    if (testCase.waitForEnd)
        return;

    // Wait for the animation to start, then instant-scroll to the end state.
    if (this.scrollElement.scrollLeft != testCase.startX || this.scrollElement.scrollTop != testCase.startY) {
        // Instant scroll, and then wait for the next scroll event. This allows
        // the instant scroll to propagate to the compositor (when using
        // composited scrolling) so that the next smooth scroll starts at this
        // position (the compositor always starts smooth scrolls at the current
        // scroll position on the compositor thread).
        this.scrollElement.scrollTo({left: testCase.endX, top: testCase.endY, behavior: "instant"});
        testCase.waitForEnd = true;
    }
};

ScrollBehaviorTest.prototype.startNextTestCase = function() {
    if (this.currentTestCase >= this.testCases.length) {
        this.allTestCasesComplete();
        return;
    }
    var testCase = this.testCases[this.currentTestCase];
    if (testCase.pageScaleFactor && window.internals) {
        internals.setPageScaleFactor(testCase.pageScaleFactor);
    }

    var isSmoothTest = (testCase.js == "smooth" || (testCase.css == "smooth" && testCase.js != "instant"));

    this.asyncTest = async_test("Scroll x:" + testCase.x + ", y:" + testCase.y + ", smooth:" + isSmoothTest);

    var currentPosition = {};
    currentPosition.x = this.scrollElement.scrollLeft;
    currentPosition.y = this.scrollElement.scrollTop;
    var endPosition = this.getEndPosition(testCase, currentPosition);
    testCase.setStartPosition(currentPosition);
    testCase.setEndPosition(endPosition);

    this.scrollElement.style.scrollBehavior = testCase.css;
    this.jsScroll(testCase);

    var scrollElement = this.scrollElement;
    if (isSmoothTest) {
        this.asyncTest.step(function() {
            assert_equals(scrollElement.scrollLeft + ", " + scrollElement.scrollTop, testCase.startX + ", " + testCase.startY);
        });
        if (scrollElement.scrollLeft == testCase.endX && scrollElement.scrollTop == testCase.endY) {
            // We've instant-scrolled. This means we've already failed the assert above, and will never
            // reach an intermediate frame. End the test case now to avoid hanging while waiting for an
            // intermediate frame.
            this.testCaseComplete();
        } else {
            testCase.scrollListener = this.scrollListener.bind(this, testCase);
            this.scrollEventTarget.addEventListener("scroll", testCase.scrollListener);
        }
    } else {
        this.asyncTest.step(function() {
            assert_equals(scrollElement.scrollLeft + ", " + scrollElement.scrollTop, testCase.endX + ", " + testCase.endY);
        });
        this.testCaseComplete();
    }
}

ScrollBehaviorTest.prototype.testCaseComplete = function() {
    var currentScrollListener = this.testCases[this.currentTestCase].scrollListener;
    if (currentScrollListener) {
        this.scrollEventTarget.removeEventListener("scroll", currentScrollListener);
    }
    this.asyncTest.done();

    this.currentTestCase++;
    this.startNextTestCase();
}

ScrollBehaviorTest.prototype.run = function() {
    setup({explicit_done: true, explicit_timeout: true});
    this.startNextTestCase();
}

ScrollBehaviorTest.prototype.allTestCasesComplete = function() {
    done();
}


// A ScrollBehaviorTestCase represents a single scroll.
//
// Creates a ScrollBehaviorTestCase. |testData| is a dictionary with fields:
// css - Value of scroll-behavior CSS property.
// js - (optional) Value of scroll behavior used in javascript.
// x, y - Coordinates to be used when carrying out the scroll.
// waitForEnd - (must be provided for smooth scrolls) Whether the test runner should
//              wait until the scroll is complete, rather than only waiting until
//              the scroll is underway.
// pageScaleFactor - (optional) if set, applies pinch-zoom by the given factor.
function ScrollBehaviorTestCase(testData) {
    this.js = testData.js;
    this.css = testData.css;
    this.waitForEnd = testData.waitForEnd;
    this.x = testData.x;
    this.y = testData.y;
    this.pageScaleFactor = testData.pageScaleFactor;
}

ScrollBehaviorTestCase.prototype.setStartPosition = function(startPosition) {
    this.startX = startPosition.x;
    this.startY = startPosition.y;
}

ScrollBehaviorTestCase.prototype.setEndPosition = function(endPosition) {
    this.endX = endPosition.x;
    this.endY = endPosition.y;
}
