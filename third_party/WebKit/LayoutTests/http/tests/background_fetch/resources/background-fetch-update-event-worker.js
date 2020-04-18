'use strict';

importScripts('/resources/testharness.js');

promise_test(async function() {
  assert_own_property(self, 'BackgroundFetchUpdateEvent');

  // The `id` and `fetches` are required options in the
  // BackgroundFetchUpdateEventInit. The latter must be an instance of
  // BackgroundFetchSettledFetches.
  assert_throws({name: "TypeError"}, () => new BackgroundFetchUpdateEvent('BackgroundFetchUpdateEvent'));
  assert_throws({name: "TypeError"}, () => new BackgroundFetchUpdateEvent('BackgroundFetchUpdateEvent', {}));
  assert_throws({name: "TypeError"}, () => new BackgroundFetchUpdateEvent('BackgroundFetchUpdateEvent', { id: 'foo' }));

  // TODO(rayankans): Add actual construction test to BackgroundFetchUpdateEvent after
  // https://github.com/WICG/background-fetch/issues/64 is resolved.

}, 'Verifies that the BackgroundFetchUpdateEvent can be constructed.');
