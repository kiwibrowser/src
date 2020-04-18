function withIframe(url, name) {
  return new Promise((resolve, reject) => {
      const frame = document.createElement('iframe');
      frame.src = url;
      frame.name = name;
      frame.onload = () => resolve(frame);
      frame.onerror = () => reject('failed to load ' + url);
      document.body.appendChild(frame);
    });
}

function loadScript(url) {
  return new Promise((resolve, reject) => {
    const scriptTag = document.createElement('script');
    scriptTag.src = url;
    scriptTag.onload = () => resolve();
    scriptTag.onerror = () => reject('failed to load ' + url);
    document.head.appendChild(scriptTag);
  });
}

// Used to delay the iframe creation after "didFinishLoadForFrame" is printed.
// This is intended to avoid the flakiness of the result outputs.
const waitUntilDidFinishLoadForFrame = new Promise((resolve) => {
  window.addEventListener('load', () => setTimeout(resolve, 0));
});

const mojoBindingsLoaded = (async () => {
  await loadScript('/gen/layout_test_data/mojo/public/js/mojo_bindings.js');
  mojo.config.autoLoadMojomDeps = false;
  const urls = [
    '/gen/mojo/public/mojom/base/time.mojom.js',
    '/gen/third_party/blink/public/mojom/web_package/web_package_internals.mojom.js',
  ];
  await Promise.all(urls.map(loadScript));
})();

async function setSignedExchangeVerificationTime(time) {
  await mojoBindingsLoaded;
  const webPackageInternals = new blink.test.mojom.WebPackageInternalsPtr();
  Mojo.bindInterface(blink.test.mojom.WebPackageInternals.name,
                     mojo.makeRequest(webPackageInternals).handle, 'process');
  const windowsEpoch = Date.UTC(1601, 0, 1, 0, 0, 0, 0);
  return webPackageInternals.setSignedExchangeVerificationTime({
        internalValue: (time - windowsEpoch) * 1000
      });
}
