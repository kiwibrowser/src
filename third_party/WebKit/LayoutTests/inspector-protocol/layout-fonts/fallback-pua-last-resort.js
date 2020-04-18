(async function(testRunner) {
  var page = await testRunner.createPage();
  await page.loadHTML(`
    <html>
    <meta charset="UTF-8">
    <body>
        <div class="test">
            <!-- Private use area Ranges: U+E000..U+F8FF, U+F0000..U+FFFFF, U+100000..U+10FFFF -->
            <div id="pua_test_run">&#xE000;&#xE401;&#xE402;&#xE403;&#xF8FF;&#xF0000;&#xFAAAA;&#xFFFFF;&#x100000;&#x10AAAA;&#x10FFFF;</div>
        </div>
    </body>
    </html>
  `);
  var session = await page.createSession();
  testRunner.log(`Test passes if the fallback font selected for the Unicode Private Use Area test text matches known last resort fonts on each platform.`);

  var helper = await testRunner.loadScript('./resources/layout-font-test.js');
  var results = await helper(testRunner, session);

  var pua_used_fonts = results.find(x => x.selector === '#pua_test_run').usedFonts;
  var passed = pua_used_fonts.length === 1 && /^(Times New Roman|Times|DejaVu Sans|Arial)$/.test(pua_used_fonts[0].familyName) && pua_used_fonts[0].glyphCount == 11;
  testRunner.log(passed ? 'PASS' : 'FAIL');
  testRunner.completeTest();
})
