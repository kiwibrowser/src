(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`
<link rel='stylesheet' type='text/css' href='${testRunner.url('resources/keyframes.css')}'></link>
<style>
#element {
    animation: animName 1s 2s, mediaAnim 2s, doesNotExist 3s, styleSheetAnim 0s;
}

@keyframes animName {
    from {
        width: 100px;
    }
    10% {
        width: 150px;
    }
    100% {
        width: 200px;
    }
}

@media (min-width: 1px) {
    @keyframes mediaAnim {
        from {
            opacity: 0;
        }
        to {
            opacity: 1;
        }
    }
}

</style>
<div id='element'></div>
`, 'Test that keyframe rules are reported.');

  var CSSHelper = await testRunner.loadScript('../resources/css-helper.js');
  var cssHelper = new CSSHelper(testRunner, dp);

  await dp.DOM.enable();
  await dp.CSS.enable();

  var documentNodeId = await cssHelper.requestDocumentNodeId();
  var nodeId = await cssHelper.requestNodeId(documentNodeId, '#element');
  await cssHelper.loadAndDumpCSSAnimationsForNode(nodeId);
  testRunner.completeTest();
});
