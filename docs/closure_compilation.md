# Closure Compilation

## What is type safety?

[Strongly-typed languages](https://en.wikipedia.org/wiki/Strong_and_weak_typing)
like C++ and Java have the notion of variable types.

This is typically baked into how you declare variables:

```c++
const int32 kUniversalAnswer = 42;  // type = 32-bit integer
```

or as [templates](https://en.wikipedia.org/wiki/Template_metaprogramming) for
containers or generics:

```c++
std::vector<int64> fibonacci_numbers;  // a vector of 64-bit integers
```

When differently-typed variables interact with each other, the compiler can warn
you if there's no sane default action to take.

Typing can also be manually annotated via mechanisms like `dynamic_cast` and
`static_cast` or older C-style casts (i.e. `(Type)`).

Using stongly-typed languages provide _some_ level of protection against
accidentally using variables in the wrong context.

JavaScript is weakly-typed and doesn't offer this safety by default. This makes
writing JavaScript more error prone, and various type errors have resulted in
real bugs seen by many users.

## Chrome's solution to typechecking JavaScript

Enter [Closure Compiler](https://developers.google.com/closure/compiler/), a
tool for analyzing JavaScript and checking for syntax errors, variable
references, and other common JavaScript pitfalls.

To get the fullest type safety possible, it's often required to annotate your
JavaScript explicitly with [Closure-flavored @jsdoc
tags](https://developers.google.com/closure/compiler/docs/js-for-compiler)

```js
/**
 * @param {string} version A software version number (i.e. "50.0.2661.94").
 * @return {!Array<number>} Numbers corresponding to |version| (i.e. [50, 0, 2661, 94]).
 */
function versionSplit(version) {
  return version.split('.').map(Number);
}
```

See also:
[the design doc](https://docs.google.com/a/chromium.org/document/d/1Ee9ggmp6U-lM-w9WmxN5cSLkK9B5YAq14939Woo-JY0/edit).

## Typechecking Your Javascript

Given an example file structure of:

  + lib/does_the_hard_stuff.js
  + ui/makes_things_pretty.js

`lib/does_the_hard_stuff.js`:

```javascript
var wit = 100;

// ... later on, sneakily ...

wit += ' IQ';  // '100 IQ'
```

`ui/makes_things_pretty.js`:

```javascript
/** @type {number} */ var mensa = wit + 50;

alert(mensa);  // '100 IQ50' instead of 150
```

Closure compiler can notify us if we're using `string`s and `number`s in
dangerous ways.

To do this, we can create:

  + ui/compiled_resources2.gyp

With these contents:

```
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'targets': [
    {
      # Target names is typically just without ".js"
      'target_name': 'makes_things_pretty',

      'dependencies': [
        '../lib/compiled_resources2.gyp:does_the_hard_stuff',

        # Teaches closure about non-standard environments/APIs, e.g.
        # chrome.send(), chrome.app.window, etc.
        '<(EXTERNS_GYP):extern_name_goes_here'
      ],

      'includes': ['../path/to/third_party/closure_compiler/compile_js2.gypi'],
    },
  ],
}
```

## Running Closure compiler locally

You can locally test that your code compiles on Linux or Mac.  This requires
[Java](http://www.oracle.com/technetwork/java/javase/downloads/index.html) and a
[Chrome checkout](https://www.chromium.org/developers/how-tos/get-the-code) (i.e.
python, depot_tools). Note: on Ubuntu, you can probably just run `sudo apt-get
install openjdk-7-jre`.

Now you should be able to run:

```shell
third_party/closure_compiler/run_compiler
```

and should see output like this:

```shell
ninja: Entering directory `out/Default/'
[0/1] ACTION Compiling ui/makes_things_pretty.js
```

To compile only a specific target, add an argument after the script name:

```shell
third_party/closure_compiler/run_compiler makes_things_pretty
```

In our example code, this error should appear:

```
(ERROR) Error in: ui/makes_things_pretty.js
## /my/home/chromium/src/ui/makes_things_pretty.js:1: ERROR - initializing variable
## found   : string
## required: number
## /** @type {number} */ var mensa = wit + 50;
##                                   ^
```

Hooray! We can catch type errors in JavaScript!

## Trying your change

Closure compilation also has [try
bots](https://build.chromium.org/p/tryserver.chromium.linux/builders/closure_compilation)
which can check whether you could *would* break the build if it was committed.

From the command line, you try your change with:

```shell
git cl try -b closure_compilation
```

To automatically check that your code typechecks cleanly before submitting, you
can add this line to your CL description:

```
CQ_INCLUDE_TRYBOTS=tryserver.chromium.linux:closure_compilation
```

Working in common resource directories in Chrome automatically adds this line
for you.

## Integrating with the continuous build

To compile your code on every commit, add your file to the `'dependencies'` list
in `src/third_party/closure_compiler/compiled_resources2.gyp`:

```
{
  'targets': [
    {
      'target_name': 'compile_all_resources',
      'dependencies': [
         # ... other projects ...
++       '../my_project/compiled_resources2.gyp:*',
      ],
    }
  ]
}
```

This file is used by the
[Closure compiler bot](https://build.chromium.org/p/chromium.fyi/builders/Closure%20Compilation%20Linux)
to automatically compile your code on every commit.

## Externs

[Externs files](https://github.com/google/closure-compiler/wiki/FAQ#how-do-i-write-an-externs-file)
define APIs external to your JavaScript. They provide the compiler with the type
information needed to check usage of these APIs in your JavaScript, much like
forward declarations do in C++.

Third-party libraries like Polymer often provide externs. Chrome must also
provide externs for its extension APIs. Whenever an extension API's `idl` or
`json` schema is updated in Chrome, the corresponding externs file must be
regenerated:

```shell
./tools/json_schema_compiler/compiler.py -g externs \
  extensions/common/api/your_api_here.idl \
  > third_party/closure_compiler/externs/your_api_here.js
```
