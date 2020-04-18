(async function layoutFontTest(testRunner, session) {
  var documentNodeId = (await session.protocol.DOM.getDocument()).result.root.nodeId;
  await session.protocol.CSS.enable();
  var testNodes = await session.evaluate(`
    Array.prototype.map.call(document.querySelectorAll('.test *'), e => ({selector: '#' + e.id, textContent: e.textContent}))
  `);

  for (var testNode of testNodes) {
    var nodeId = (await session.protocol.DOM.querySelector({nodeId: documentNodeId , selector: testNode.selector})).result.nodeId;
    var response = await session.protocol.CSS.getPlatformFontsForNode({nodeId});
    var usedFonts = response.result.fonts;
    usedFonts.sort((a, b) => b.glyphCount - a.glyphCount);

    testRunner.log(testNode.textContent);
    testRunner.log(testNode.selector + ':');
    for (var i = 0; i < usedFonts.length; i++) {
      var usedFont = usedFonts[i];
      var isLast = i === usedFonts.length - 1;
      testRunner.log(`"${usedFont.familyName}" : ${usedFont.glyphCount}${isLast ? '' : ','}`);
    }
    testRunner.log('');
    testNode.usedFonts = usedFonts;
  }
  return testNodes;
})
