// This should be accessed only in the install event or the message event. When
// this is false, it implies that this service worker is restarted after the
// install event.
let did_run_install_event = false;

self.addEventListener('install', e => {
  var scope = new URL(self.registration.scope);
  if (scope.searchParams.get('type') == 'features-during-install') {
    internals.countFeature(scope.searchParams.get('feature'));
    internals.countDeprecation(scope.searchParams.get('deprecated'));
  } else if (scope.searchParams.get('type') == 'skip-waiting') {
    e.waitUntil(self.skipWaiting());
  }
  did_run_install_event = true;
});

onmessage = e => {
  if (e.data.type == 'COUNT_FEATURE') {
    internals.countFeature(e.data.feature);
  } else if (e.data.type == 'COUNT_DEPRECATION') {
    internals.countDeprecation(e.data.feature);
  } else if (e.data.type == 'CLAIM') {
    let promise = self.clients.claim()
        .then(() => e.source.postMessage(
            {type: 'CLAIMED', restarted: !did_run_install_event}));
    e.waitUntil(promise);
  } else if (e.data.type == 'PING') {
    e.source.postMessage({type: 'PONG'});
  }
};
