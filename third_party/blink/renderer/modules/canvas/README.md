# `Source/modules/canvas`

Contains canvas-related subdirectories.

## Class structure of Canvas-related objects

The classes on this structure are divided between all subdirectories that are
used by canvas:

1. `htmlcanvas`

   Contains context creation for HTML canvas element

2. `canvas2d`

   Contains base class for 2D rendering contexts and `CanvasRenderingContext2D` related classes.

3. `offscreencanvas`

   Contains offscreencanvas object.

4. `offscreencanvas2d`

   Contains offscreencanvas 2D rendering context.

5. `imagebitmap`

   Contains ImageBitmap rendering context.

Canvas-related classes are also present in these directories in `core`:
`core/html/canvas` and `core/html`.

There are also some other directories in `modules` that are a bit related to
canvas: `modules/webgl`, `modules/imagebitmap` and `modules/csspaint`.

### Virtual classes

`CanvasRenderingContextHost` : All elements that provides rendering contexts
(`HTMLCanvasElement` and `OffscreenCanvas`) This is the main interface that a
`CanvasRenderingContext` uses.

`CanvasRenderingContext` - Base class for everything that exposes a rendering
context API. This includes `2d`, `webgl`, `webgl2`, `imagebitmap` contexts.

`BaseRenderingContext2D` - Class for `2D` canvas contexts. Implements most 2D
rendering context API. Used by `CanvasRenderingContext2D`,
`OffscreenCanvasRenderingContext2D` and `PaintRenderingContext2D`.

`WebGLRenderingContextBase` - Base class for `webgl` contexts.

### Final classes

`CanvasRenderingContext2D` - 2D context for HTML Canvas element. [[spec](https://html.spec.whatwg.org/multipage/scripting.html#2dcontext)]

`OffscreenCanvasRenderingContext2D` - 2D context for OffscreenCanvas.
[[spec](https://html.spec.whatwg.org/multipage/scripting.html#the-offscreen-2d-rendering-context)]

`WebGLRenderingContext` - WebGL context for both HTML and Offscreen canvas.
[[spec](https://www.khronos.org/registry/webgl/specs/latest/1.0/#5.14)]

`WebGL2RenderingContext` - WebGL2 context for both HTML and Offscreen canvas.
[[spec](https://www.khronos.org/registry/webgl/specs/latest/2.0/#3.7)]

`ImageBitmapRenderingContext` - The rendering context provided by `ImageBitmap`.
[[spec](https://html.spec.whatwg.org/multipage/scripting.html#the-imagebitmap-rendering-context)]

`PaintRenderingContext2D` - Rendering context for CSS Painting.
[[spec](https://www.w3.org/TR/css-paint-api-1/)]
