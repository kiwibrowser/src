(async function(testRunner) {
  var {page, session, dp} = await testRunner.startURL('../resources/dom-snapshot-input-value.html', 'Tests DOMSnapshot.getSnapshot method returning input values.');

  function stabilize(key, value) {
    var unstableKeys = ['documentURL', 'baseURL', 'frameId', 'backendNodeId', 'layoutTreeNodes', 'computedStyles'];
    if (unstableKeys.indexOf(key) !== -1)
      return '<' + typeof(value) + '>';
    if (typeof value === 'string' && value.indexOf('/dom-snapshot/') !== -1)
      value = '<value>';
    return value;
  }

  var response = await dp.DOMSnapshot.getSnapshot({'computedStyleWhitelist': [], 'includeUserAgentShadowTree': true});
  if (response.error)
    testRunner.log(response);
  else
    testRunner.log(JSON.stringify(response.result, stabilize, 2));
  testRunner.completeTest();
})
