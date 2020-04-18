importScripts('../../serviceworker/resources/worker-testharness.js');

test(function() {
    assert_true('PushEvent' in self);

    var event = new PushEvent('PushEvent');
    assert_equals(event.type, 'PushEvent');
    assert_idl_attribute(event, 'data');
    assert_equals(event.cancelable, false);
    assert_equals(event.bubbles, false);
    assert_inherits(event, 'waitUntil');

    const textContents = 'Hello, world!';

    var eventWithInit = new PushEvent('PushEvent',
                                      { cancelable: true,
                                        bubbles: true,
                                        data: textContents,
                                      });
    assert_equals(eventWithInit.cancelable, true);
    assert_equals(eventWithInit.bubbles, true);
    assert_equals(eventWithInit.data.text(), textContents);

}, 'PushEvent is exposed and extends ExtendableEvent.');
