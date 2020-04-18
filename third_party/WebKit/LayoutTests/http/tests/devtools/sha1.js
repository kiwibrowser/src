// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests SHA-1 hashes.\n`);
  await TestRunner.loadModule('product_registry_impl');

  TestRunner.addResult('foobar : ' + ProductRegistryImpl.sha1('foobar'));
  TestRunner.addResult('hello : ' + ProductRegistryImpl.sha1('hello'));
  TestRunner.addResult('abcdefghijklmnopqrstuvwxyz : ' + ProductRegistryImpl.sha1('abcdefghijklmnopqrstuvwxyz'));
  TestRunner.addResult('ABCDEFGHIJKLMNOPQRSTUVWXYZ : ' + ProductRegistryImpl.sha1('ABCDEFGHIJKLMNOPQRSTUVWXYZ'));
  TestRunner.addResult('a : ' + ProductRegistryImpl.sha1('a'));
  TestRunner.addResult('A : ' + ProductRegistryImpl.sha1('A'));
  TestRunner.addResult('A1 : ' + ProductRegistryImpl.sha1('A1'));
  TestRunner.completeTest();
})();
