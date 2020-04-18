<?php
// Generate token with the command:
// generate_token.py http://127.0.0.1:8000 WebLocksAPI --expire-timestamp=2000000000
header("Origin-Trial: Aq40kr/ZTqxmfeh35cvBQcwBrmiL7pSDR6PrUZaVC7xxGe3ff4fECD/TdP+w+Ic9cXZ1ek6N4kg6oR876PQd/QoAAABTeyJvcmlnaW4iOiAiaHR0cDovLzEyNy4wLjAuMTo4MDAwIiwgImZlYXR1cmUiOiAiV2ViTG9ja3NBUEkiLCAiZXhwaXJ5IjogMjAwMDAwMDAwMH0=");
header('Content-Type: application/javascript');
?>
importScripts('/resources/testharness.js',
              '/resources/origin-trials-helper.js');

test(t => {
  OriginTrialsHelper.check_properties(this,
      {'LockManager': ['request', 'query'],
       'Lock': ['name', 'mode'],
       });
}, 'Web Locks API interfaces and properties in Origin-Trial enabled serviceworker.');

test(t => {
  assert_true('locks' in self.navigator, 'locks property exists on navigator');
}, 'Web Locks API entry point in Origin-Trial enabled serviceworker.');

done();
