layer at (0,0) size 800x600
  LayoutView at (0,0) size 800x600
layer at (0,0) size 800x600
  LayoutBlockFlow {HTML} at (0,0) size 800x600
    LayoutBlockFlow {BODY} at (8,8) size 784x572
      LayoutBlockFlow {P} at (0,0) size 784x18
        LayoutText {#text} at (0,0) size 754x18
          text run at (0,0) width 754: "Tests that selection rects take transforms into account. The red box should be the bounds of the transformed selection."
layer at (28,46) size 622x98
  LayoutBlockFlow {DIV} at (20,38) size 622x98 [border: (1px solid #000000)]
    LayoutBlockFlow {P} at (11,35) size 600x28
      LayoutText {#text} at (0,0) size 135x28
        text run at (0,0) width 135: "Lorem ipsum "
      LayoutInline {SPAN} at (0,0) size 388x28
        LayoutText {#text} at (134,0) size 388x28
          text run at (134,0) width 388: "dolor sit amet, consectetur adipisicing"
      LayoutText {#text} at (521,0) size 44x28
        text run at (521,0) width 44: " elit."
selection start: position 0 of child 0 {#text} of child 1 {SPAN} of child 1 {P} of child 3 {DIV} of body
selection end:   position 5 of child 0 {#text} of child 1 {SPAN} of child 1 {P} of child 3 {DIV} of body
