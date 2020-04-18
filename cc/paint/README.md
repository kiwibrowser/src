# cc/paint

This document gives an overview of the paint component of cc.

[TOC]

## Overview

cc/paint is a replacement for SkPicture/SkCanvas/SkPaint
recording data structures throughout the Chrome codebase, primarily
meaning Blink and ui.  The reason for a separate data structure
is to change the way that recordings are stored to improve
transport and recording performance.

Skia will still be the ultimate backend for raster, and so
any place in code that still wants to raster directly (either
for test expectations or to create an SkImage, for example)
should continue to use Skia data structures.

## Dependencies

As this component is used in both Blink and ui, it should only include
files that Blink is also allowed to depend on.  This means not including
base/ or using std data structures publicly.

This is why cc/paint uses sk_sp reference counting at the moment as
a compromise between Blink and chrome style.
