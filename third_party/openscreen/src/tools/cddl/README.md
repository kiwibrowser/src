# CDDL Compiler

This CDDL compiler takes a CDDL specification as input and produces a C++ header
and source file which contain structs, enums, encode functions, and decode
functions.  This simplifies the process of taking a CDDL message definition from
the OSP control protocol spec and making it usable in C++.  Additionally, it
simplifies adding new messages or changing existing messages during development.

This compiler is not intended to support all or even most of the CDDL spec.
CDDL allows many patterns that are not useful, practical, or efficient when
considering a C++ implementation of CDDL messages.  Our specialization for enums
is a good example, but more details are given below.

## Usage Overview

This section gives some examples of CDDL syntax that is supported and what the
generated C++ looks like.  For the complete set of messages currently supported
for OSP, see [//msgs/osp.cddl](../../msgs/osp.cddl).

### Maps

The following example shows a map in CDDL:
``` cddl
x = {
  alpha: uint,
  beta: text,
}
```

This translates into a normal C++ struct (i.e. not `std::map`):
``` c++
struct X {
 uint64_t alpha;
 std::string beta;
};
```

The string keys are handled only by the encoding and decoding functions.

### Heterogenous Arrays

An array of heterogeneous (or indeed a fixed number of homogeneous types) such
as
``` cddl
x = [
  alpha: uint,
  beta: text,
]
```
also translates into a plain C++ struct.
``` c++
struct X {
 uint64_t alpha;
 std::string beta;
};
```
In the array case, the field keys are only used as variable names and no strings
are used in encoding.

Because these must be implemented as a C++ struct and we don't want to define an
automatic naming scheme, all array fields must have a key.  For example, CDDL
would allow this definition:
``` cddl
x = [
  uint,
  text,
]
```
but this is not allowed by our compiler.

### Homogeneous Arrays

An array of unspecified length containing only one type:
``` cddl
x = [* uint]
```
is translated to a `std::vector`.  In this case, a key for the single array
field isn't necessary.  It's currently not supported to put length constraints
(e.g. `x = [2*5 uint]`) on the array length.

### Group Inclusion

If common fields are placed in a separate CDDL group (which is not a map or
array), it can be included directly in another map, array, or group type.  So
``` cddl
x = (alpha: uint)
y = {
  x,
  beta: text,
}
```
will translate to the following C++ struct:
``` c++
struct Y {
  uint64_t alpha;
  std::string beta;
};
```
If you prefer that a group is included explicitly as its own struct type, you
should make it a map or array.  For example,
``` cddl
x = {alpha: uint}
y = {
  x: x,
  beta: text,
}
```
will translate to the following C++ struct:
``` c++
struct X {
  uint64_t alpha;
};
struct Y {
  X x;
  std::string beta;
};
```

### Optional Fields

Fields that are not required are prefixed with a '?' in CDDL.
``` cddl
x = { ? alpha: uint }
```
These are translated to a bool flag and value pair:
``` c++
struct X {
  bool has_alpha;
  uint64_t alpha;
};
```

### Choice from a Group as an Enum

CDDL allows specifying a type as one of any member of a group:
``` cddl
x = &(
  alpha: 0,
  beta: 1,
)
```
This is implemented as an enum in C++:
``` c++
enum X {
  kAlpha = 0,
  kBeta = 1,
};
```
Recursive group inclusion in choices handled by the simple fact that plain enum
constants are global and not type-checked.  This leads to a global definition
caveat that is explained below, but here is an example of such an inclusion:
``` cddl
x = ( alpha: 0, beta: 1 )
y = &( x, gamma: 2 )
```
``` c++
enum X {
  kAlpha = 0,
  kBeta = 1,
};
enum Y {
  // union: enum X
  kGamma = 2,
};
```

### Type Choice as a Discriminated Union

Specifying multiple possible types for a value in CDDL
``` cddl
x = { alpha: text / uint }
```
is translated to a discriminated union in C++:
``` c++
struct X {
  X();
  ~X();  // NOTE: This requires defining a ctor/dtor to deal with the union.
  enum class WhichAlpha {
    kString,
    kUint64,
  } which_alpha;
  union {
    std::string str;
    uint64_t uint;
  } alpha;
};
```
Currently, only `uint`, `text`, and `bytes` are allowed here.  Additionally, as
an implementation note, a placeholder `bool` is also included in the union so it
can always be created as "uninitialized".  This means that no destructor is
necessary before the first proper member assignment.

### Tagged Types

This example
``` cddl
x = #6.1234(uint)
```
translates to a single `uint64_t` variable.  The 1234 tag is placed before it
during encoding and the same tag is checked during decoding.

### Caveats

In addition to completely unsupported aspects of CDDL, there are some places
where there are additional constraints placed on accepted CDDL forms.  The
following sections describe these additional constraints.

#### Naming

CDDL allows identifiers to use characters from the set `[a-zA-Z0-9_-@$.]`, but
these do not correspond to valid C++ identifiers or typenames.  As a result, we
need to either restrict the CDDL identifier character set or define a mapping to
C++ identifiers and typenames.  We chose the latter, since CDDL prefers '-' over
'\_'.  The mapping to C++ identifiers is done by converting '-' to '\_' and the
mapping to C++ typenames is done by converting to camel case on words delimited
by '-'.  As a result, `[@$.]` are still disallowed in CDDL identifiers.
Additionally, the names `dead_beef` and `dead-beef` would translate to the same
C++ identifier/typename.

#### Enums

In order to simplify the sharing of enumeration values across messages (see
example below), they are implemented in C++ as enums and not enum classes.  As a
result, the enum constant names are global and cannot be defined more than once.
The example below illustrates how to handle a case where you have odd enum set
intersections.

``` cddl
result = (
  success: 0,
  timeout: 1,
  unknown-error: 2,
)

message1 = {
  result: &(
    result,
    invalid-input: 10,
    internal-error: 20,
  )
}

message2 = {
  result: &(
    result,
    invalid-input: 10,  ; ERROR - redefinition of enum constant in resulting C++
    cancelled: 30,
  )
}
```

``` cddl
result = (
  success: 0,
  timeout: 1,
  unknown-error: 2,
)

invalid-input = (
  invalid-input: 10,
)

message1 = {
  result: &(
    result,
    invalid-input,
    internal-error: 20,
  )
}

message2 = {
  result: &(
    result,
    invalid-input,  ; OK - reference existing enum in resulting C++
    cancelled: 30,
  )
}
```

As a corollary, care should be taken to not allow duplicate enum constant
_values_ in enums that are used together.

**TODO(btolsch): Make this a compiler check.**

## Implementation Overview

The implementation is broken up into the following files:
 - [main.cc](main.cc): Compiler driver.  Command line arguments are:
   - `--header <filename>`: Specify the filename of the output header file.
     This is also the name that will be used for the include guard and as the
     include path in the source file.
   - `--cc <filename>`: Specify the filename of the output source file.
   - `--gen-dir <filename>`: Specify the directory prefix that should be added
     to the output header and source file.
   - A filename (in any position) without a preceding flag specifies the input
     file which contains the CDDL spec.
 - [cddl.py](cddl.py): Python adapter to allow the tool to be invoked as a GN
   action.
 - [parse.cc](parse.cc): Parser which produces a tree of `AstNode`s
   corresponding to the input's derivation in the grammar.
 - [sema.cc](sema.cc):  "Semantic analysis" step (named for clang's semantic
   analysis layer) which generates a table of `CppType`s.  `CppType` represents
   something that will become a C++ type in the final output.
 - [codegen.cc](codegen.cc):  C++ generation step which outputs struct, enum,
   and function declarations to the specified header file and function
   definitions to the specified source file.

### Grammar

Since CDDL is still an IETF draft spec and the grammar has changed at least a
few times, the grammar used for this implementation is duplicated in
[grammar.md](grammar.md).
