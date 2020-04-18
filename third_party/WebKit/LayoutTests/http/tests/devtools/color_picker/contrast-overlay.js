// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that the contrast line algorithm produces good results and terminates.\n`);


  await self.runtime.loadModulePromise('color_picker');
  var contrastInfo = new ColorPicker.ContrastInfo();
  var contrastLineBuilder = new ColorPicker.ContrastRatioLineBuilder(contrastInfo);
  TestRunner.assertTrue(contrastLineBuilder != null);

  var colorPairs = [
    // Boring black on white
    {fg: 'black', bg: 'white'},
    // Blue on white - line does not go to RHS
    {fg: 'blue', bg: 'white'},
    // Transparent on white - no possible line
    {fg: 'transparent', bg: 'white'},
    // White on color which previously caused infinite loop
    {fg: 'rgba(255, 255, 255, 1)', bg: 'rgba(157, 83, 95, 1)'}
  ];

  function logLineForColorPair(fgColorString, bgColorString) {
    var contrastInfoData = {
      backgroundColors: [bgColorString],
      computedFontSize: '16px',
      computedFontWeight: '400',
      computedBodyFontSize: '16px'
    };
    contrastInfo.update(contrastInfoData);
    var fgColor = Common.Color.parse(fgColorString);
    contrastInfo.setColor(fgColor.hsva(), fgColorString);
    var d = contrastLineBuilder.drawContrastRatioLine(100, 100);

    TestRunner.addResult('');
    TestRunner.addResult(
        'For fgColor ' + fgColorString + ', bgColor ' + bgColorString + ', path was' + (d.length ? '' : ' empty.'));
    if (d.length)
      TestRunner.addResult(d);
  }

  for (var pair of colorPairs)
    logLineForColorPair(pair.fg, pair.bg);

  TestRunner.completeTest();
})();
