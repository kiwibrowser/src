// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that all color nicknames are lowercase to facilitate lookup\n`);
  await TestRunner.showPanel('elements');

  var badNames = [];
  for (var nickname in Common.Color.Nicknames) {
    if (nickname.toLowerCase() !== nickname)
      badNames.push(nickname);
  }

  if (badNames.length === 0)
    TestRunner.addResult('PASSED');
  else
    TestRunner.addResult('Non-lowercase color nicknames: ' + badNames.join(', '));

  TestRunner.completeTest();
})();
