This Markdown file intends to demystify some of the StyleRecalc process:
TODO(crbug.com/795634): Add more details about the StyleRecalc process here. 

## Different StyleRecalc states

Different StyleRecalc states are stored in blink::ComputedStyleConstants in blink::StyleRecalcChange.

The states are as follows:
1. NoChange -> No style recalc change is needed.
2. NoInherit -> Only perform style recalc on the Node itself.
3. UpdatePseudoElements -> Causes an update of just the pseudo element children in the case where the styles for the element itself were recalced, but resulted in NoChange or NoInherit.
4. IndependentInherit -> Same as Inherit except style recalc stops early if only independent properties were changed. We still visit every descendant, but we apply the styles directly instead of doing selector matching to compute a new style. Independent properties are those which do not depend on and do not affect any other properties on ComputedStyle (e.g. visibility and Pointer Events).
5. Inherit -> Do a full style recalc of children.
6. Force -> Fallback that causes us to do a full style recalc. This is as we don't know what changes. The primary reason for it is SubtreeStyleChange.
7. Reattach -> reattachLayoutTree() needs to be performed.
