importScripts('../../serviceworker/resources/worker-testharness.js');

test(() => {
  assert_true('AbortPaymentEvent' in self);
  assert_inherits(AbortPaymentEvent.prototype, 'waitUntil');
  assert_own_property(AbortPaymentEvent.prototype, 'respondWith');
});

promise_test(() => {
  return new Promise(resolve => {
    var abortEvent = new AbortPaymentEvent('abortpayment', {});

    self.addEventListener('abortpayment', e => {
      resolve();
    });

    self.dispatchEvent(abortEvent);
  });
});
