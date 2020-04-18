layer at (0,0) size 800x600
  LayoutView at (0,0) size 800x600
layer at (0,0) size 800x202
  LayoutBlockFlow {html} at (0,0) size 800x202
    LayoutBlockFlow {body} at (8,16) size 784x178
      LayoutBlockFlow {p} at (0,0) size 784x20
        LayoutText {#text} at (0,0) size 284x19
          text run at (0,0) width 284: "In this test, the TFOOT text should be justified."
      LayoutTable {table} at (0,36) size 300x142 [border: (1px outset #808080)]
        LayoutTableSection {thead} at (1,1) size 298x28
          LayoutTableRow {tr} at (0,2) size 298x24
            LayoutTableCell {td} at (2,2) size 294x24 [border: (1px inset #808080)] [r=0 c=0 rs=1 cs=1]
              LayoutText {#text} at (2,2) size 155x19
                text run at (2,2) width 155: "This text is in the THEAD"
        LayoutTableSection {tfoot} at (1,55) size 298x86
          LayoutTableRow {tr} at (0,0) size 298x84
            LayoutTableCell {td} at (2,0) size 294x84 [border: (1px inset #808080)] [r=0 c=0 rs=1 cs=1]
              LayoutText {#text} at (2,2) size 290x79
                text run at (2,2) width 290: "This TFOOT text should be justified which"
                text run at (2,22) width 124: "means that the right "
                text run at (126,22) width 166: "and left margins should line"
                text run at (2,42) width 236: "up, no matter how long the content "
                text run at (238,42) width 54: "becomes"
                text run at (2,62) width 289: "(except the last line which should be left aligned)"
        LayoutTableSection {tbody} at (1,29) size 298x26
          LayoutTableRow {tr} at (0,0) size 298x24
            LayoutTableCell {td} at (2,0) size 294x24 [border: (1px inset #808080)] [r=0 c=0 rs=1 cs=1]
              LayoutText {#text} at (2,2) size 157x19
                text run at (2,2) width 157: "This text is in the TBODY"
