# `Source/core/html/canvas`

Contains canvas-related support classes, including:

- the base class for all `CanvasRenderingContext`
- the base class for all elements that can host a rendering context
 (`CanvasRenderingContextHost`), namely `HTMLCanvasElement` and
 `OffscreenCanvas`
- canvas font cache
- canvas async blob creator
- base class for `CanvasImageSource` (used as source for `drawImage`)
- base class for `ImageElements` (`HTMLImageElement` and `SVGImageElement`) that
can be both a `CanvasImageSource` and a `ImageBitmapSource`

For more information on the structure of canvas-related classes, check
[Source/modules/canvas/README.md](../../../modules/canvas/README.md).
