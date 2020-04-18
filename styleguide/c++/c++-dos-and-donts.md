# C++ Dos and Don'ts

## A Note About Usage

Unlike the style guide, the content of this page is advisory, not required. You
can always deviate from something on this page, if the relevant
author/reviewer/OWNERS agree that another course is better.


## Minimize Code in Headers

### Don't include unneeded headers

If a file isn't using the symbols from some header, remove the header. It turns
out that this happens frequently in the Chromium codebase due to refactoring.

### Move inner classes into the implementation

You can also forward declare classes inside a class:

```cpp
class Whatever {
 public:
  /* ... */
 private:
  struct DataStruct;
  std::vector<DataStruct> data_;
};
```

Any headers that DataStruct needed no longer need to be included in the header
file and only need to be included in the implementation. This will often let you
pull includes out of the header. For reference, the syntax in the implementation
file is:

```cpp
struct Whatever::DataStruct {
};
```

Note that sometimes you can't do this because certain STL data structures
require the full definition at declaration time (most notably, std::deque and
the STL adapters that wrap it).

### Move static implementation details to the implementation whenever possible

If you have the class in a header file, you should try to move that from a class
member into the anonymous namespace in the implementation file:

DON'T:

```cpp
#include "BigImplementationDetail.h"
class PublicInterface {
  public:
   /* ... */
  private:
   static BigImplementationDetail detail_;
};
```

DO:

```cpp
namespace {
BigImplementationDetail g_detail;
}  // namespace
```

That way, people who don't use your interface don't need to know about or care
about `BigImplementationDetail`.

You can do this for helper functions, too.  Note that if there is more than one
class in the .cc file, it can aid clarity to define your translation-unit-scope
helpers in an anonymous namespace just above the class that uses them, instead
of at the top of the file.

## Stop inlining code in headers

*** note
**BACKGROUND**: Unless something is a cheap accessor or you truly need it to be
inlined, don't ask for it to be inlined.  Remember that definitions inside class
declarations are implicitly requested to be inlined.
***

DON'T:

```cpp
class InlinedMethods {
  InlinedMethods() {
    // This constructor is equivalent to having the inline keyword in front
    // of it!
  }
  void Method() {
    // Same here!
  }
};
```

### Stop inlining complex methods.

DON'T:

```cpp
class DontDoThis {
 public:
  int ComputeSomething() {
    int sum = 0;
    for (int i = 0; i < limit; ++i) {
      sum += OtherMethod(i, ... );
    }
    return sum;
  }
};
```

A request to inline is merely a suggestion to the compiler, and anything more
than a few operations on integral data types will probably not be inlined.
However, every file that has to use an inline method will also emit a function
version in the resulting .o, even if the method was inlined. (This is to support
function pointer behavior properly.) Therefore, by requesting an inline in this
case, you're likely just adding crud to the .o files which the linker will need
to do work to resolve.

If the method has significant implementation, there's also a good chance that by
not inlining it, you could eliminate some includes.

### Stop inlining virtual methods

You can't inline virtual methods under most circumstances, even if the method
would otherwise be inlined because it's very short. The compiler must do runtime
dispatch on any virtual method where the compiler doesn't know the object's
complete type, which rules out the majority of cases where you have an object.

### Stop inlining constructors and destructors

Constructors and destructors are often significantly more complex than you think
they are, especially if your class has any non-POD data members. Many STL
classes have inlined constructors/destructors which may be copied into your
function body. Because the bodies of these appear to be empty, they often seem
like trivial functions that can safely be inlined.  Don't give in to this
temptation.  Define them in the implementation file unless you really _need_
them to be inlined.  Even if they do nothing now, someone could later add
something seemingly-trivial to the class and make your hundreds of inlined
destructors much more complex.

Even worse, inlining constructors/destructors prevents you from using forward
declared variables:

DON'T:

```cpp
class Forward;
class WontCompile {
 public:
   // THIS WON'T COMPILE, BUT IT WOULD HAVE IF WE PUT THESE IN THE
   // IMPLEMENTATION FILE!
   //
   // The compiler needs the definition of Forward to call the
   // vector/scoped_ptr ctors/dtors.
   Example() { }
   ~Example() { }

 private:
  std::vector<Forward> examples_;
  scoped_ptr<Forward> super_example_;
};
```

For more information, read Item 30 in Effective C++.

### When you CAN inline constructors and destructors

C++ has the concept of a
[trivial destructor](http://publib.boulder.ibm.com/infocenter/macxhelp/v6v81/index.jsp?topic=/com.ibm.vacpp6m.doc/language/ref/clrc15cplr380.htm).
If your class has only POD types and does not explicitly declare a destructor,
then the compiler will not bother to generate or run a destructor.

```cpp
struct Data {
  Data() : count_one(0), count_two(0) {}
  // No explicit destructor, thus no implicit destructor either.

  // The members must all be POD for this trick to work.
  int count_one;
  int count_two;
};
```

In this example, since there is no inheritance and only a few POD members, the
constructor will be only a few trivial integer operations, and thus OK to
inline.

For abstract base classes with no members, it's safe to define the (trivial)
destructor inline:

```cpp
class Interface {
 public:
  virtual ~Interface() {}

  virtual void DoSomething(int parameter) = 0;
  virtual int GetAValue() = 0;
};
```
But be careful; these two "interfaces" don't count:

DON'T:

```cpp
class ClaimsToBeAnInterface : public base::RefCounted<ClaimsToBeAnInterface> {
 public:
  virtual ~ClaimsToBeAnInterface() { /* But derives from a template! */ }
};

class HasARealMember {
 public:
  virtual void InterfaceMethod() = 0;
  virtual ~HasARealMember() {}

 protected:
  vector<string> some_data_;
};
```

If in doubt, don't rely on these sorts of exceptions.  Err on the side of not
inlining.

### Be careful about your accessors

Not all accessors are light weight. Compare:

```cpp
class Foo {
 public:
  int count() const { return count_; }

 private:
  int count_;
};
```

Here the accessor is trivial and safe to inline.  But the following code is
probably not, even though it also looks simple:

DON'T:

```cpp
struct MyData {
  vector<GURL> urls_;
  base::Time last_access_;
};

class Manager {
 public:
  MyData get_data() { return my_data_; }

 private:
  MyData my_data_;
};
```

The underlying copy constructor calls for MyData are going to be complex. (Also,
they're going to be synthesized, which is bad.)

### What about code outside of headers?

For classes declared in .cc files, there's no risk of bloating several .o files
with the definitions of the same "inlined" function.  While there are other,
weaker arguments to continue to avoid inlining, the primary remaining
consideration is simply what would make code most readable.

This is especially true in testing code.  Test framework classes don't tend to
be instantiated separately and passed around as objects; they're effectively
just bundles of translation-unit-scope functionality coupled with a mechanism
to reset state between tests.  In these cases, defining the test functions
inline at their declaration sites has little negative effect and can reduce the
amount of "boilerplate" in the test file.

Different reviewers may have different opinions here; use good judgment.

## Static variables

Dynamic initialization of function-scope static variables is now thread**safe**
in Chromium (per standard C++11 behavior). Before 2017, this was thread-
unsafe, and base::LazyInstance was widely used. This is no longer necessary.
Background can be found in this
[thread](https://groups.google.com/a/chromium.org/forum/#!msg/chromium-dev/p6h3HC8Wro4/HHBMg7fYiMYJ)
and this
[thread](https://groups.google.com/a/chromium.org/d/topic/cxx/j5rFewBzSBQ/discussion).

```cpp
void foo() {
    static int ok_count = ComputeTheCount();  // OK now, previously a problem.
    static int good_count = 42;  // C++03 3.6.2 says this is done before dynamic initialization, so probably thread-safe.
    static constexpr int better_count = 42;  // Even better, as this will now likely be inlined at compile time.
    static auto& object = *new Object;  // For class types.
}
```

## Variable initialization

There are myriad ways to initialize variables in C++11.  Prefer the following
general rules:

1. Use assignment syntax when performing "simple" initialization with one or
   more literal values which will simply be composed into the object:

   ```cpp
   int i = 1;
   std::string s = "Hello";
   std::pair<bool, double> p = {true, 2.0};
   std::vector<std::string> v = {"one", "two", "three"};
   ```

   Using '=' here is no less efficient than "()" (the compiler won't generate a
   temp + copy), and ensures that only implicit constructors are called, so
   readers seeing this syntax can assume    nothing complex or subtle is
   happening.  Note that "{}" are allowed on the right side of the '=' here
   (e.g. when you're merely passing a set of initial values to a "simple"
   struct/   container constructor; see below items for contrast).

2. Use constructor syntax when construction performs significant logic, uses an
   explicit constructor, or in some other way is not intuitively "simple" to the
   reader:

   ```cpp
   MyClass c(1.7, false, "test");
   std::vector<double> v(500, 0.97);  // Creates 50 copies of the provided initializer
   ```

3. Use C++11 "uniform init" syntax ("{}" without '=') only when neither of the
   above work:

   ```cpp
   class C {
    public:
     explicit C(bool b) { ... };
     ...
   };
   class UsesC {
     ...
    private:
     C c{true};  // Cannot use '=' since C() is explicit (and "()" is invalid syntax here)
   };
   class Vexing {
    public:
     explicit Vexing(const std::string&amp; s) { ... };
     ...
   };
   void func() {
     Vexing v{std::string()};  // Using "()" here triggers "most vexing parse";
                               // "{}" is arguably more readable than "(())"
     ...
   }
   ```

4.  Never mix uniform init syntax with auto, since what it deduces is unlikely
    to be what was intended:

   ```cpp
   auto x{1};  // Until C++17, decltype(x) is std::initializer_list<int>, not int!
   ```

## Prefer `make_unique` to `WrapUnique`

[`std::make_unique`](http://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique)`<Type>(...)`
and
[`base::WrapUnique`](https://cs.chromium.org/chromium/src/base/memory/ptr_util.h?q=WrapUnique)`(new Type(...))`
are equivalent.
`std::make_unique` should be preferred, because it is harder to use unsafely
than `WrapUnique`. In general, bare calls to `new` require careful scrutiny.
Bare calls to `new` are currently required to construct reference-counted types;
however, reference counted types themselves require careful scrutiny.

```cpp
return std::unique_ptr<C>(new C(1, 2, 3));  // BAD: type name mentioned twice
return base::WrapUnique(new C(1, 2, 3));    // BAD: bare call to new
return std::make_unique<C>(1, 2, 3);        // GOOD
```

**Notes:**

1. Never friend `std::make_unique` to work around constructor access
   restrictions. It will allow anyone to construct the class. Use `WrapUnique`
   in this case.

   DON'T:
   ```cpp
   class Bad {
    public:
     std::unique_ptr<Bad> Create() { return std::make_unique<Bad>(); }
     // ...
    private:
     Bad();
     // ...
     friend std::unique_ptr<Bad> std::make_unique<Bad>();  // Lost access control
   };
   ```

   DO:
   ```cpp
   class Okay {
    public:
     // For explanatory purposes. If Create() adds no value, it is better just
     // to have a public constructor instead.
     std::unique_ptr<Okay> Create() { return base::WrapUnique(new Okay()); }
     // ...
    private:
     Okay();
     // ...
   };
   ```

2. `WrapUnique(new Foo)` and `WrapUnique(new Foo())` mean something different if
   `Foo` does not have a user-defined constructor. Don't make future maintainers
   guess whether you left off the '()' on purpose. Use `std::make_unique<Foo>()`
   instead. If you're intentionally leaving off the "()" as an optimisation,
   please leave a comment.

   ```cpp
   auto a = base::WrapUnique(new A); // BAD: "()" omitted intentionally?
   auto a = std::make_unique<A>();   // GOOD
   // "()" intentionally omitted to avoid unnecessary zero-initialisation.
   // WrapUnique() does the wrong thing for array pointers.
   auto array = std::unique_ptr<A[]>(new A[size]);
   ```

See also [TOTW 126](https://abseil.io/tips/126).

## Do not use `auto` to deduce a raw pointer

The use of the `auto` keyword to deduce the type from the initializing
expression is encouraged when it improves readability. However, do not use
`auto` when the type would be deduced to be a pointer type. This can cause
confusion. Instead, prefer specifying the "pointer" part outside of `auto`:

```cpp
auto item = new Item();  // BAD: auto deduces to Item*, type of |item| is Item*
auto* item = new Item(); // GOOD: auto deduces to Item, type of |item| is Item*
```

## Use `const` correctly

*** promo
**TLDR:** For safety and simplicity, **don't return pointers or references to
non-const objects from const methods**. Within that constraint, **mark methods
as const where possible**.  **Avoid `const_cast` to remove const**, except when
implementing non-const getters in terms of const getters.
***

### A brief primer on const

To the compiler, the `const` qualifier on a method refers to _physical
constness_: calling this method does not change the bits in this object.  What
we want is _logical constness_, which is only partly overlapping: calling this
method does not affect the object in ways callers will notice, nor does it give
you a handle with the ability to do so.

Mismatches between these concepts can occur in both directions.  When something
is logically but not physically const, C++ provides the `mutable` keyword to
silence compiler complaints.  This is valuable for e.g. cached calculations,
where the cache is an implementation detail callers do not care about.  When
something is physically but not logically const, however, the compiler will
happily accept it, and there are no tools that will automatically save you.
This discrepancy usually involves pointers.  For example,

```cpp
void T::Cleanup() const { delete pointer_member_; }
```

Deleting a member is a change callers are likely to care about, so this is
probably not logically const.  But because `delete` does not affect the pointer
itself, but only the memory it points to, this code is physically const, so it
will compile.

Or, more subtly, consider this pseudocode from a node in a tree:

```cpp
class Node {
 public:
  void RemoveSelf() { parent_->RemoveChild(this); }
  void RemoveChild(Node* node) {
    if (node == left_child_)
      left_child_ = nullptr;
    else if (node == right_child_)
      right_child_ = nullptr;
  }
  Node* left_child() const { return left_child_; }
  Node* right_child() const { return right_child_; }

 private:
  Node* parent_;
  Node* left_child_;
  Node* right_child_;
};
```

The `left_child()` and `right_child()` methods don't change anything about
`|this|`, so making them `const` seems fine.  But they allow code like this:

```cpp
void SignatureAppearsHarmlessToCallers(const Node& node) {
  node.left_child()->RemoveSelf();
  // Now |node| has no |left_child_|, despite having been passed in by const ref.
}
```

The original class definition compiles, and looks locally fine, but it's a
timebomb: a const method returning a handle that can be used to change the
system in ways that affect the original object.  Eventually, someone will
actually modify the object, potentially far away from where the handle is
obtained.

These modifications can be difficult to spot in practice.  As we see in the
previous example, splitting related concepts or state (like "a tree") across
several objects means a change to one object affects the behavior of others.
And if this tree is in turn referred to by yet more objects (e.g. the DOM of a
web page, which influences all sorts of other data structures relating to the
page), then small changes can have visible ripples across the entire system.  In
a codebase as complex as Chromium, it can be almost impossible to reason about
what sorts of local changes could ultimately impact the behavior of distant
objects, and vice versa.

"Logically const correct" code assures readers that const methods will not
change the system, directly or indirectly, nor allow callers to easily do so.
They make it easier to reason about large-scale behavior.  But since the
compiler verifies physical constness, it will not guarantee that code is
actually logically const.  Hence the recommendations here.

### Classes of const (in)correctness

In a
[larger discussion of this issue](https://groups.google.com/a/chromium.org/d/topic/platform-architecture-dev/C2Szi07dyQo/discussion),
Matt Giuca
[postulated three classes of const(in)correctness](https://groups.google.com/a/chromium.org/d/msg/platform-architecture-dev/C2Szi07dyQo/lbHMUQHMAgAJ):

* **Const correct:** All code marked "const" is logically const; all code that
  is logically const is marked "const".
* **Const okay:** All code marked "const" is logically const, but not all code
  that is logically const is marked "const".  (Basically, if you see "const" you
  can trust it, but sometimes it's missing.)
* **Const broken:** Some code marked "const" is not logically const.

The Chromium codebase currently varies. A significant amount of Blink code is
"const broken". A great deal of Chromium code is "const okay". A minority of
code is "const correct".

While "const correct" is ideal, it can take a great deal of work to achieve.
Const (in)correctness is viral, so fixing one API often requires a yak shave.
(On the plus side, this same property helps prevent regressions when people
actually use const objects to access the const APIs.)

At the least, strive to convert code that is "const broken" to be "const okay".
A simple rule of thumb that will prevent most cases of "const brokenness" is for
const methods to never return pointers to non-const objects.  This is overly
conservative, but less than you might think, due to how objects can transitively
affect distant, seemingly-unrelated parts of the system.  The discussion thread
linked above has more detail, but in short, it's hard for readers and reviewers
to prove that returning pointers-to-non-const is truly safe, and will stay safe
through later refactorings and modifications.  Following this rule is easier
than debating about whether individual cases are exceptions.

One way to ensure code is "const okay" would be to never mark anything const.
This is suboptimal for the same reason we don't choose to "never write comments,
so they can never be wrong".  Marking a method "const" provides the reader
useful information about the system behavior.  Also, despite physical constness
being different than logical constness, using "const" correctly still does catch
certain classes of errors at compile time. Accordingly, the
[Google style guide requests the use of const where possible](http://google.github.io/styleguide/cppguide.html#Use_of_const),
so mark methods const when they are logically const.

Making code more const correct leads to cases where duplicate const and non-const getters are required:

```cpp
const T* Foo::GetT() const { return t_; }
T* Foo::GetT() { return t_; }
```

If the implementation of GetT() is complex, there's a
[trick to implement the non-const getter in terms of the const one](https://stackoverflow.com/questions/123758/how-do-i-remove-code-duplication-between-similar-const-and-non-const-member-func/123995#123995),
courtesy of _Effective C++_:

```cpp
T* Foo::GetT() { return const_cast<T*>(static_cast<const Foo*>(this)->GetT()); }
```

While this is a mouthful, it does guarantee the implementations won't get out of
sync and no const-incorrectness will occur. And once you've seen it a few times,
it's a recognizable pattern.

This is probably the only case where you should see `const_cast` used to remove
constness.  Its use anywhere else is generally indicative of either "const
broken" code, or a boundary between "const correct" and "const okay" code that
could change to "const broken" at any future time without warning from the
compiler.  Both cases should be fixed.


## Prefer to use `=default`

Use `=default` to define special member functions where possible, even if the
default implementation is just {}. Be careful when defaulting move operations.
Moved-from objects must be in a valid but unspecified state, i.e., they must
satisfy the class invariants, and the default implementations may not achieve
this.

```cpp
class Good {
 public:
  // We can, and usually should, provide the default implementation separately
  // from the declaration.
  Good();

  // Use =default here for consistency, even though the implementation is {}.
  ~Good() = default;
  Good(const Good& other) = default;

 private:
  std::vector<int> v_;
};

Good::Good() = default;
```

### What are the advantages of `=default?`?

* Compiler-defined copy and move operations don't need maintenance every time
  members are added or removed.
* Compiler-provided special member functions can be "trivial" (if defaulted in
  the class), and can be better optimized by the compiler and library.
* Types with defaulted constructors can be aggregates (if defaulted in the
  class), and hence support aggregate initialization. User provided constructors
  disqualify a class from being an aggregate.
* Defaulted functions are constexpr if the implicit version would have been (and
  if defaulted in the class).
* Using `=default` consistently helps readers identify customized operations.
