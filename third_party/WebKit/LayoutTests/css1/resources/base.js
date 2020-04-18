if (window.testRunner) {
  // This is a workaroud for an issue that the load event may be dispatched
  // before loading ../resources/basebg.gif which is linked from
  // ../resources/base.css.
  testRunner.waitUntilDone();
  window.addEventListener('load', () => {
    const id = setInterval(() => {
      // Force layout.
      document.body.offsetTop;
      if (internals.isLoading('../resources/basebg.gif'))
        return;

      clearInterval(id);
      testRunner.notifyDone();
    }, 50);
  });
}
