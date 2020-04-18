These tests exercise Chromium/Blink specific behavior (like garbage
collection, not crashing, etc) and will not be upstreamed to the
W3C web-platform-tests.

The simple-queue-*.html tests test the SimpleQueue class which is used
internally to Chrome's Stream API implementation. It uses nodes of 16K
elements. These tests exercise the code paths which create and switch
nodes. They are split into separate files because the large number of
iterations cause timeouts on MSAN bots.
