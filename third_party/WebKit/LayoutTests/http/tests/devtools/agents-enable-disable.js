// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Test that each agent could be enabled/disabled separately.\n`);


  var requestsSent = 0;
  var responsesReceived = 0;

  function finishWhenDone(agentName, action, errorString) {
    if (action === 'enable')
      TestRunner.addResult('');
    if (errorString)
      TestRunner.addResult(agentName + '.' + action + ' finished with error ' + errorString);
    else
      TestRunner.addResult(agentName + '.' + action + ' finished successfully');

    ++responsesReceived;
    if (responsesReceived === requestsSent)
      TestRunner.completeTest();
  }

  var targets = SDK.targetManager.targets();
  for (var target of targets) {
    var agentNames = Object.keys(target._agents)
                         .filter(function(agentName) {
                           var agent = target._agents[agentName];
                           return agent['enable'] && agent['disable']
                               && agentName !== 'ServiceWorker'
                               && agentName !== 'Security'
                               && agentName !== 'Inspector'
                               && agentName !== 'HeadlessExperimental';
                         })
                         .sort();

    async function disableAgent(agentName) {
      ++requestsSent;
      var agent = target._agents[agentName];
      var response = await agent.invoke_disable({});
      finishWhenDone(agentName, 'disable', response[Protocol.Error]);
    }

    async function enableAgent(agentName) {
      ++requestsSent;
      var agent = target._agents[agentName];
      var response = await agent.invoke_enable({});
      finishWhenDone(agentName, 'enable', response[Protocol.Error]);
    }

    agentNames.forEach(disableAgent);

    agentNames.forEach(agentName => {
      enableAgent(agentName);
      disableAgent(agentName);
    });
  }
})();
