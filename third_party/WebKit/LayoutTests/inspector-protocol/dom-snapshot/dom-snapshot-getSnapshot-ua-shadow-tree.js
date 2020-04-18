(async function(testRunner) {
  var {page, session, dp} = await testRunner.startURL('../resources/dom-snapshot-ua-shadow-tree.html', 'Tests DOMSnapshot.getSnapshot method with includeUserAgentShadowTree defaulted to false.');

  await session.evaluate(`
    var shadowroot = document.querySelector('#shadow-host').attachShadow({mode: 'open'});
    var textarea = document.createElement('textarea');
    textarea.value = 'hello hello!';
    var video = document.createElement('video');
    video.src = 'test.webm';
    shadowroot.appendChild(textarea);
    shadowroot.appendChild(video);
  `);

  function stabilize(key, value) {
    var unstableKeys = ['documentURL', 'baseURL', 'frameId', 'backendNodeId', 'layoutTreeNodes'];
    if (unstableKeys.indexOf(key) !== -1)
      return '<' + typeof(value) + '>';
    return value;
  }

  var response = await dp.DOMSnapshot.getSnapshot({'computedStyleWhitelist': [], 'includeEventListeners': true});
  if (response.error)
    testRunner.log(response);
  else
    testRunner.log(JSON.stringify(response.result, stabilize, 2));
  testRunner.completeTest();
})
