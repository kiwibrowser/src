# Wrapper Tracing Reference

This document describes wrapper tracing and how its API is supposed to be used.

[TOC]

## Quickstart guide

Wrapper tracing is used to represent reachability across V8 and Blink. The
following checklist highlights the modifications needed to make a class
participate in wrapper tracing.

1. Make sure that objects participating in tracing either inherit from
`ScriptWrappable` (if they can reference wrappers) or `TraceWrapperBase`
(transitively holding wrappers alive).
2. Use `TraceWrapperMember<T>` to annotate fields that need to be followed to
find other wrappers that this object should keep alive.
3. Use `TraceWrapperV8Reference<T>` to annotate references to V8 that this
object should keep alive.
4. Declare a `virtual void TraceWrappers(ScriptWrappableVisitor*) const`
method to trace other wrappers.
5. Define the method and trace all fields that received a wrapper tracing type
in (1) and (2) using `visitor->TraceWrappers(<field_>)` in the body.

The following example illustrates these steps:

```c++
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperMember.h"
#include "platform/bindings/TraceWrapperV8Reference.h"

class SomeDOMObject : public ScriptWrappable {          // (1)
  DEFINE_WRAPPERTYPEINFO();

 public:
  virtual void TraceWrappers(
      ScriptWrappableVisitor*) const;             // (4)

 private:
  TraceWrapperMember<OtherWrappable> other_wrappable_;  // (2)
  Member<NonWrappable> non_wrappable_;
  TraceWrapperV8Reference<v8::Value> v8object_;         // (3)
  // ...
};

void SomeDOMObject::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {      // (5)
  visitor->TraceWrappers(other_wrappable_);             // (5)
  visitor->TraceWrappers(v8object_);                    // (5)
}
```

For more in-depth information and how to deal with corner cases continue on reading.

## Background

Blink and V8 need to cooperate to collect JavaScript *wrappers*. Each V8
*wrapper* object (*W*) in JavaScript is assigned a C++ *DOM object* (*D*) in
Blink. A single C++ *DOM object* can hold onto one or many *wrapper* objects.
During a garbage collection initiated from JavaScript, a *wrapper* then keeps
the  C++ *DOM object* and all its transitive dependencies, including other
*wrappers*, alive, effectively tracing paths like
*W<sub>x</sub> -> D<sub>1</sub>  -> ⋯ -> D<sub>n</sub> -> W<sub>y</sub>*.

Previously, this relationship was expressed using so-called object groups,
effectively assigning all C++ *DOM objects* in a given DOM tree the same group.
The group would only die if all objects belonging to the same group die. A brief
introduction on the limitations on this approach can be found in
[this slide deck][object-grouping-slides].

Wrapper tracing solves this problem by determining liveness based on
reachability by tracing through the C++ *DOM objects*. The rest of this document
describes how the API is used to keep JavaScript wrapper objects alive.

Note that *wrappables* have to be *on-heap objects* and thus all
[Oilpan-related rules][oilpan-docs] apply.

[object-grouping-slides]: https://docs.google.com/presentation/d/1I6leiRm0ysSTqy7QWh33Gfp7_y4ngygyM2tDAqdF0fI/
[oilpan-docs]: https://chromium.googlesource.com/chromium/src/+/master/third_party/blink/renderer/platform/heap/BlinkGCAPIReference.md

## Basic usage

The annotations that are required can be found in the following header files.
Pick the header file depending on what types are needed.

```c++
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/trace_wrapper_v8_reference.h"
```

The following example will guide through the modifications that are needed to
adjust a given class `SomeDOMObject` to participate in wrapper tracing.

```c++
class SomeDOMObject : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

  // ...
 private:
  Member<OtherWrappable /* extending ScriptWrappable */> other_wrappable_;
  Member<NonWrappable> non_wrappable_;
};
```

In this scenario `SomeDOMObject` is the object that is wrapped by an object on
the JavaScript side. The next step is to identify the paths that lead to other
wrappables. In this case, only `other_wrappable_` needs to be traced to find
other *wrappers* in V8.

```c++
class SomeDOMObject : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  virtual void TraceWrappers(ScriptWrappableVisitor*) const;

 private:
  Member<OtherWrappable> other_wrappable_;
  Member<NonWrappable> non_wrappable_;
};

void SomeDOMObject::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(other_wrappable_);
}
```


Blink and V8 implement *incremental* wrapper tracing, which means that marking
can be interleaved with JavaScript or even DOM operations. This poses a
challenge, because already marked objects will not be considered again if they
are reached through some other path.

For example, consider an object `A` that has already been marked and a write
to a field `A.x` setting `x` to an unmarked object `Y`.  Since `A.x` is
the only reference keeping `Y`, and `A` has already been marked, the garbage
collector will not find `Y` and reclaim it.

To overcome this problem we require all writes of interesting objects, i.e.,
writes to traced fields, to go through a write barrier.
The write barrier will check for the problem case above and make sure
`Y` will be marked. In order to automatically issue a write barrier
`other_wrappable_` needs `TraceWrapperMember` type.

```c++
class SomeDOMObject : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  virtual void TraceWrappers(ScriptWrappableVisitor*) const;

 private:
  TraceWrapperMember<OtherWrappable> other_wrappable_;
  Member<NonWrappable> non_wrappable_;
};

void SomeDOMObject::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(other_wrappable_);
}
```

`TraceWrapperMember` makes sure that any write to `other_wrappable_` will
consider doing a write barrier. Using the proper type, the write barrier is
correct by construction, i.e., it will never be missed.

## Heap collections

The proper type usage for collections, e.g. `HeapVector` looks like the
following.

```c++
class SomeDOMObject : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // ...
  void AppendNewValue(OtherWrappable* newValue);
  virtual void TraceWrappers(ScriptWrappableVisitor*) const;

 private:
  HeapVector<TraceWrapperMember<OtherWrappable>> other_wrappables_;
};

void SomeDOMObject::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  for (auto other : other_wrappables_)
    visitor->TraceWrappers(other);
}
```

Note that this is different to Oilpan which can just trace the whole collection.
`TraceWrapperMember` can be constructed in place, so  using `append` and
friends will work out of the box, e.g.

```c++
void SomeDOMObject::AppendNewValue(OtherWrappable* newValue) {
  other_wrappables_.append(newValue);
}
```

The compiler will throw an error for each omitted `TraceWrapperMember`
construction.

### Swapping `HeapVector` containing `TraceWrapperMember` and `Member`

It is possible to swap two `HeapVectors` containing `TraceWrapperMember` and
`Member` by using `blink::swap`. The underlying swap will avoid copies and
write barriers if possible.

```c++
// Swap two wrapper traced heap vectors.
HeapVector<TraceWrapperMember<Wrappable>> a;
HeapVector<TraceWrapperMember<Wrappable>> b;
blink::swap(a, b);

// Swap in a non-traced heap vector into a wrapper traced one.
HeapVector<TraceWrapperMember<Wrappable>> c;
HeapVector<Member<Wrappable>> temporary;
blink::swap(c, temporary);
```

## Tracing through non-`ScriptWrappable` types

Sometimes it is necessary to trace through types that do not inherit from
`ScriptWrappable`. For example, consider the object graph
`A -> B -> C` where both `A` and `C` are `ScriptWrappable`s that
need to be traced.

In this case, the same rules as with `ScriptWrappables` apply, except for the
difference that these classes need to inherit from `TraceWrapperBase`.

### Memory-footprint critical uses

In the case we cannot afford inheriting from `TraceWrapperBase`, which will
add a vtable pointer for tracing wrappers, use
`DEFINE_TRAIT_FOR_TRACE_WRAPPERS(ClassName)` after defining
`ClassName` to define the proper tracing specializations.

## Explicit write barriers

Sometimes it is necessary to stick with the regular types and issue the write
barriers explicitly. In this case, tracing needs to be adjusted to tell the
system that all barriers will be done manually.

```c++
class ManualWrappable : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  void setNew(OtherWrappable* newValue) {
    other_wrappable_ = newValue;
    SriptWrappableVisitor::WriteBarrier(other_wrappable_);
  }

  virtual void TraceWrappers(ScriptWrappableVisitor*) const;
 private:
  Member<OtherWrappable>> other_wrappable_;
};

void ManualWrappable::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappersWithManualWriteBarrier(other_wrappable_);
}
```
