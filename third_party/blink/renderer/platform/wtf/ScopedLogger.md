# Debugging with ScopedLogger

## Overview

ScopedLogger is a logger that shows nested calls by indenting.

For example, if you were debugging a layout issue you could add a ScopedLogger
to the top of the `LayoutBlock::layout` function:

```c++
void LayoutBlock::layout()
{
    WTF_CREATE_SCOPED_LOGGER(logger, "layout %s", debugName().utf8().data());
    ...
```

The arguments of the `WTF_CREATE_SCOPED_LOGGER` macro are the name of the
object, followed by the first log message, followed by printf-style varargs.
In the above example, the log message includes the debug name of the block that
is currently being laid out.

ScopedLogger wraps log messages in parentheses, with indentation proportional to
the number of instances.  This makes it easy to see the flow of control in the
output, particularly when instrumenting recursive functions.  Here is some of
the output of the above example when laying out www.google.com:

```
( layout LayoutView #document
  ( layout LayoutBlockFlow HTML
    ( layout LayoutBlockFlow BODY id='gsr' class='hp vasq'
      ( layout LayoutBlockFlow (relative positioned) DIV id='viewport' class='ctr-p'
        ( layout LayoutBlockFlow DIV id='doc-info' )
        ( layout LayoutBlockFlow DIV id='cst' )
        ( layout LayoutBlockFlow (positioned) A )
        ( layout LayoutBlockFlow (positioned) DIV id='searchform' class='jhp' )
      )
    )
  )
)
```

## Appending to a ScopedLogger

Every ScopedLogger has an initial log message, which is often sufficient.  But
you can also write additional messages to an existing ScopedLogger with
`WTF_APPEND_SCOPED_LOGGER`.  For example:

```c++
    // further down in LayoutBlock::layout...

    if (needsScrollAnchoring) {
        WTF_APPEND_SCOPED_LOGGER(logger, "restoring scroll anchor");
        getScrollableArea()->scrollAnchor().restore();
    }
```

## Conditional ScopedLoggers

It's often useful to create a ScopedLogger only if some condition is met.
Unfortunately, the following doesn't work correctly:

```c++
void foo() {
    if (condition) {
        WTF_CREATE_SCOPED_LOGGER(logger, "foo, with condition");
        // Oops: logger exits scope prematurely!
    }
    bar();  // any ScopedLogger in bar won't nest
}
```

To guard a ScopedLogger construction with a condition without restricting its
scope, use `WTF_CREATE_SCOPED_LOGGER_IF`:

```c++
void foo() {
    WTF_CREATE_SCOPED_LOGGER_IF(logger, condition, "message");
    bar();
}
```

## Requirements

The ScopedLogger class and associated macros are defined in
[Assertions.h](Assertions.h), which most Blink source files already include
indirectly.  ScopedLogger can't be used outside of Blink code yet.

The ScopedLogger macros work in debug builds by default.  They are compiled out
of release builds, unless your `GYP_DEFINES` or GN args file includes one of the
following:

* `dcheck_always_on`: enables assertions and ScopedLogger

The macro names are cumbersome to type, but most editors can be configured to
make this easier.  For example, you can add the following to a Sublime Text key
binding file to make Ctrl+Alt+L insert a ScopedLogger:

```
  { "keys": ["ctrl+alt+l"], "command": "insert",
    "args": {"characters": "WTF_CREATE_SCOPED_LOGGER(logger, \"msg\");"}
  }
```
