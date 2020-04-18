
This tests that an input text field can be properly scrolled with touch gestures

On success, you will see a series of "PASS" messages, followed by "TEST COMPLETE".


PASS successfullyParsed is true

TEST COMPLETE
===Testing fling behavior===
PASS box.scrollLeft is 0
PASS container.scrollLeft is 0
Flinging input text should scroll text by the specified amount
PASS box.scrollLeft is 40
PASS container.scrollLeft is 0
Flinging input text past the scrollable width shouldn't scroll containing div
PASS box.scrollLeft is fullyScrolled
PASS container.scrollLeft is 0
Flinging fully scrolled input text should fling containing div
PASS box.scrollLeft is fullyScrolled
PASS container.scrollLeft is 60
===Testing scroll behavior===
PASS box.scrollLeft is 0
PASS container.scrollLeft is 0
Gesture scrolling input text should scroll text the specified amount
PASS box.scrollLeft is 60
PASS container.scrollLeft is 0
Gesture scrolling input text past scroll width shouldn't scroll container div
PASS box.scrollLeft is fullyScrolled
PASS container.scrollLeft is 0
===Testing vertical scroll behavior===
PASS box.scrollTop is 0
PASS container.scrollTop is 0
Vertically gesture scrolling input text should scroll containing div the specified amount
PASS box.scrollTop is 0
PASS container.scrollTop is 60
PASS box.scrollTop is 0
PASS container.scrollTop is 0
Vertically flinging input text should scroll the containing div the specified amount
PASS box.scrollTop is 0
PASS container.scrollTop is 60
===Testing non-overflow scroll behavior===
Input box without overflow should not scroll
PASS box.scrollLeft is 0
PASS container.scrollLeft is 0
PASS box.clientWidth is >= box.scrollWidth
PASS box.scrollLeft is 0
PASS container.scrollLeft is 60

