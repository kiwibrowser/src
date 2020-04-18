function runTest() {
    if (!window.testRunner || !window.sessionStorage)
        return;

    if (!window.targetScaleFactor)
        window.targetScaleFactor = 2;

    if (!sessionStorage.scaleFactorIsSet) {
        testRunner.waitUntilDone();
        testRunner.setBackingScaleFactor(targetScaleFactor, scaleFactorIsSet);
    }

    if (sessionStorage.pageReloaded && sessionStorage.scaleFactorIsSet) {
        delete sessionStorage.pageReloaded;
        delete sessionStorage.scaleFactorIsSet;
        if (!window.manualNotifyDone) {
            setTimeout(function() {
                testRunner.notifyDone();
            }, 0);
        }
    } else {
        // Right now there is a bug that srcset does not properly deal with dynamic changes to the scale factor,
        // so to work around that, we must reload the page to get the new image.
        //
        // At the time of the Document load event, there might be other
        // ongoing tasks that can cause new images to be loaded. To evict
        // those images, we delay evictAllResources() call a little.
        setTimeout(function() {
          sessionStorage.pageReloaded = true;
          if (window.internals) {
              internals.evictAllResources();
          }
          document.location.reload(true);
        }, 300);
    }
}

function scaleFactorIsSet() {
    sessionStorage.scaleFactorIsSet = true;
}

window.addEventListener("load", runTest, false);
