layer at (0,0) size 800x600
  LayoutView at (0,0) size 800x600
layer at (0,0) size 800x600
  LayoutBlockFlow {HTML} at (0,0) size 800x600
    LayoutBlockFlow {BODY} at (8,8) size 784x576
      LayoutBlockFlow {P} at (0,0) size 784x20
        LayoutInline {A} at (0,0) size 58x19 [color=#0000EE]
          LayoutText {#text} at (0,0) size 58x19
            text run at (0,0) width 58: "bug 8626"
        LayoutText {#text} at (58,0) size 348x19
          text run at (58,0) width 7: ": "
          text run at (65,0) width 341: "Strict mode erroneously triggered by a broken comment."
      LayoutBlockFlow {P} at (0,36) size 784x20 [color=#00FF00]
        LayoutText {#text} at (0,0) size 611x19
          text run at (0,0) width 611: "This text should be green, not black (CSS color values not beginning with '#' are OK in quirks mode)."
