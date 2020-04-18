layer at (0,0) size 800x600
  LayoutView at (0,0) size 800x600
layer at (0,0) size 800x600
  LayoutBlockFlow {HTML} at (0,0) size 800x600
    LayoutBlockFlow {BODY} at (8,8) size 784x584
      LayoutBlockFlow {P} at (0,0) size 784x40
        LayoutText {#text} at (0,0) size 202x19
          text run at (0,0) width 202: "This tests for a regression against "
        LayoutInline {I} at (0,0) size 710x39
          LayoutInline {A} at (0,0) size 350x19 [color=#0000EE]
            LayoutText {#text} at (202,0) size 350x19
              text run at (202,0) width 350: "http://bugzilla.opendarwin.org/show_bug.cgi?id=6618"
          LayoutText {#text} at (551,0) size 710x39
            text run at (551,0) width 5: " "
            text run at (555,0) width 155: "Inline in RTL block with"
            text run at (0,20) width 349: "overflow:auto and left border makes scroll bar appear"
        LayoutText {#text} at (348,20) size 5x19
          text run at (348,20) width 5: "."
      LayoutBlockFlow {HR} at (0,56) size 784x2 [border: (1px inset #EEEEEE)]
layer at (8,74) size 784x20 clip at (18,74) size 774x20
  LayoutBlockFlow {DIV} at (0,66) size 784x20 [border: none (10px solid #0000FF)]
    LayoutText {#text} at (484,0) size 300x19
      text run at (484,0) width 110: "This block should "
      text run at (780,0) width 4 RTL: "."
    LayoutInline {EM} at (0,0) size 21x19
      LayoutText {#text} at (594,0) size 21x19
        text run at (594,0) width 21: "not"
    LayoutText {#text} at (615,0) size 165x19
      text run at (615,0) width 165: " have a horizontal scroll bar"
