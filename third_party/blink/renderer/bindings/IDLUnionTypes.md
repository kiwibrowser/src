# How to use Blink IDL Union Types

Using [IDL union types](https://heycam.github.io/webidl/#idl-union) in
Blink is a bit tricky. Here are some tips to use union types
correctly.

## Generated classes

For each union type, the code generator creates a C++ class which is
used like an "impl" class of a normal interface type. The name of a
generated class is a
[type name](https://heycam.github.io/webidl/#dfn-type-name) of the
union type. For example, the code generator will create
`StringOrFloat` class for `(DOMString or float)`.

## Paths for generated classes

The code generator puts generated classes into separate files. You need
to include generated header files when you use union types in
core/modules.

The file name for a generated class is basically the same as its class
name, but we use some aliases to avoid too-long file names
(See http://crbug.com/611437 why we need to avoid long file names).
Currently we use following alias(es).

```
CanvasRenderingContext2DOrWebGLRenderingContextOrWebGL2RenderingContextOrImageBitmapRenderingContextOrXRPresentationContext -> RenderingContext
```

The paths for generated classes depend on the places union types are
used. If a union type is used only by IDL files under modules/, the
include path is `bindings/modules/v8/FooOrBar.h`. Otherwise, the
include path is `bindings/core/v8/FooOrBar.h`. For example, given
following definitions:

```webidl
// core/fileapi/FileReader.idl
readonly attribute (DOMString or ArrayBuffer)? result;

// dom/CommonDefinitions.idl
typedef (ArrayBuffer or ArrayBufferView) BufferSource;

// modules/encoding/TextDecoder.idl
DOMString decode(optional BufferSource input, optional TextDecodeOptions options);

// modules/fetch/Request.idl
typedef (Request or USVString) RequestInfo;
```

The include paths will be:
- bindings/core/v8/StringOrArrayBuffer.h
- bindings/core/v8/ArrayBufferOrArrayBufferView.h
- bindings/modules/v8/RequestOrUSVString.h

Note that `ArrayBufferOrArrayBufferView` is located under core/ even
it is used by `Request.idl` which is located under modules/.

**Special NOTE**: If you are going to use a union type under core/ and
the union type is currently used only under modules/, you will need
to update the include path for the union type under modules/.

## Updating GN/GYP files
TODO(bashi): Mitigate the pain of updating GN/GYP files.

Due to the requirements of GN/GYP, we need to put generated file names
in gni/gypi files. Please update
`bindings/core/v8/generated.{gni,gypi}` and/or
`bindings/modules/v8/generated.{gni,gypi}` accordingly.
