if (window.testRunner) {
  // This is a workaroud for an issue that the test runner sends notifyDone
  // before resources specified in CSS url finish loading. Currently all
  // such resources are specified in inline CSSs so we can rely on the
  // Document load event (Note that we cannot rely on that with linked
  // CSSs).
  testRunner.waitUntilDone();
  window.addEventListener('load', () => {
    // Force layout.
    document.body.offsetTop;

    testRunner.notifyDone();
  });
}

