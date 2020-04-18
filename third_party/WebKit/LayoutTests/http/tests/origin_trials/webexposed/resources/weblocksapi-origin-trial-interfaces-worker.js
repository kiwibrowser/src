importScripts('/resources/testharness.js',
              '/resources/origin-trials-helper.js');

test(t => {
  OriginTrialsHelper.check_properties(this,
      {'LockManager': ['request', 'query'],
       'Lock': ['name', 'mode'],
       });
}, 'Web Locks API interfaces and properties in Origin-Trial enabled worker.');

test(t => {
  assert_true('locks' in self.navigator, 'locks property exists on navigator');
}, 'Web Locks API entry point in Origin-Trial enabled worker.');

done();
