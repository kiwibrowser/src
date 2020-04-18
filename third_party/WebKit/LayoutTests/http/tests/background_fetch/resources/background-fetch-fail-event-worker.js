'use strict';

importScripts('/resources/testharness.js');

test(function() {
  assert_own_property(self, 'BackgroundFetchFailEvent');

  // The `id` and `fetches` are required options in the
  // BackgroundFetchFailEventInit. The latter must be a sequence of
  // BackgroundFetchSettledFetch instances.
  assert_throws({name: "TypeError"}, () => new BackgroundFetchFailEvent('BackgroundFetchFailEvent'));
  assert_throws({name: "TypeError"}, () => new BackgroundFetchFailEvent('BackgroundFetchFailEvent', {}));
  assert_throws({name: "TypeError"}, () => new BackgroundFetchFailEvent('BackgroundFetchFailEvent', { id: 'foo' }));
  assert_throws({name: "TypeError"}, () => new BackgroundFetchFailEvent('BackgroundFetchFailEvent', { id: 'foo', fetches: 'bar' }));

  const fetches = [
    new BackgroundFetchSettledFetch(new Request('non-existing-image.png'), new Response()),
    new BackgroundFetchSettledFetch(new Request('non-existing-image-2.png'), new Response())
  ];

  const event = new BackgroundFetchFailEvent('BackgroundFetchFailEvent', {
    id: 'my-id',
    fetches
  });

  assert_equals(event.type, 'BackgroundFetchFailEvent');
  assert_equals(event.cancelable, false);
  assert_equals(event.bubbles, false);
  assert_equals(event.id, 'my-id');

  assert_true(Array.isArray(event.fetches));
  assert_array_equals(event.fetches, fetches);

  assert_inherits(event, 'waitUntil');

}, 'Verifies that the BackgroundFetchFailEvent can be constructed.');
