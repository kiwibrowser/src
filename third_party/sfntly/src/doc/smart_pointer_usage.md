Usage of smart pointers in sfntly C++ port
=========================================================================================================================================================

In sfntly C++ port, an object ref-counting and smart pointer mechanism
is implemented. The implementation works very much like COM.

Ref-countable object type inherits from RefCounted&lt;&gt;, which have
addRef() and release() just like IUnknown in COM (but no
QueryInterface). Ptr&lt;&gt; is a smart pointer class like
CComPtr&lt;&gt; which is used to hold the ref-countable objects so that
the object ref count is handled correctly.

Lets take a look at the example:

``` {.prettyprint}
class Foo : public RefCounted<Foo> {
 public:
  static Foo* CreateInstance() {
    Ptr<Foo> obj = new Foo();  // ref count = 1
    return obj.detach();  // Giving away the control of this instance.
  }
};
typedef Ptr<Foo> FooPtr;  // Common short-hand notation.

FooPtr obj;
obj.attach(Foo::CreateInstance());  // ref count = 1
                                    // Take over control but not bumping
                                    // ref count.
{
  FooPtr obj2 = obj;  // ref count = 2
                      // Assignment bumps up ref count.
}  // ref count = 1
   // obj2 out of scope, decrements ref count

obj.release();  // ref count = 0, object destroyed
```

Notes on usage
---------------------------------------------------------------------

-   Virtual inherit from RefCount interface in base class if smart
    pointers are going to be defined.

<!-- -->

-   All RefCounted objects must be instantiated on the heap. Allocating
    the object on stack will cause crash.

<!-- -->

-   Be careful when you have complex inheritance. For example,

> In this case the smart pointer is pretty dumb and dont count on it to
> nicely destroy your objects as designed. Try refactor your code like
>
> ``` {.prettyprint}
> class I;  // the common interface and implementations
> class A : public I, public RefCounted<A>;  // A specific implementation
> class B : public I, public RefCounted<B>;  // B specific implementation
> ```

-   Smart pointers here are very bad candidates for function parameters
    and return values. Use dumb pointers when passing over the stack.

<!-- -->

-   When down\_cast is performed on a dangling pointer due to bugs in
    code, VC++ will generate SEH which is not handled well in VC++
    debugger. One can use WinDBG to run it and get the faulting stack.

<!-- -->

-   Idioms for heap object as return value

> If you are not passing that object back, you are the end of scope.

> Be very careful when using the assignment operator as opposed to
> attaching. This can easily cause memory leaks. Have a look at [The
> difference between assignment and attachment with ATL smart
> pointers](http://blogs.msdn.com/b/oldnewthing/archive/2009/11/20/9925918.aspx).
> Since were using essentially the same model, it applies in pretty much
> the same way. The easiest way of knowing when to Attach and when to
> assign is to check the declaration of the function you are using.
>
> ``` {.prettyprint}
>   // Attach to the pointer returned by GetInstance
>   static CALLER_ATTACH FontFactory* GetInstance();
>
>   // Assign pointer returned by NewCMapBuilder 
>   CMap::Builder* NewCMapBuilder(const CMapId& cmap_id,
>                                 ReadableFontData* data);
> ```

Detecting Memory Leak
------------------------------------------------------------------------------------------

> The implementation of COM-like ref-counting and smart pointer remedies
> the lack of garbage collector in C++ to certain extent, however, it
> also introduces other problems. The most common one is memory leakage.
> How do we know that our code leak memory or not?

-   For Linux/Mac, valgrind
    [http://valgrind.org](http://valgrind.org/)
    is very useful.
-   For Windows, the easiest is to use Visual C++ built-in CRT debugging
    [http://msdn.microsoft.com/en-us/library/x98tx3cf(v=vs.80).aspx](http://msdn.microsoft.com/en-us/library/x98tx3cf(v=vs.80).aspx)

Useful Tips for Debugging Ref-Couting Issue
------------------------------------------------------------------------------------------------------------------------------------------------------------

-   Define `ENABLE_OBJECT_COUNTER` and `REF_COUNT_DEBUGGING`. All
    ref-count related activity will be ouput to stderr.

<!-- -->

-   The logs will look like (under VC2010)

> Use your favorite editor to transform them into SQL statements, e.g.
> Regex pattern
>
> ``` {.prettyprint}
> ^([ACDR]) class RefCounted<class sfntly::([A-Za-z0-9:]+)>[ *const]+:oc=([-0-9]+),oid=([0-9]+),rc=([-0-9]+)
> ```

> Replace to
>
> ``` {.prettyprint}
> insert into log values(\1, \2, \3, \4, \5);
> ```

-   Add one line to the beginning of log

<!-- -->

-   Run sqlite shell, use .read to input the SQL file.

<!-- -->

-   Run following commands to get the leaking object class and object
    id:

<!-- -->

-   Once you know which object is leaking, its much easier to setup
    conditional breakpoints to identify the real culprit.
