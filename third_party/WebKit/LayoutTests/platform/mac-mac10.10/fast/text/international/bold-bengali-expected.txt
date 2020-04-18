layer at (0,0) size 800x600
  LayoutView at (0,0) size 800x600
layer at (0,0) size 800x600
  LayoutBlockFlow {HTML} at (0,0) size 800x600
    LayoutBlockFlow {BODY} at (8,8) size 784x576
      LayoutBlockFlow {P} at (0,0) size 784x36
        LayoutText {#text} at (0,0) size 755x36
          text run at (0,0) width 351: "This test verifies we can render bold Bengali properly. "
          text run at (350,0) width 405: "(This is complicated on Linux because it's typically covered by"
          text run at (0,18) width 473: "fake-bolded FreeSans even though there's also a FreeSansBold available.)"
      LayoutBlockFlow {P} at (0,52) size 784x18
        LayoutText {#text} at (0,0) size 751x18
          text run at (0,0) width 487: "The test passes if the two words below look similar, but the top one is bold. "
          text run at (486,0) width 265: "There should be no missing-glyph boxes."
      LayoutBlockFlow {P} at (0,86) size 784x29
        LayoutInline {B} at (0,0) size 38x18
          LayoutText {#text} at (0,3) size 38x18
            text run at (0,3) width 38: "\x{9AC}\x{9BE}\x{982}\x{9B2}\x{9BE}"
      LayoutBlockFlow {P} at (0,131) size 784x0
      LayoutBlockFlow {P} at (0,131) size 784x29
        LayoutText {#text} at (0,3) size 29x18
          text run at (0,3) width 29: "\x{9AC}\x{9BE}\x{982}\x{9B2}\x{9BE}"
