Canvas Manual Performance Tests
===============================

How to run the tests
--------------------

Start Chromium with `--disable-frame-rate-limit --enable-precise-memory-info`.
Open `RunAllTests.html`. The tests should run automatically until the final
green result page.

How to add tests to this set
----------------------------

Create a HTML-based test that renders a canvas with `requestAnimationFrame`. Add
the following to the document's `<head>`:

```
  <meta charset="utf-8">
  <script src="performance.js" type="module"></script>
```

This already enables all the infrastructure needed for performance measurement.
To add the test to the list of all tests that are usually run, include it in the
`TESTS` list of `RunAllTests.html`.

If you also want to enable scroll tests (where the page keeps scrolling in both
axes while measuring, add a new entry with `?scroll` appended to it.
