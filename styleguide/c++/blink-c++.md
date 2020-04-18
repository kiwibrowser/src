# Blink C++ Style Guide

This document is a list of differences from the overall [Chromium Style Guide],
which is in turn a set of differences from the [Google C++ Style Guide]. The
long-term goal is to make both Chromium and Blink style more similar to Google
style over time, so this document aims to highlight areas where Blink style
differs from Google style.

[TOC]

## Use references for all non-null pointer arguments
Pointer arguments that can never be null should be passed as a reference, even
if this results in a mutable reference argument.

> Note: Even though Google style prohibits mutable reference arguments, Blink
style explicitly permits their use.

**Good:**
```c++
// Passed by mutable reference since |frame| is assumed to be non-null.
FrameLoader::FrameLoader(LocalFrame& frame)
    : frame_(&frame),
      progress_tracker_(ProgressTracker::Create(frame)) {
  // ...
}

// Optional arguments should still be passed by pointer.
void LocalFrame::SetDOMWindow(LocalDOMWindow* dom_window) {
  if (dom_window)
    GetScriptController().ClearWindowProxy();

  if (this->DomWindow())
    this->DomWindow()->Reset();
  dom_window_ = dom_window;
}
```

**Bad:**
```c++
// Since the constructor assumes that |frame| is never null, it should be
// passed as a mutable reference.
FrameLoader::FrameLoader(LocalFrame* frame)
    : frame_(frame),
      progress_tracker_(ProgressTracker::Create(frame)) {
  DCHECK(frame_);
  // ...
}
```

## Prefer WTF types over STL types

Outside of `//third_party/blink/common`, Blink should use WTF types. STL string
and container types should only be used at the boundary to interoperate with
'//base', `//third_party/blink/common`, and other Chromium-side code.
Similarly, Blink should prefer `KURL` over `GURL` and `SecurityOrigin` over
`url::Origin`.

**Good:**
```c++
  String title;
  Vector<KURL> urls;
  HashMap<int, Deque<RefPtr<SecurityOrigin>>> origins;
```

**Bad:**
```c++
  std::string title;
  std::vector<GURL> urls;
  std::unordered_map<int, std::deque<url::Origin>> origins;
```

## Naming

### Use `CamelCase` for all function names

All function names should use `CamelCase()`-style names, beginning with an
uppercase letter.

As an exception, method names for web-exposed bindings begin with a lowercase
letter to match JavaScript.

**Good:**
```c++
class Document {
 public:
  // Function names should begin with an uppercase letter.
  virtual void Shutdown();

  // However, web-exposed function names should begin with a lowercase letter.
  LocalDOMWindow* defaultView();

  // ...
};
```

**Bad:**
```c++
class Document {
 public:
  // Though this is a getter, all Blink function names must use camel case.
  LocalFrame* frame() const { return frame_; }

  // ...
};
```

### Precede boolean values with words like “is” and “did”
```c++
bool is_valid;
bool did_send_data;
```

**Bad:**
```c++
bool valid;
bool sent_data;
```

### Precede setters with the word “Set”; use bare words for getters
Precede setters with the word “set”. Prefer bare words for getters. Setter and
getter names should match the names of the variable being accessed/mutated.

If a getter’s name collides with a type name, prefix it with “Get”.

**Good:**
```c++
class FrameTree {
 public:
  // Prefer to use the bare word for getters.
  Frame* FirstChild() const { return first_child_; }
  Frame* LastChild() const { return last_child_; }

  // However, if the type name and function name would conflict, prefix the
  // function name with “Get”.
  Frame* GetFrame() const { return frame_; }

  // ...
};
```

**Bad:**
```c++
class FrameTree {
 public:
  // Should not be prefixed with “Get” since there's no naming conflict.
  Frame* GetFirstChild() const { return first_child_; }
  Frame* GetLastChild() const { return last_child_; }

  // ...
};
```

### Precede getters that return values via out-arguments with the word “Get”
**Good:**
```c++
class RootInlineBox {
 public:
  Node* GetLogicalStartBoxWithNode(InlineBox*&) const;
  // ...
}
```

**Bad:**
```c++
class RootInlineBox {
 public:
  Node* LogicalStartBoxWithNode(InlineBox*&) const;
  // ...
}
```

### May leave obvious parameter names out of function declarations
[Google C++ Style Guide] allows us to leave parameter names out only if
the parameter is not used. In Blink, you may leave obvious parameter
names out of function declarations for historical reason. A good rule of
thumb is if the parameter type name contains the parameter name (without
trailing numbers or pluralization), then the parameter name isn’t needed.

**Good:**
```c++
class Node {
 public:
  Node(TreeScope* tree_scope, ConstructionType construction_type);
  // You may leave them out like:
  // Node(TreeScope*, ConstructionType);

  // The function name makes the meaning of the parameters clear.
  void SetActive(bool);
  void SetDragged(bool);
  void SetHovered(bool);

  // Parameters are not obvious.
  DispatchEventResult DispatchDOMActivateEvent(int detail,
                                               Event& underlying_event);
};
```

**Bad:**
```c++
class Node {
 public:
  // ...

  // Parameters are not obvious.
  DispatchEventResult DispatchDOMActivateEvent(int, Event&);
};
```

## Prefer enums to bools for function parameters
Prefer enums to bools for function parameters if callers are likely to be
passing constants, since named constants are easier to read at the call site.
An exception to this rule is a setter function, where the name of the function
already makes clear what the boolean is.

**Good:**
```c++
// An named enum value makes it clear what the parameter is for.
if (frame_->Loader().ShouldClose(CloseType::kNotForReload)) {
  // No need to use enums for boolean setters, since the meaning is clear.
  frame_->SetIsClosing(true);

  // ...
```

**Bad:**
```c++
// Not obvious what false means here.
if (frame_->Loader().ShouldClose(false)) {
  frame_->SetIsClosing(ClosingState::kTrue);

  // ...
```

## Comments
Please follow the standard [Chromium Documentation Guidelines]. In particular,
most classes should have a class-level comment describing the purpose, while
non-trivial code should have comments describing why the code is written the
way it is. Often, what is apparent when the code is written is not so obvious
a year later.

From [Google C++ Style Guide: Comments]:
> Giving sensible names to types and variables is much better than obscure
> names that must be explained through comments.

### Use `README.md` to document high-level components

Documentation for a related-set of classes and how they interact should be done
with a `README.md` file in the root directory of a component.

### TODO style

Comments for future work should use `TODO` and have a name or bug attached.

From [Google C++ Style Guide: TODO Comments]:

> The person named is not necessarily the person who must fix it.

**Good:**
```c++
// TODO(dcheng): Clean up after the Blink rename is done.
// TODO(https://crbug.com/675877): Clean up after the Blink rename is done.
```

**Bad:**
```c++
// FIXME(dcheng): Clean up after the Blink rename is done.
// FIXME: Clean up after the Blink rename is done.
// TODO: Clean up after the Blink rename is done.
```

[Chromium Style Guide]: c++.md
[Google C++ Style Guide]: https://google.github.io/styleguide/cppguide.html
[Chromium Documentation Guidelines]: ../../docs/documentation_guidelines.md
[Google C++ Style Guide: Comments]: https://google.github.io/styleguide/cppguide.html#Comments
[Google C++ Style Guide: TODO Comments]:https://google.github.io/styleguide/cppguide.html#TODO_Comments
