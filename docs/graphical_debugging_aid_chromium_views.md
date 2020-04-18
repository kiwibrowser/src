# Graphical Debugging Aid for Chromium Views

## Introduction

A simple debugging tool exists to help visualize the views tree during
debugging. It consists of 4 components:

1.  The function `views::PrintViewGraph()` (in the file
    `ui/views/debug_utils.h`),
1.  a gdb script file `viewg.gdb` (see below),
1.  the graphViz package (http://www.graphviz.org/ - downloadable for Linux,
    Windows and Mac), and
1.  an SVG viewer (_e.g._ Chrome).

## Details

To use the tool,

1.  Make sure you have 'dot' installed (part of graphViz),
1.  run gdb on your build and
1.  `source viewg.gdb` (this can be done automatically in `.gdbinit`),
1.  stop at any breakpoint inside class `View` (or any derived class), and
1.  type `viewg` at the gdb prompt.

This will cause the current view, and any descendants, to be described in a
graph which is stored as `~/state.svg` (Windows users may need to modify the
script slightly to run under CygWin). If `state.svg` is kept open in a browser
window and refreshed each time `viewg` is run, then it provides a graphical
representation of the state of the views hierarchy that is always up to date.

It is easy to modify the gdb script to generate PDF in case viewing with evince
(or other PDF viewer) is preferred.

If you don't use gdb, you may be able to adapt the script to work with your
favorite debugger. The gdb script invokes

    views::PrintViewGraph(this)

on the current object, returning `std::string`, whose contents must then be
saved to a file in order to be processed by dot.

## viewg.gdb

```
define viewg
  if $argc != 0
    echo Usage: viewg
  else
    set pagination off
    set print elements 0
    set logging off
    set logging file ~/state.dot
    set logging overwrite on
    set logging redirect on
    set logging on
    printf "%s\n", view::PrintViewGraph(this).c_str()
    set logging off
    shell dot -Tsvg -o ~/state.svg ~/state.dot
    set pagination on
  end
end
```
