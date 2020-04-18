importScripts('/resources/testharness.js');

test(function() {
    assert_own_property(self, 'SyncManager', 'SyncManager needs to be exposed as a global.');
    assert_idl_attribute(registration, 'sync', 'One-shot SyncManager needs to be exposed on the registration.');

    assert_inherits(registration.sync, 'register');
    assert_inherits(registration.sync, 'getTags');

}, 'SyncManager should be exposed and have the expected interface.');

test(function() {
    assert_own_property(self, 'SyncEvent');

    var instance = new SyncEvent('dummy', {tag: ''});
    assert_idl_attribute(instance, 'tag');
    assert_idl_attribute(instance, 'lastChance');

    // SyncEvent should be extending ExtendableEvent.
    assert_inherits(SyncEvent.prototype, 'waitUntil');

}, 'SyncEvent should be exposed and have the expected interface.');
