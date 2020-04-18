// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests Common.Color\n`);


  function dumpColor(colorText) {
    var color = Common.Color.parse(colorText);
    TestRunner.addResult('Dumping \'' + colorText + '\' in different formats:');
    TestRunner.addResult(' - ' + color.asString(Common.Color.Format.RGB));
    TestRunner.addResult(' - ' + color.asString(Common.Color.Format.RGBA));
    TestRunner.addResult(' - ' + color.asString(Common.Color.Format.HSL));
    TestRunner.addResult(' - ' + color.asString(Common.Color.Format.HSLA));

    var hsv = color.hsva();
    var hsvString = String.sprintf(
        'hsv(%d, %d%, %d%)', Math.round(hsv[0] * 360), Math.round(hsv[1] * 100), Math.round(hsv[2] * 100));
    TestRunner.addResult(' - ' + hsvString);
    var hsva = color.hsva();
    var hsvaString = String.sprintf(
        'hsva(%d, %d%, %d%, %f)', Math.round(hsva[0] * 360), Math.round(hsva[1] * 100), Math.round(hsva[2] * 100),
        hsva[3]);
    TestRunner.addResult(' - ' + hsvaString);

    TestRunner.addResult(' - ' + color.asString(Common.Color.Format.HEXA));
    TestRunner.addResult(' - ' + color.asString(Common.Color.Format.HEX));
    TestRunner.addResult(' - ' + color.asString(Common.Color.Format.ShortHEXA));
    TestRunner.addResult(' - ' + color.asString(Common.Color.Format.ShortHEX));
    TestRunner.addResult(' - ' + color.asString(Common.Color.Format.Nickname));
    TestRunner.addResult(' - default: ' + color.asString());

    TestRunner.addResult(' - inverse color: ' + color.invert().asString());
    TestRunner.addResult(' - setAlpha(0.42): ' + color.setAlpha(0.42).asString());
  }

  dumpColor('red');
  dumpColor('green');
  dumpColor('blue');
  dumpColor('cyan');
  dumpColor('magenta');
  dumpColor('yellow');
  dumpColor('white');
  dumpColor('black');

  dumpColor('rgb(94, 126, 91)');
  dumpColor('rgba(94 126 91)');
  dumpColor('rgba(94, 126, 91, 0.5)');
  dumpColor('rgb(94 126 91 / 50%)');

  dumpColor('hsl(212, 55%, 32%)');
  dumpColor('hsla(212 55% 32%)');
  dumpColor('hsla(212, 55%, 32%, 0.5)');
  dumpColor('hsla(212  55%  32% /  50%)');
  dumpColor('hsla(212deg 55% 32% / 50%)');

  dumpColor('#12345678');
  dumpColor('#00FFFF');
  dumpColor('#1234');
  dumpColor('#0FF');
  TestRunner.completeTest();
})();
