(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`
<div id='shadow-host'>
  <p id='shadow-content'>This text is bold</p>
</div>`, 'Test that ::content pseudo selector is reported in matched rules.');
  await session.evaluate(() => {
    var host = document.querySelector('#shadow-host');
    var root = host.createShadowRoot();
    root.innerHTML = '<style>:host ::content * { font-weight: bold; }</style><content></content>';
  });

  var CSSHelper = await testRunner.loadScript('../resources/css-helper.js');
  var cssHelper = new CSSHelper(testRunner, dp);

  await dp.DOM.enable();
  await dp.CSS.enable();
  var documentNodeId = await cssHelper.requestDocumentNodeId();
  var nodeId = await cssHelper.requestNodeId(documentNodeId, '#shadow-content');
  await cssHelper.loadAndDumpMatchingRulesForNode(nodeId);
  testRunner.completeTest();
})
