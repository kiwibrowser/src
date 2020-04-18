# HTML Imports

The `Source/core/html/imports` directory contains the implementation of
HTML Imports.

The spec can be found [here](https://w3c.github.io/webcomponents/spec/imports/).

## Basic Data Structure and Algorithms of HTML Imports implementation.

### The Import Tree

HTML Imports form a tree:

* The root of the tree is `HTMLImportTreeRoot`.
* The `HTMLImportTreeRoot` is owned by `HTMLImportsController`, which is owned
  by the master document as a `DocumentSupplement`.
* The non-root nodes are `HTMLImportChild`. They are all owned by
  `HTMLImporTreeRoot`.  `LinkStyle` is wired into `HTMLImportChild` by
  implementing `HTMLImportChildClient` interface.
* Both `HTMLImportTreeRoot` and `HTMLImportChild` are derived from `HTMLImport`
  superclass that models the tree data structure using `WTF::TreeNode` and
  provides a set of virtual functions.

`HTMLImportsController` also owns all loaders in the tree and manages their
lifetime through it.  One assumption is that the tree is append-only and
nodes are never inserted in the middle of the tree nor removed.

Full diagram is [here](https://docs.google.com/drawings/d/1jFQrO0IupWrlykTNzQ3Nv2SdiBiSz4UE9-V3-vDgBb0/)

## Import Sharing and `HTMLImportLoader`

[The HTML Imports spec](https://w3c.github.io/webcomponents/spec/imports/) calls
for de-dup mechanism to share already loaded imports.
To implement this, the actual loading machinery is split out from
`HTMLImportChild` to `HTMLImportLoader`, and each loader shares
`HTMLImportLoader` with other loader if the URL is same.  Check around
`HTMLImportTreeRoot::Find()` for more detail.

`HTMLImportLoader` can be shared by multiple imports.

```
   HTMLImportChild (1)-->(*) HTMLImportLoader
```

## Script Blocking

- An import blocks the HTML parser of its own imported document from running
  `<script>` until all of its children are loaded.  Note that dynamically added
  import won't block the parser.
- An import under loading also blocks imported documents that follow from
  being created.  This is because an import can include another import that
  has same URLs of following ones.  In such case, the preceding import should
  be loaded and following ones should be de-duped.