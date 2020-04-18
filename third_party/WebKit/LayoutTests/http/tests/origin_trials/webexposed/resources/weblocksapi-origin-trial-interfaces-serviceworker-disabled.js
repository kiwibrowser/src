importScripts('/resources/testharness.js',
              '/resources/origin-trials-helper.js');

test(t => {
  OriginTrialsHelper.check_interfaces_missing(
    self,
    ['LockManager', 'Lock']);
}, 'Web Locks API interfaces in Origin-Trial disabled worker.');

test(t => {
  assert_false('locks' in self.navigator,
               'locks property does not exist on navigator');
}, 'Web Locks API entry point in Origin-Trial disabled worker.');

done();
