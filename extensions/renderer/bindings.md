# Extension Bindings

[TOC]

## What Is It?

The Bindings System is responsible for creating the JS entry points for APIs.
It creates the `chrome` object (if it does not exist) and adds the API objects
(e.g. `tabs`) that should be accessible to the context.

## Initialization

Bindings are initialized by creating an ObjectTemplate from an API specification
and stamping out copies of this template.  This means that once an API is
instantiated once, further instantiations within that same process are
significantly faster.  The API itself is specified from a .json or .idl file in
extensions/common/api or chrome/common/extensions/api.

This is slightly complicated because APIs may have features (such as specific
methods or events) that are restricted in certain contexts, even if the rest of
the API is available.  As a result, after object instantiation, there’s a chance
we may have to alter the object in order to remove these unavailable features.

## API Features

A "feature" of an API is a property on the API object to expose some
functionality.  There are three main types of features exposed on APIs.

* __Functions__:
Functions are the main type of feature exposed on APIs.  They allow callers to
interact with the browser and trigger behavior.

* __Events__:
Most events are dispatched when something happens to inform an interested party
of the instance.  Callers subscribe to the events they are interested in, and
are notified only for subscribed events.  While most events do not influence
behavior change in the browser, declarative events may.

* __Properties__:
Certain APIs have exposed properties that are accessed directly on the API
object.  These are frequently constants (including enum definitions), but are
also sometimes properties relating to the state of the context.

## Restriction

Not all APIs are available to all contexts; we restrict which capabilities are
exposed based on multiple factors.

### Scope

Features may be restricted at multiple scopes.  The most common is at the
API-scope - where none of the API will be made available if the requirements
aren’t met.  In this case, the chrome.<apiName> property will simply be
undefined.  However, we also have the ability to restrict features on a more
granular scope, such as at the method or event level.  In this case, even though
most of an API may be available, a certain function might not be; or,
conversely, only a small subset of features may be available while the rest of
the API is restricted.

### Restricting Properties
Feature restrictions are based on a specific v8::Context.  Different
contexts within the same frame may have different API availabilities (this is
significantly different than the web platform, where features are exposed at the
frame-level).  The bindings system takes into account context type, associated
extensions, URL, and more when evaluating features; for more information, see
the [feature documentation](/chrome/common/extensions/api/_features.md).

## Typical Function Flow

The typical flow for all API methods is the same.  A JS entry point (the method
on the API object) leads to a common native implementation.  This implementation
has the following steps:

* __Argument Parsing__:

Passed arguments are parsed against an expected signature defined in the API
specification.  If the passed arguments match the signature, the arguments are
normalized and converted to a serialized format (base::Value).
* __Request Dispatch__:
A request is dispatched with the parsed arguments and other information about
the request (such as requesting context and user gesture status).  If a callback
is included in the arguments, it is stored (along with other information about
the request) until the response is received.
* __Request Response__:
A response is provided asynchronously, indicating success or failure, along with
any return values (to pass to a provided callback) or an error message.  The
pending request is removed.

## Custom Function Hooks

Certain APIs need to deviate from this typical flow in order to customize
behavior.  We provide the following general custom hooks for APIs to modify the
typical behavior.

* __updateArgumentsPreValidate__:
Allows an API implementation to modify passed arguments before the argument
signature is validated.  This can be useful in the case of undocumented
(internal) parameters or properties, such as a generated ID.
* __updateArgumentsPostValidate__:
Allows an API implementation to modify passed arguments after the argument
signature is validated, but before the request is handled.  Note: this is
usually bad practice, as any modification means that the arguments no longer
match the expected signature.  This can cause headaches when we attempt to
deserialize these values.
* __handleRequest__:
Allows an API implementation to internally handle a request.  This is useful
when the request itself should not go through the normal flow, such as when the
logic requires a greater level of involvement on the renderer, or is entirely
handled without needing to message the browser.
* __customCallback__:
Allows an API implementation to add a callback that should be called with the
result of an API function call before the caller’s callback is invoked.  It is
the responsibility of the custom callback to invoke the original callback, which
is passed as an argument.  This is useful when the return results should be
mutated before returning to the caller (which can be necessary when the eventual
result could be a renderer-specific concept, such as a DOMWindow).

An API implementation may use one or more of these hooks.

### Registering Hooks

Custom Hooks can be registered through either native or JS bindings. In native
bindings, APIs can subclass APIBindingHooksDelegate and register themselves with
the bindings system. This typically happens during the bootstrapping of the
renderer process. Native binding hooks are the preferred approach for new
bindings.

We also expose hooks in JS through the APIBindingBridge object, which provides
a registerCustomHook method to allow APIs to create hooks in JS. This style of
custom hooks is __not preferred__ and will be __deprecated__. These are bad
because a) JS is much more susceptible to untrusted code and b) since these run
on each object instantiation, the performance cost is significantly higher.

## Events

Events are dispatched when the associated action occurs.

### Types

There are three types of events.

* __Regular__:
These events are dispatched to the subscriber when something happens, and merely
serve as a notification to allow the subscriber to react.
* __Declarative__:
Declarative events allow a subscriber to specify some action to be taken when an
event occurs.  For instance, the declarativeContent API allows a subscriber to
indicate that an action should be shown whenever a certain URL pattern or CSS
rule is matched.  For these events, the subscriber is not notified when the
event happens; rather, the browser immediately takes the specified action.  By
virtue of not notifying the subscriber, we help preserve the user’s privacy; if
a subscriber says "do X when the user visits example.com", it does not know
whether the user visited example.com.  (Note: subsequent actions, such as a user
interacting with the action on a given page, can expose this.)
* __Imperative__:
A few events are designed to be dispatched and to return a response from the
subscriber, indicating an action the browser should take.  These are
predominantly used in the webRequest API, where a subscriber can register events
for navigations, receive notifications of those navigations, and return a result
of whether the navigation should continue, cancel, or redirect.  These events
are generally discouraged for performance reasons, and declarative events are
preferred.

### Filters

Certain events also allow the registration of filters, which allow subscribers
to only be notified of a subset of events.  For example, the webNavigation and
webRequest APIs allow filtering by URL pattern, so that uninteresting
navigations are ignored.

## Legacy JavaScript Implementations

The prior bindings system was implemented primarily in JavaScript, rather than
utilizing native code.  There were many reasons for this, but they include ease
of coding and more limited interactions with Blink (WebKit at the time) and V8.
Unfortunately, this led to numerous security vulnerabilities (because untrusted
code can run in the same context) and performance issues (because bindings were
set up per context, and could not be cached in any way).

While the native bindings system replaces the core functionality with a native
implementation, individual APIs may still be implemented in JavaScript custom
bindings, or hooks.  These should eventually be replaced by native-only
implementations.

## Differences Between Web/Blink Bindings

There are a number of differences between the Extensions Bindings System and
Blink Bindings.

### Common Implementation to Optimize Binary Size

Most Extension APIs are implemented in the browser process after a common flow
in the renderer.  This allows us to optimize the renderer implementation for
space and have the majority of APIs lead to a single entry point, which can
match an API against an expected schema.  This is contrary to Blink Bindings,
which set up a distinct separate entry point for each API, and then individually
parses the expected results.

The Blink implementation provides greater speed, but comes at a larger generated
code cost, since each API has its own generated parsing and handling code.
Since most Blink/open web APIs are implemented in the renderer, this cost is not
as severe - each API would already require specialized code in the renderer.

Extension APIs, on the other hand, are predominantly implemented in the browser;
this means we can optimize space by having a single parsing/handling point.
This is also beneficial because many extension APIs are exposed on a more
limited basis, where only a handful of contexts need access to them, and thus
the binary size savings is more valuable, and the speed cost less harmful.

### Signature Matching

Signature matching differs significantly between WebIDL and Extension APIs.

#### Optional Inner Parameters

Unlike OWP APIs, Extension APIs allow for optional inner parameters.  For
instance, if an API has the signature `(integer, optional string, optional
function)`, it may be invoked with `(integer, function)` - which would not be
valid in the OWP.  This also allows for inner parameters to be optional with
subsequent required parameters, such as `(integer, optional string, function)` -
again, something which would be disallowed on the OWP.

#### Unknown Properties

Unknown properties on objects are, by default, unallowed.  That is, if a
function accepts an object that has properties of `foo` and `bar`, passing
`{foo: <foo>, bar: <bar>, baz: <baz>}` is invalid.
