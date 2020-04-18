// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that generated agent prototypes match with domains returned by schema.getDomains.\n`);


  var target = TestRunner.mainTarget;
  var domains = await target.schemaAgent().getDomains();

  if (!domains) {
    TestRunner.addResult('error getting domains');
    TestRunner.completeTest();
    return;
  }
  var domainNames = domains.map(domain => domain.name).sort();
  var agentNames = Object.keys(target._agents).sort();
  for (var domain of domainNames) {
    if (agentNames.indexOf(domain) === -1)
      TestRunner.addResult('agent ' + domain + ' is missing from target');
  }
  for (var agent of agentNames) {
    if (domainNames.indexOf(agent) === -1)
      TestRunner.addResult('domain ' + agent + ' is missing from schema.getDomains');
  }
  TestRunner.completeTest();
})();
