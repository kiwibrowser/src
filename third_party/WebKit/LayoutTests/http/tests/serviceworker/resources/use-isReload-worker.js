// A service worker that calls FetchEvent.isReload for UseCounter purposes.
self.addEventListener('fetch', e => { e.isReload; });
