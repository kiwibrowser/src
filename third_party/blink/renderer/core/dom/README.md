# DOM

[Rendered](https://chromium.googlesource.com/chromium/src/+/master/third_party/blink/renderer/core/dom/README.md)

Author: hayato@chromium.org

The `Source/core/dom` directory contains the implementation of [DOM].

[DOM]: https://dom.spec.whatwg.org/
[DOM Standard]: https://dom.spec.whatwg.org/

Basically, this directory should contain only a file which is related to [DOM Standard].
However, for historical reasons, `Source/core/dom` directory has been used
as if it were *misc* directory. As a result, unfortunately, this directory
contains a lot of files which are not directly related to DOM.

Please don't add unrelated files to this directory any more.  We are trying to
organize the files so that developers wouldn't get confused at seeing this
directory.

-   See the [spreadsheet](https://docs.google.com/spreadsheets/d/1OydPU6r8CTj8HC4D9_gVkriJETu1Egcw2RlajYcw3FM/edit?usp=sharing), as a rough plan to organize Source/core/dom files.

    The classification in the spreadsheet might be wrong. Please update the spreadsheet, and move files if you can,
    if you know more appropriate places for each file.

-   See [crbug.com/738794](http://crbug.com/738794) for tracking our efforts.


# Node and Node Tree

In this README, we draw a tree in left-to-right direction. `A` is the root of the tree.


``` text
A
├───B
├───C
│   ├───D
│   └───E
└───F
```


`Node` is a base class of all kinds of nodes in a node tree. Each `Node` has following 3 pointers (but not limited to):

-   `parent_or_shadow_host_node_`: Points to the parent (or the shadow host if it is a shadow root; explained later)
-   `previous_`: Points to the previous sibling
-   `next_`: Points to the next sibling

`ContainerNode`, from which `Element` extends, has additional pointers for its child:

-   `first_child_`: The meaning is obvious.
-   `last_child_`: Nit.

That means:
- Siblings are stored as a linked list. It takes O(N) to access a parent's n-th child.
- Parent can't tell how many children it has in O(1).

Further info:
- `Node`, `ContainerNode`

# C++11 range-based for loops for traversing a tree

You can traverse a tree manually:

``` c++
// In C++

// Traverse a children.
for (Node* child = parent.firstChild(); child; child = child->nextSibling()) {
  ...
}

// ...

// Traverse nodes in tree order, depth-first traversal.
void foo(const Node& node) {
  ...
  for (Node* child = node.firstChild(); child; child = child->nextSibling()) {
    foo(*child);  // Recursively
  }
}
```

However, traversing a tree in this way might be error-prone.
Instead, you can use `NodeTraversal` and `ElementTraversal`. They provides a C++11's range-based for loops, such as:

``` c++
// In C++
for (Node& child : NodeTraversal::childrenOf(parent) {
  ...
}
```

e.g. Given a parent *A*, this traverses *B*, *C*, and *F* in this order.


``` c++
// In C++
for (Node& node : NodeTraversal::startsAt(root)) {
  ...
}
```

e.g. Given the root *A*, this traverses *A*, *B*, *C*, *D*, *E*, and *F* in this order.

There are several other useful range-based for loops for each purpose.
The cost of using range-based for loops is zero because everything can be inlined.

Further info:

- `NodeTraversal` and `ElementTraversal` (more type-safe version)
- The [CL](https://codereview.chromium.org/642973003), which introduced these range-based for loops.

# Shadow Tree

A **shadow tree** is a node tree whose root is a `ShadowRoot`.
From web developer's perspective, a shadow root can be created by calling `element.attachShadow{ ... }` API.
The *element* here is called a **shadow host**, or just a **host** if the context is clear.

- A shadow root is always attached to another node tree through its host. A shadow tree is therefore never alone.
- The node tree of a shadow root’s host is sometimes referred to as the **light tree**.

For example, given the example node tree:

``` text
A
├───B
├───C
│   ├───D
│   └───E
└───F
```

Web developers can create a shadow root, and manipulate the shadow tree in the following way:

``` javascript
// In JavaScript
const b = document.querySelector('#B');
const shadowRoot = b.attachShadow({ mode: 'open'} )
const sb = document.createElement('div');
shadowRoot.appendChild(sb);
```

The resulting shadow tree would be:

``` text
shadowRoot
└── sb
```

The *shadowRoot* has one child, *sb*. This shadow tree is being *attached* to B:

``` text
A
└── B
    ├──/shadowRoot
    │   └── sb
    ├── C
    │   ├── D
    │   └── E
    └── F
```

In this README, a notation (`──/`) is used to represent a *shadowhost-shadowroot* relationship, in a **composed tree**.
A composed tree will be explained later. A *shadowhost-shadowroot* is 1:1 relationship.

Though a shadow root has always a corresponding shadow host element, a light tree and a shadow tree should be considered separately, from a node tree's perspective. (`──/`) is *NOT* a parent-child relationship in a node tree.

For example, even though *B* *hosts* the shadow tree, *shadowRoot* is not considered as a *child* of *B*.
The means the following traversal:


``` c++
// In C++
for (Node& node : NodeTraversal::startsAt(A)) {
  ...
}
```

traverses only *A*, *B*, *C*, *D*, *E* and *F* nodes. It never visits *shadowRoot* nor *sb*.
NodeTraversal never cross a shadow boundary, `──/`.

Further info:

- `ShadowRoot`
- `Element#attachShadow`

# TreeScope

`Document` and `ShadowRoot` are always the root of a node tree.
Both`Document` and `ShadowRoot` implements `TreeScope`.

`TreeScope` maintains a lot of information about the underlying tree for efficiency.
For example, TreeScope has a *id-to-element* mapping, as [`TreeOrderedMap`](./TreeOrderedMap.h), so that `querySelector('#foo')` can find an element whose id attribute is "foo" in O(1).
In other words,  `root.querySelector('#foo')` can be slow if that is used in a node tree whose root is not `TreeScope`.

Each `Node` has `tree_scope_` pointer, which points to:

- The root node: if the node's root is either Document or ShadowRoot.
- [owner document](https://dom.spec.whatwg.org/#concept-node-documentOwnerDocument), otherwise.

The means `tree_scope_` pointer is always non-null (except for while in a DOM mutation),
but it doesn't always point to the node's root.

Since each node doesn't have a pointer which *always* points to the root,
`Node::getRootNode(...)` may take O(N) if the node is neither in a document tree nor in a shadow tree.
If the node is in TreeScope (`Node#IsInTreeScope()` can tell it), we can get the root in O(1).

Each node has flags, which is updated in DOM mutation, so that we can tell whether the node is in a
document tree, in a shadow tree, or in none of them, by using
`Node::IsInDocumentTree()` and/or `Node::IsInShadowTree()`.

If you want to add new features to `Document`, `Document` might be a wrong place to add.
Instead, please consider to add functionality to `TreeScope`.  We want to treat a document tree and a shadow tree equally as much as possible.

## Example

``` text
document
└── a1
    ├──/shadowRoot1
    │   └── s1
    └── a2
        └── a3

document-fragment
└── b1
    ├──/shadowRoot2
    │   └── t2
    └── b2
        └── b3
```

- Here, there are 4 node trees; The root node of each tree is *document*, *shadowRoot1*, *document-fragment*, and *shadowRoot2*.
- Suppose that each node is created by `document.createElement(...)` (except for Document and ShadowRoot).
  That means each node's **owner document** is *document*.

| node              | node's root              | node's `_tree_scope` points to: |
|-------------------|--------------------------|---------------------------------|
| document          | document (self)          | document (self)                 |
| a1                | document                 | document                        |
| a2                | document                 | document                        |
| a3                | document                 | document                        |
| shadowRoot1       | shadowRoot1 (self)       | shadowRoot1 (self)              |
| s1                | shadowRoot1              | shadowRoot1                     |
| document-fragment | document-fragment (self) | document                        |
| b1                | document-fragment        | document                        |
| b2                | document-fragment        | document                        |
| b3                | document-fragment        | document                        |
| shadowRoot2       | shadowRoot2 (self)       | shadowRoot2 (self)              |
| t1                | shadowRoot2              | shadowRoot2                     |

Further Info:

- [`TreeScope.h`](./TreeScope.h), [`TreeScope.cpp`](./TreeScope.cpp)
- `Node#GetTreeScope()`, `Node#ContainingTreeScope()`, `Node#IsInTreeScope()`

# Composed Tree (a tree of node trees)

In the previous picture, you might think that more than one node trees, a document tree and a shadow tree, were *connected* to each other. That is *true* in some sense.
The following is a more complex example:


``` text
document
├── a1 (host)
│   ├──/shadowRoot1
│   │   └── b1
│   └── a2 (host)
│       ├──/shadowRoot2
│       │   ├── c1
│       │   │   ├── c2
│       │   │   └── c3
│       │   └── c4
│       ├── a3
│       └── a4
└── a5
    └── a6 (host)
        └──/shadowRoot3
            └── d1
                ├── d2
                ├── d3 (host)
                │   └──/shadowRoot4
                │       ├── e1
                │       └── e2
                └── d4 (host)
                    └──/shadowRoot5
                        ├── f1
                        └── f2
```

If you see this carefully, you can notice that this *composed tree* is composed of 6 node trees; 1 document tree and 5 shadow trees:


- document tree

  ``` text
  document
  ├── a1 (host)
  │   └── a2 (host)
  │       ├── a3
  │       └── a4
  └── a5
      └── a6 (host)
  ```

- shadow tree 1

  ``` text
  shadowRoot1
  └── b1
  ```

- shadow tree 2

  ``` text
  shadowRoot2
  ├── c1
  │   ├── c2
  │   └── c3
  └── c4
  ```

- shadow tree 3

  ``` text
  shadowRoot3
  └── d1
      ├── d2
      ├── d3 (host)
      └── d4 (host)
  ```

- shadow tree 4

  ``` text
  shadowRoot4
  ├── e1
  └── e2
  ```

- shadow tree 5

  ``` text
  shadowRoot5
  ├── f1
  └── f2
  ```

If we consider each *node tree* as *node* of a *super-tree*, we can draw a super-tree as such:

``` text
document
├── shadowRoot1
├── shadowRoot2
└── shadowRoot3
    ├── shadowRoot4
    └── shadowRoot5
```

Here, a root node is used as a representative of each node tree; A root node and a node tree itself can be sometimes exchangeable in explanations.

We call this kind of a *super-tree* (*a tree of node trees*) a **composed tree**.
The concept of a *composed tree* is very useful to understand how Shadow DOM's encapsulation works.

[DOM Standard] defines the following terminologies:

- [shadow-including tree order](https://dom.spec.whatwg.org/#concept-shadow-including-tree-order)
- [shadow-including root](https://dom.spec.whatwg.org/#concept-shadow-including-root)
- [shadow-including descendant](https://dom.spec.whatwg.org/#concept-shadow-including-descendant)
- [shadow-including inclusive descendant](https://dom.spec.whatwg.org/#concept-shadow-including-inclusive-descendant)
- [shadow-including ancestor](https://dom.spec.whatwg.org/#concept-shadow-including-ancestor)
- [shadow-including inclusive ancestor](https://dom.spec.whatwg.org/#concept-shadow-including-inclusive-ancestor)
- [closed-shadow-hidden](https://dom.spec.whatwg.org/#concept-closed-shadow-hidden)

For example,

- *d1*'s *shadow-including ancestor nodes* are *shadowRoot3*, *a6*, *a5*, and *document*
- *d1*'s *shadow-including descendant nodes* are *d2*, *d3*, *shadowRoot4*, *e1*, *e2*, *d4*, *shadowRoot5*, *f1*, and *f2*.


To honor Shadow DOM's encapsulation, we have a concept of *visibility relationship* between two nodes.

In the following table, "`-`" means that "node *A* is *visible* from node *B*".


| *A* \ *B* | document | a1     | a2     | b1     | c1     | d1     | d2     | e1     | f1     |
|-----------|----------|--------|--------|--------|--------|--------|--------|--------|--------|
| document  | -        | -      | -      | -      | -      | -      | -      | -      | -      |
| a1        | -        | -      | -      | -      | -      | -      | -      | -      | -      |
| a2        | -        | -      | -      | -      | -      | -      | -      | -      | -      |
| b1        | hidden   | hidden | hidden | -      | hidden | hidden | hidden | hidden | hidden |
| c1        | hidden   | hidden | hidden | hidden | -      | hidden | hidden | hidden | hidden |
| d1        | hidden   | hidden | hidden | hidden | hidden | -      | -      | -      | -      |
| d2        | hidden   | hidden | hidden | hidden | hidden | -      | -      | -      | -      |
| e1        | hidden   | hidden | hidden | hidden | hidden | hidden | hidden | -      | hidden |
| f1        | hidden   | hidden | hidden | hidden | hidden | hidden | hidden | hidden | -      |

For example, *document* is *visible* from any nodes.

To understand *visibility relationship* easily, here is a rule of thumb:

- If node *B* can reach node *A* by traversing an *edge* (in the first picture of this section), recursively, *A* is visible from *B*.
- However, an *edge* of (`──/`) ( *shadowhost-shadowroot* relationship) is one-directional:
  - From a shadow root to the shadow host -> Okay
  - From a shadow host to the shadow root -> Forbidden

In other words, a node in an *inner tree* can see a node in an *outer tree* in a composed tree, but the opposite is not true.

We have designed (or re-designed) a bunch of Web-facing APIs to honor this basic principle.
If you add a new API to the web platform and Blink, please consider this rule and don't *leak* a node which should be hidden to web developers.

Warning: Unfortunately, a *composed tree* had a different meaning in the past; it was used to specify a *flat tree* (which will be explained later).
If you find a wrong usage of a composed tree in Blink, please fix it.

Further Info:

- `TreeScope::ParentTreeScope()`
- `Node::IsConnected()`
- DOM Standard: [connected](https://dom.spec.whatwg.org/#connected)
- DOM Standard: [retarget](https://dom.spec.whatwg.org/#retarget)

# Flat tree

A composed tree itself can't be rendered *as is*. From the rendering's
perspective, Blink has to construct a *layout tree*, which would be used as an input to
the *paint phase*.  A layout tree is a tree whose node is `LayoutObject`, which
points to `Node` in a node tree, plus additional calculated layout information.

Before the Web Platform got Shadow DOM, the structure of a layout tree is almost
*similar* to the structure of a document tree; where only one node tree,
*document tree*, is being involved there.

Since the Web Platform got Shadow DOM, we now have a composed tree which is composed of multiple node
trees, instead of a single node tree. That means We have to *flatten* the composed tree to the one node tree, called
a *flat tree*, from which a layout tree is constructed.

For example, given the following composed tree,

``` text
document
├── a1 (host)
│   ├──/shadowRoot1
│   │   └── b1
│   └── a2 (host)
│       ├──/shadowRoot2
│       │   ├── c1
│       │   │   ├── c2
│       │   │   └── c3
│       │   └── c4
│       ├── a3
│       └── a4
└── a5
    └── a6 (host)
        └──/shadowRoot3
            └── d1
                ├── d2
                ├── d3 (host)
                │   └──/shadowRoot4
                │       ├── e1
                │       └── e2
                └── d4 (host)
                    └──/shadowRoot5
                        ├── f1
                        └── f2

```

This composed tree would be flattened into the following *flat tree* (assuming there are not `<slot>` elements there):


``` text
document
├── a1 (host)
│   └── b1
└── a5
    └── a6 (host)
        └── d1
            ├── d2
            ├── d3 (host)
            │   ├── e1
            │   └── e2
            └── d4 (host)
                ├── f1
                └── f2
```

We can't explain the exact algorithm how to flatten a composed tree into a flat tree until I explain the concept of *slots* and *node distribution*
If we are ignoring the effect of `<slot>`, we can have the following simple definition. A flat tree can be defined as:

- A root of a flat tree: *document*
- Given node *A* which is in a flat tree, its children are defined, recursively, as follows:
  - If *A* is a shadow host, its shadow root's children
  - Otherwise, *A*'s children

# Distribution and slots

TODO(hayato): Explain.

In the meantime, please see [Incremental Shadow DOM](https://docs.google.com/document/d/1R9J8CVaSub_nbaVQwwm3NjCoZye4feJ7ft7tVe5QerM/edit?usp=sharing).

# FlatTreeTraversal

TODO(hayato): Explain.

# DOM mutations

TODO(hayato): Explain.

# Related flags

TODO(hayato): Explain.

# Event path and Event Retargeting

TODO(hayato): Explain.
