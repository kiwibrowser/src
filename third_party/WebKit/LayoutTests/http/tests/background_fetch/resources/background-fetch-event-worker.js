'use strict';

importScripts('/resources/testharness.js');

test(function() {
  assert_own_property(self, 'BackgroundFetchEvent');

  // The `id` is required in the BackgroundFetchEventInit.
  assert_throws({name: "TypeError"}, () => new BackgroundFetchEvent('BackgroundFetchEvent'));
  assert_throws({name: "TypeError"}, () => new BackgroundFetchEvent('BackgroundFetchEvent', {}));

  const event = new BackgroundFetchEvent('BackgroundFetchEvent', {
    id: 'my-id'
  });

  assert_equals(event.type, 'BackgroundFetchEvent');
  assert_equals(event.cancelable, false);
  assert_equals(event.bubbles, false);
  assert_equals(event.id, 'my-id');

  assert_inherits(event, 'waitUntil');

}, 'Verifies that the BackgroundFetchEvent can be constructed.');
