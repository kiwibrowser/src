'use strict';

importScripts('/resources/testharness.js',
              '/w3c/resources/webidl2.js',
              '/w3c/resources/idlharness.js');

promise_test(async t => {
  const response = await fetch('../interfaces.idl');
  const idls = await response.text();

  const idl_array = new IdlArray();

  idl_array.add_untested_idls('interface Navigator {};');
  idl_array.add_untested_idls('interface WorkerNavigator {};');

  idl_array.add_idls(idls);

  let lock;
  await navigator.locks.request('name', l => { lock = l; });

  idl_array.add_objects({
    LockManager: [navigator.locks],
    Lock: [lock],
  });

  idl_array.test();
});

done();
